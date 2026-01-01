#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"

namespace esphome {
namespace evse_cp_sampler {

class CpSampler;

class StateChangeTrigger : public Trigger<int> {
 public:
  explicit StateChangeTrigger(CpSampler *parent);
};

class RawValueTrigger : public Trigger<int> {
 public:
  explicit RawValueTrigger(CpSampler *parent);
};

class CpSampler : public Component {
 public:
  void setup() override;
  void dump_config() override;

  void set_samples(int s) { samples_ = s; }
  void set_state_change_trigger(StateChangeTrigger *t) { state_change_trigger_ = t; }
  void set_raw_value_trigger(RawValueTrigger *t) { raw_value_trigger_ = t; }

 protected:
  // Hardcoded PWM input pin
  static constexpr gpio_num_t PWM_PIN = GPIO_NUM_10;
  // Adjust this to match your actual CP ADC channel
  static constexpr adc_channel_t CP_ADC_CHANNEL = ADC_CHANNEL_0;
  // Decimate and use sparse ADC samples
  static constexpr int DECIMATE_SAMPLES = 10;

  int samples_;
  int sum_raw_;
  int count_;
  int prev_state_;

  StateChangeTrigger *state_change_trigger_{nullptr};
  RawValueTrigger *raw_value_trigger_{nullptr};

  adc_oneshot_unit_handle_t adc_handle_{nullptr};
  esp_timer_handle_t sample_timer_{nullptr};     // one-shot, 50 us after rising edge
  esp_timer_handle_t heartbeat_timer_{nullptr};  // periodic heartbeat timer

  static void IRAM_ATTR timer_callback(void *arg);
  void start_sample_timer();
};

inline StateChangeTrigger::StateChangeTrigger(CpSampler *parent) {
  if (parent != nullptr)
    parent->set_state_change_trigger(this);
}

inline RawValueTrigger::RawValueTrigger(CpSampler *parent) {
  if (parent != nullptr)
    parent->set_raw_value_trigger(this);
}

}  // namespace evse_cp_sampler
}  // namespace esphome

