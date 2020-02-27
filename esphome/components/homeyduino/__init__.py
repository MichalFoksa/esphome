import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import web_server_base
from esphome.components import sensor
from esphome.components.homey_model import HomeyDevicePtr
from esphome.components.web_server_base import CONF_WEB_SERVER_BASE_ID
from esphome.const import (CONF_ID, CONF_DEVICE)
from esphome.core import coroutine_with_priority

HOMEYDUINO_PORT = 46639
AUTO_LOAD = ['json', 'web_server_base']

homeyduino_ns = cg.esphome_ns.namespace('homeyduino')
Homeyduino = homeyduino_ns.class_('Homeyduino', cg.Component, cg.Controller)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(Homeyduino),
    cv.GenerateID(CONF_WEB_SERVER_BASE_ID): cv.use_id(web_server_base.WebServerBase),
    cv.Required(CONF_DEVICE): cv.use_id(HomeyDevicePtr)
}).extend(cv.COMPONENT_SCHEMA)


@coroutine_with_priority(40.0)
def to_code(config):
    parent = yield cg.get_variable(config[CONF_WEB_SERVER_BASE_ID])

    device = yield cg.get_variable(config[CONF_DEVICE])
    var = cg.new_Pvariable(config[CONF_ID], parent, device)
    cg.add(parent.set_port(HOMEYDUINO_PORT))
    yield cg.register_component(var, config)
