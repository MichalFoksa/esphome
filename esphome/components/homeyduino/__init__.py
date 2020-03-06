import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.components.homey_model import HomeyDevicePtr
from esphome.const import (CONF_ID, CONF_DEVICE)
from esphome.core import coroutine_with_priority

HOMEYDUINO_HTTP_PORT = 46639
AUTO_LOAD = ['json']

homeyduino_ns = cg.esphome_ns.namespace('homeyduino')
Homeyduino = homeyduino_ns.class_('Homeyduino', cg.Component, cg.Controller)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(Homeyduino),
    cv.Required(CONF_DEVICE): cv.use_id(HomeyDevicePtr)
}).extend(cv.COMPONENT_SCHEMA)


@coroutine_with_priority(40.0)
def to_code(config):
    device = yield cg.get_variable(config[CONF_DEVICE])
    var = cg.new_Pvariable(config[CONF_ID], device)
    cg.add(var.set_port(HOMEYDUINO_HTTP_PORT))
    yield cg.register_component(var, config)
