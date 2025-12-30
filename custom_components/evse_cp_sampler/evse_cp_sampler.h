#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/hal.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"

namespace esphome {
namespace evse_cp_sampler {

class CpSampler;

// Forward-declare triggers
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

  // Called from Python: set_pwm_pin(pin)
  void set_pwm_pin(GPIOPin *pin) { pwm_pin_ = pin; }
  void set_samples(int samples) { samples_ = samples; }

  void set_state_change_trigger(StateChangeTrigger *t) { state_change_trigger_ = t; }
  void set_raw_value_trigger(RawValueTrigger *t) { raw_value_trigger_ = t; }

  // You can add public helpers here if you had them before.

 protected:
  // NOTE: now a GPIOPin* (fixes invalid conversion error)
  GPIOPin *pwm_pin_{nullptr};
  int samples_{0};

  int sum_raw_values_{0};
  int counter_{0};
  int old_state_{0};

  StateChangeTrigger *state_change_trigger_{nullptr};
  RawValueTrigger *raw_value_trigger_{nullptr};

  esp_timer_handle_t sample_timer_{nullptr};
  esp_timer_handle_t heartbeat_timer_{nullptr};

  adc_oneshot_unit_handle_t adc_handle_{nullptr};

  static void IRAM_ATTR timer_callback(void *arg);
  void start_sample_timer();
};

// Inline constructors for triggers so they hook themselves into CpSampler
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
