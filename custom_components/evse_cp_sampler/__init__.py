import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, pins
from esphome.components import sensor
from esphome.const import CONF_ID, CONF_PIN

DEPENDENCIES = ["sensor"]

evse_cp_sampler_ns = cg.esphome_ns.namespace("evse_cp_sampler")
CpSampler = evse_cp_sampler_ns.class_("CpSampler", cg.Component)

# Trigger for state change (int)
StateChangeTrigger = evse_cp_sampler_ns.class_(
    "StateChangeTrigger", automation.Trigger.template(cg.int_)
)

# Trigger for averaged raw value (int)
RawValueTrigger = evse_cp_sampler_ns.class_(
    "RawValueTrigger", automation.Trigger.template(cg.int_)
)

CONF_TRIGGER_ID = "trigger_id"

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(CpSampler),
            cv.Required(CONF_PIN): pins.gpio_output_pin_schema,
            cv.Required("adc_sensor"): cv.use_id(sensor.Sensor),
            cv.Optional("samples", default=100): cv.positive_int,
            cv.Optional("on_state_change"): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(StateChangeTrigger),
                }
            ),
            cv.Optional("on_raw_value"): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(RawValueTrigger),
                }
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

async def disabled_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    pin = await cg.gpio_pin_expression(config[CONF_PIN])
    cg.add(var.set_pwm_pin(pin))

    adc = await cg.get_variable(config["adc_sensor"])
    cg.add(var.set_adc_sensor(adc))

    cg.add(var.set_samples(config["samples"]))

    # on_state_change
    if "on_state_change" in config:
        for conf in config["on_state_change"]:
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            await automation.build_automation(trigger, [(cg.int_, "x")], conf)

    # on_raw_value
    if "on_raw_value" in config:
        for conf in config["on_raw_value"]:
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            await automation.build_automation(trigger, [(cg.float_, "x")], conf)
