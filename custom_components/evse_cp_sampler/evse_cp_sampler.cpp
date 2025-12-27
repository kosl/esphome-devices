#include "evse_cp_sampler.h"
#include "esphome/core/log.h"
#include "driver/gpio.h"       // for gpio_*
#include "esp_timer.h"         // for esp_timer_*

namespace esphome {
namespace evse_cp_sampler {

static const char *const TAG = "evse_cp_sampler";

void CpSampler::setup() {
  // Set pin as input-readable (even if used as LEDC output)
  gpio_set_direction((gpio_num_t)pwm_pin_, GPIO_MODE_INPUT);

  // Register rising edge interrupt
  gpio_set_intr_type((gpio_num_t)pwm_pin_, GPIO_INTR_POSEDGE);
  gpio_install_isr_service(0);
  gpio_isr_handler_add((gpio_num_t)pwm_pin_, [](void *arg) {
    auto self = static_cast<CpSampler *>(arg);
    self->start_sample_timer();
  }, this);
  gpio_intr_enable((gpio_num_t)pwm_pin_);

  // Create one-shot timer
  esp_timer_create_args_t timer_args = {
      .callback = timer_callback,
      .arg = this,
      .dispatch_method = ESP_TIMER_TASK,  // Correct value
      .name = "cp_sample",
      .skip_unhandled_events = true,
  };
  esp_timer_create(&timer_args, &sample_timer_);

  ESP_LOGI(TAG, "EVSE CP Sampler ready on pin %d (threshold: %d samples)", pwm_pin_, samples_);
}

void CpSampler::start_sample_timer() {
  esp_timer_start_once(sample_timer_, 50);  // 50 µs
}

void IRAM_ATTR CpSampler::timer_callback(void *arg) {
  auto self = static_cast<CpSampler *>(arg);
  if (!self->adc_sensor_) return;

  float raw = self->adc_sensor_->get_raw_state();

  int new_state = 0;
  if (raw > 4000) new_state = 1;
  else if (fabsf(raw - 3650.0f) < 100) new_state = 2;
  else if (fabsf(raw - 3200.0f) < 100) new_state = 3;

  if (self->state_change_trigger_) {
    self->state_change_trigger_->trigger(new_state);
  }
}

void CpSampler::dump_config() {
  ESP_LOGCONFIG(TAG, "EVSE CP Sampler:");
  ESP_LOGCONFIG(TAG, "  PWM Pin: %d", pwm_pin_);
  ESP_LOGCONFIG(TAG, "  Samples threshold: %d", samples_);
}

}  // namespace evse_cp_sampler
}  // namespace esphome
