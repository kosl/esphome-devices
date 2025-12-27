import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import sensor, binary_sensor
from esphome.const import (
    CONF_ID,
    CONF_PIN,
)

DEPENDENCIES = [] # ["gpio", "sensor"]

evse_cp_sampler_ns = cg.esphome_ns.namespace("evse_cp_sampler")
CpSampler = evse_cp_sampler_ns.class_("CpSampler", cg.Component)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(CpSampler),
            cv.Required(CONF_PIN): pins.gpio_output_pin_schema,
            cv.Required("adc_sensor"): cv.use_id(sensor.Sensor),
            cv.Optional("samples", default=10): cv.positive_int,
            cv.Optional("on_state_change"): cv.lambda_,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)
