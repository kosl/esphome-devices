#include "evse_cp_sampler.h"
#include "esphome/core/log.h"

namespace esphome {
namespace evse_cp_sampler {

static const char *const TAG = "evse_cp_sampler";

void CpSampler::setup() {
  sum_raw_ = 0;
  count_ = 0;
  prev_state_ = -1;

  // Configure PWM input pin (hardcoded GPIO10)
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/api-reference/peripherals/gpio.html
  //gpio_set_direction(GPIO_NUM_10, GPIO_MODE_INPUT);
  gpio_set_intr_type(GPIO_NUM_10, GPIO_INTR_POSEDGE);
#if 0
  gpio_intr_register(self->start_sample_timer(), 
#else
  gpio_install_isr_service(0);
  gpio_isr_handler_add(GPIO_NUM_10, [](void *arg) {
    auto self = static_cast<CpSampler *>(arg);
    self->start_sample_timer();
  }, this);
#endif
  gpio_intr_enable(GPIO_NUM_10);
  
  // Initialize oneshot ADC (GPIO0 = ADC1_CHANNEL_0) GPIO0 is SAR ADC1 CHANNEL 0
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

  // Start heartbeat (periodic) timer immediately
  esp_timer_start_periodic(heartbeat_timer_, 2000);  // 2000 µs = 500 Hz
}

void CpSampler::start_sample_timer() {
  // Restart heartbeat timer (resets its period) so that it will not trigger if under duty cycle
  esp_timer_restart(heartbeat_timer_, 2000);

  
  // Start one-shot sampling timer
  esp_timer_start_once(sample_timer_, 50);  // 50 µs
}

void IRAM_ATTR CpSampler::timer_callback(void *arg) {
  auto self = static_cast<CpSampler *>(arg);

  int raw_adc;
  if (self->count_ % DECIMATE_SAMPLES == 0) {
    esp_err_t ret = adc_oneshot_read(self->adc_handle_, ADC_CHANNEL_0, &raw_adc);

    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "ADC read failed");
      return;
    }
    self->sum_raw_ += raw_adc;
  }
  
  self->count_ += 1;
  if (self->count_ < self->samples_) {
    return;
  }


  // Calculate average raw value
  int avg_raw = DECIMATE_SAMPLES*self->sum_raw_ / self->samples_ ;
  // Reset accumulators
  self->sum_raw_ = 0;
  self->count_ = 0;

  // Trigger on_raw_value with average raw
  if (self->raw_value_trigger_) {
    self->raw_value_trigger_->trigger(avg_raw);
  }

  return;
  
  // State detection
  int new_state = 0;
  if (avg_raw > 4000) new_state = 1; // 4095
  else if (abs(avg_raw - 3650) < 100) new_state = 2; // avg 3712
  else if (abs(avg_raw - 3200) < 100) new_state = 3; // avg 3228
  else if (abs(avg_raw - 755) < 100) new_state = 4; // -12 V

  if (new_state != self->prev_state_) {
    // Trigger on_state_change
    if (self->state_change_trigger_) {
      self->state_change_trigger_->trigger(new_state);
    }
    self->prev_state_ = new_state;
  }
}

void CpSampler::dump_config() {
  ESP_LOGCONFIG(TAG, "EVSE CP Sampler:");
  ESP_LOGCONFIG(TAG, "  PWM on GPIO10, ADC GPIO0");
  ESP_LOGCONFIG(TAG, "  Samples averaged: %d", samples_);
}

}  // namespace evse_cp_sampler
}  // namespace esphome
