#include "evse_cp_sampler.h"
#include "esphome/core/log.h"

namespace esphome {
namespace evse_cp_sampler {

static const char *const TAG = "evse_cp_sampler";

void CpSampler::setup() {
  count_ = 0;
  median_index_ = 0;
  median_count_ = 0;
  prev_state_ = -1;

  // Fill buffer with 0 initially (or some safe value)
  for (int i = 0; i < MEDIAN_WINDOW; i++) {
    median_buffer_[i] = 0;
  }

  // PWM GPIO interrupt setup (rising edge on GPIO10)
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/api-reference/peripherals/gpio.html
  gpio_set_intr_type(GPIO_NUM_10, GPIO_INTR_POSEDGE);
  gpio_install_isr_service(0);
  gpio_isr_handler_add(GPIO_NUM_10, [](void *arg) {
    auto self = static_cast<CpSampler *>(arg);
    self->start_sample_timer();
  }, this);
  gpio_intr_enable(GPIO_NUM_10);

  // ADC oneshot setup (GPIO0 = ADC1_CHANNEL_0)
  // https://documentation.espressif.com/api/resource/doc/file/aY69Zg1p/FILE/esp32-c3_technical_reference_manual_en.pdf p.870
  adc_oneshot_unit_init_cfg_t init_cfg = {
    .unit_id = ADC_UNIT_1,
    .ulp_mode = ADC_ULP_MODE_DISABLE,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle_));

  adc_oneshot_chan_cfg_t chan_cfg = {
    .atten = ADC_ATTEN_DB_12,
    .bitwidth = ADC_BITWIDTH_DEFAULT,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle_, ADC_CHANNEL_0, &chan_cfg));

  // One-shot sampling timer (20 µs after rising edge)
  esp_timer_create_args_t timer_args = {
      .callback = timer_callback,
      .arg = this,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "cp_sample",
      .skip_unhandled_events = true,
  };
  esp_timer_create(&timer_args, &sample_timer_);

  // Create periodic heartbeat timer (500 Hz = 2000 µs period)
  esp_timer_create_args_t heartbeat_args = {
      .callback = timer_callback,
      .arg = this,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "cp_heartbeat",
      .skip_unhandled_events = true,
  };
  esp_timer_create(&heartbeat_args, &heartbeat_timer_);

  // Start heartbeat (periodic) timer immediately
  esp_timer_start_periodic(heartbeat_timer_, 2000);  // 2000 µs = 500 Hz
}

void CpSampler::start_sample_timer() {
  // Restart heartbeat timer (resets its period) so that it will not trigger if under duty cycle
  esp_timer_restart(heartbeat_timer_, 2000);
  esp_timer_start_once(sample_timer_, 20);  // 20 µs delay
}

int CpSampler::compute_median() {
  // Copy buffer to avoid modifying original during sort
  int sorted[MEDIAN_WINDOW];
  for (int i = 0; i < MEDIAN_WINDOW; i++) {
    sorted[i] = median_buffer_[i];
  }
  std::sort(sorted, sorted + MEDIAN_WINDOW);
  return sorted[MEDIAN_WINDOW / 2];
}

void IRAM_ATTR CpSampler::timer_callback(void *arg) {
  auto self = static_cast<CpSampler *>(arg);

  // Only sample ADC every DECIMATE_SAMPLES calls
  if (self->count_ % DECIMATE_SAMPLES == 0) {
    int raw_adc;
    esp_err_t ret = adc_oneshot_read(self->adc_handle_, ADC_CHANNEL_0, &raw_adc);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "ADC read failed: %s", esp_err_to_name(ret));
      return;
    }

    // Store in circular buffer
    self->median_buffer_[self->median_index_] = raw_adc;
    self->median_index_ = (self->median_index_ + 1) % self->MEDIAN_WINDOW;
    if (self->median_count_ < self->MEDIAN_WINDOW) {
      self->median_count_++;
    }
  }

  self->count_ += 1;

  // Only process when we've collected enough PWM cycles
  if (self->count_ < self->samples_) {
    return;
  }

  // Reset counter
  self->count_ = 0;

  // Only trigger if we have enough samples for a valid median
  if (self->median_count_ < self->MEDIAN_WINDOW) {
    return;  // Not enough data yet
  }

  int filtered_value = self->compute_median();

  // Trigger raw value (now median-filtered)
  if (self->raw_value_trigger_) {
    self->raw_value_trigger_->trigger(filtered_value);
  }

  // --- State detection based on median-filtered value ---
  int new_state = 0;
  if (filtered_value > 4000) {
    new_state = 1;           // +12V → State A (no vehicle)
  } else if (abs(filtered_value - 3650) < 150) {
    new_state = 2;           // +9V  → State B (connected, not ready)
  } else if (abs(filtered_value - 3200) < 150) {
    new_state = 3;           // +6V  → State C (charging)
  } else if (abs(filtered_value - 755) < 150) {
    new_state = 4;           // -12V → State E/F (error or ventilation)
  }

  if (new_state != self->prev_state_) {
    if (self->state_change_trigger_) {
      self->state_change_trigger_->trigger(new_state);
    }
    self->prev_state_ = new_state;
  }
}

void CpSampler::dump_config() {
  ESP_LOGCONFIG(TAG, "EVSE CP Sampler:");
  ESP_LOGCONFIG(TAG, "  PWM on GPIO10, ADC on GPIO0");
  ESP_LOGCONFIG(TAG, "  Samples per update: %d", samples_);
  ESP_LOGCONFIG(TAG, "  Median filter window: %d", MEDIAN_WINDOW);
  ESP_LOGCONFIG(TAG, "  Decimation factor: %d", DECIMATE_SAMPLES);
}

}  // namespace evse_cp_sampler
}  // namespace esphome
