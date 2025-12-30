import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, pins
from esphome.const import CONF_ID, CONF_PIN

DEPENDENCIES = []

evse_cp_sampler_ns = cg.esphome_ns.namespace("evse_cp_sampler")
CpSampler = evse_cp_sampler_ns.class_("CpSampler", cg.Component)

StateChangeTrigger = evse_cp_sampler_ns.class_(
    "StateChangeTrigger", automation.Trigger.template(cg.int_)
)
RawValueTrigger = evse_cp_sampler_ns.class_(
    "RawValueTrigger", automation.Trigger.template(cg.int_)
)

CONF_SAMPLES = "samples"
CONF_ON_STATE_CHANGE = "on_state_change"
CONF_ON_RAW_VALUE = "on_raw_value"
CONF_TRIGGER_ID = "trigger_id"

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(CpSampler),

            # CP pin – treated as a GPIO pin object (GPIOPin*)
            cv.Required(CONF_PIN): pins.gpio_input_pin_schema,

            cv.Optional(CONF_SAMPLES, default=100): cv.positive_int,

            cv.Optional(CONF_ON_STATE_CHANGE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(StateChangeTrigger),
                }
            ),

            cv.Optional(CONF_ON_RAW_VALUE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(RawValueTrigger),
                }
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    # Create C++ object
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Configure pin as GPIOPin* (matches C++ set_pwm_pin(GPIOPin*))
    pin = await cg.gpio_pin_expression(config[CONF_PIN])
    cg.add(var.set_pwm_pin(pin))

    # Configure sample count
    cg.add(var.set_samples(config[CONF_SAMPLES]))

    # on_state_change triggers
    for conf in config.get(CONF_ON_STATE_CHANGE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.int_, "x")], conf)

    # on_raw_value triggers
    for conf in config.get(CONF_ON_RAW_VALUE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.int_, "x")], conf)


__all__ = [
    "CpSampler",
    "StateChangeTrigger",
    "RawValueTrigger",
]

