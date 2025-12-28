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

  ESP_LOGI(TAG, "EVSE CP Sampler ready on pin %d (threshold: %d samples)", pwm_pin_, samples_);
}

void CpSampler::start_sample_timer() {
  // Restart heartbeat timer (resets its period)
  esp_timer_restart(heartbeat_timer_, 2000);

  // Start one-shot sampling timer
  esp_timer_start_once(sample_timer_, 50);  // 50 µs
}

void IRAM_ATTR CpSampler::timer_callback(void *arg) {
  auto self = static_cast<CpSampler *>(arg);
  if (!self->adc_sensor_) return;

  self->adc_sensor_->update(); // TODO use adc_oneshot_read() directly
  uint16_t raw = self->adc_sensor_->get_raw_state();
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
