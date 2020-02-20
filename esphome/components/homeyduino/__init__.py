import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import web_server_base
from esphome.components.web_server_base import CONF_WEB_SERVER_BASE_ID
from esphome.const import (CONF_ID)
from esphome.core import coroutine_with_priority

HOMEYDUINO_PORT = 46639
AUTO_LOAD = ['json', 'web_server_base']

homeyduino_ns = cg.esphome_ns.namespace('homeyduino')
Homeyduino = homeyduino_ns.class_('Homeyduino', cg.Component, cg.Controller)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(Homeyduino),
    cv.GenerateID(CONF_WEB_SERVER_BASE_ID): cv.use_id(web_server_base.WebServerBase),
}).extend(cv.COMPONENT_SCHEMA)


@coroutine_with_priority(40.0)
def to_code(config):
    paren = yield cg.get_variable(config[CONF_WEB_SERVER_BASE_ID])

    var = cg.new_Pvariable(config[CONF_ID], paren)
    yield cg.register_component(var, config)

    cg.add(paren.set_port(HOMEYDUINO_PORT))
