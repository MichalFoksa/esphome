#include "homey_model.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/util.h"

#include <cstdlib>

#ifdef USE_LOGGER
#include <esphome/components/logger/logger.h>
#endif

namespace esphome {
namespace homey_model {

static const char *TAG = "homey_model";

void DeviceProperty::dump_config() {
  ESP_LOGCONFIG(TAG, "    - Property [type: '%s', name: '%s', sensor: '%s', unit: '%s']",
      this->type_, this->name_, this->sensor_->get_name().c_str(),
      this->sensor_->get_unit_of_measurement().c_str());
}

DeviceProperty* HomeyDevice::get_property(const char *name, const char *type) {
  for (DeviceProperty *p : this->properties_) {
    if (strcmp(p->get_name(), name) == 0 &&
        strcmp(p->get_type(), type) == 0) {
      return p;
    }
  }
  return nullptr;
}

void HomeyDevice::dump_config() {
  ESP_LOGCONFIG(TAG, "  - Device [name: '%s', class: '%s']", this->name_, this->class_);
  for (DeviceProperty *p : this->properties_) {
    p->dump_config();
  }
}

void HomeyDevice::register_property(DeviceProperty* property) {
  if (property == nullptr) {
    ESP_LOGW(TAG, "Tried to register null property!");
    return;
  }

  for (DeviceProperty *p : this->properties_) {
    if (property == p) {
      ESP_LOGW(TAG, "Property already registered! (%p)", p);
      return;
    }
  }
  this->properties_.push_back(property);
}

void HomeyModel::dump_config() {
  ESP_LOGCONFIG(TAG, "Homey model:");
  for (HomeyDevice *d : this->devices_) {
    d->dump_config();
  }
}

void HomeyModel::register_device(HomeyDevice* device) {
  if (device == nullptr) {
    ESP_LOGW(TAG, "Tried to register null device!");
    return;
  }

  for (HomeyDevice *d : this->devices_) {
    if (device == d) {
      ESP_LOGW(TAG, "Device already registered! (%p)", d);
      return;
    }
  }
  this->devices_.push_back(device);
}

}  // namespace homey_model
}  // namespace esphome
