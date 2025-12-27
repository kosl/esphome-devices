import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, pins
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_PIN,
)

DEPENDENCIES = ["sensor"] # ["gpio", "sensor"]

evse_cp_sampler_ns = cg.esphome_ns.namespace("evse_cp_sampler")
CpSampler = evse_cp_sampler_ns.class_("CpSampler", cg.Component)

# Automation trigger class
StateChangeTrigger = evse_cp_sampler_ns.class_(
    "StateChangeTrigger", automation.Trigger.template(cg.int_)
)
CONF_TRIGGER_ID = "trigger_id"

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(CpSampler),
            cv.Required(CONF_PIN): pins.gpio_output_pin_schema,
            cv.Required("adc_sensor"): cv.use_id(sensor.Sensor),
            cv.Optional("samples", default=10): cv.positive_int,
            cv.Optional("on_state_change"): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(StateChangeTrigger),
                }
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)
