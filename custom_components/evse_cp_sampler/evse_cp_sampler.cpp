#include "evse_cp_sampler.h"
#include "esphome/core/log.h"
#include "driver/gpio.h"
#include "esp_timer.h"

namespace esphome {
namespace evse_cp_sampler {

static const char *const TAG = "evse_cp_sampler";

void CpSampler::setup() {
  sum_raw_values_ = 0;
  counter_ = 0;

  gpio_set_direction((gpio_num_t)pwm_pin_, GPIO_MODE_INPUT);

  gpio_set_intr_type((gpio_num_t)pwm_pin_, GPIO_INTR_POSEDGE);
  gpio_install_isr_service(0);
  gpio_isr_handler_add((gpio_num_t)pwm_pin_, [](void *arg) {
    auto self = static_cast<CpSampler *>(arg);
    self->start_sample_timer();
  }, this);
  gpio_intr_enable((gpio_num_t)pwm_pin_);

  // Initialize oneshot ADC (GPIO0 = ADC1_CHANNEL_0)
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

  // Create one-shot sampling timer
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

  // Start heartbeat timer immediately (periodic)
  esp_timer_start_periodic(heartbeat_timer_, 2000);  // 2000 µs = 500 Hz

  ESP_LOGI(TAG, "EVSE CP Sampler ready on pin %d (samples: %d, heartbeat: 500 Hz)", pwm_pin_, samples_);
}

void CpSampler::start_sample_timer() {
  // Restart heartbeat timer (resets its period)
  esp_timer_restart(heartbeat_timer_, 2000);

  // Start one-shot sampling timer
  esp_timer_start_once(sample_timer_, 50);  // 50 µs
}

void IRAM_ATTR CpSampler::timer_callback(void *arg) {
  auto self = static_cast<CpSampler *>(arg);

  int raw_adc = 0;
  esp_err_t ret = adc_oneshot_read(self->adc_handle_, ADC_CHANNEL_0, &raw_adc);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "ADC read failed");
    return;
  }

  uint16_t raw = static_cast<uint16_t>(raw_adc);  // 0–4095
  
  self->sum_raw_values_ += raw;

  if (++self->counter_ < self->samples_) {
    return;
  }

  // Calculate average raw value
  int avg_raw = static_cast<int>(self->sum_raw_values_) / self->samples_;

  // Reset accumulators
  self->sum_raw_values_ = 0;
  self->counter_ = 0;

  // Trigger on_raw_value with average raw
  if (self->raw_value_trigger_) {
    self->raw_value_trigger_->trigger(avg_raw);
  }
  
  // State detection
  int new_state = 0;
  if (avg_raw > 4000) new_state = 1;
  else if (abs(avg_raw - 3650) < 100) new_state = 2;
  else if (abs(avg_raw - 3200) < 100) new_state = 3;

  if (new_state != self->old_state_) {
    // Trigger on_state_change
    if (self->state_change_trigger_) {
      self->state_change_trigger_->trigger(new_state);
    }
    self->old_state_ = new_state;
  }
}

void CpSampler::dump_config() {
  ESP_LOGCONFIG(TAG, "EVSE CP Sampler:");
  ESP_LOGCONFIG(TAG, "  PWM Pin: %d", pwm_pin_);
  ESP_LOGCONFIG(TAG, "  Samples threshold: %d", samples_);
}

}  // namespace evse_cp_sampler
}  // namespace esphome
