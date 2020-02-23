import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, CONF_NAME, CONF_TYPE

homey_model_ns = cg.esphome_ns.namespace('homey_model')
DeviceProperty = homey_model_ns.class_('DeviceProperty')
HomeyDevice = homey_model_ns.class_('HomeyDevice')
HomeyDevicePtr = HomeyDevice.operator('ptr')
HomeyModel = homey_model_ns.class_('HomeyModel', cg.Component)

CONF_DEVICES = 'devices'
CONF_CLASS = 'class'
CONF_PROPERTIES = 'properties'
CONF_REF = 'ref'

DEVICE_PROPERTIES_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(DeviceProperty),
    cv.Required(CONF_TYPE): cv.string_strict,
    cv.Required(CONF_NAME): cv.string_strict,
    cv.Required(CONF_REF): cv.use_id(sensor.SensorPtr)
})

HOMEY_DEVICE_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(HomeyDevice),
    cv.Required(CONF_NAME): cv.string_strict,
    cv.Required(CONF_CLASS): cv.string_strict,
    cv.Required(CONF_PROPERTIES): cv.Schema([DEVICE_PROPERTIES_SCHEMA])
})

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(HomeyModel),
    cv.Required(CONF_DEVICES): cv.Schema([HOMEY_DEVICE_SCHEMA])
}).extend(cv.COMPONENT_SCHEMA)


def to_code(config):

    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)

    for devConf in config[CONF_DEVICES]:
        dev = cg.new_Pvariable(devConf[CONF_ID], devConf[CONF_NAME], devConf[CONF_CLASS])
        cg.add(var.register_device(dev))
        for propConf in devConf[CONF_PROPERTIES]:
            component = yield cg.get_variable(propConf[CONF_REF])
            prop = cg.new_Pvariable(propConf[CONF_ID], propConf[CONF_TYPE], propConf[CONF_NAME], component)
            cg.add(dev.register_property(prop))
