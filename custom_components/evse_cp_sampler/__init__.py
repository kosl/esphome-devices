import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import CONF_ID

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
            cv.Optional(CONF_SAMPLES, default=250): cv.positive_int,
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
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_samples(config[CONF_SAMPLES]))

    for conf in config.get(CONF_ON_STATE_CHANGE, []):
        trig = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trig, [(cg.int_, "x")], conf)

    for conf in config.get(CONF_ON_RAW_VALUE, []):
        trig = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trig, [(cg.int_, "x")], conf)

__all__ = [
    "CpSampler",
    "StateChangeTrigger",
    "RawValueTrigger",
]
