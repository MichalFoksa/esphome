#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

#include <vector>
#include <initializer_list>

namespace esphome {
namespace homey_model {

class DeviceProperty {
 public:
  DeviceProperty(const char *type, const char *name, sensor::Sensor *sensor) {
    type_ = type;
    name_ = name;
    sensor_ = sensor;
  }

  const char *get_type() { return type_; }
  const char *get_name() { return name_; }
  sensor::Sensor *get_sensor() { return sensor_; }
  void dump_config();

 protected:
  const char *type_{nullptr};
  const char *name_{nullptr};
  sensor::Sensor *sensor_{nullptr};
};

class HomeyDevice {
 public:
  HomeyDevice(const char *name, const char *sensor_class) {
    name_ = name;
    class_ = sensor_class;
  }

  const char *get_name() { return name_; }
  const char *get_class() { return class_; }
  std::vector<DeviceProperty *> get_properties() { return properties_; }
  // Get property by name and type
  DeviceProperty* get_property(const char *name, const char *type);
  void dump_config();
  void register_property(DeviceProperty* property);

 protected:
  const char *name_{nullptr};
  const char *class_{nullptr};
  std::vector<DeviceProperty *> properties_{};
};

class HomeyModel : public Component {
 public:
  HomeyModel() {}

  void dump_config() override;
  void register_device(HomeyDevice* devices);

 protected:
  std::vector<HomeyDevice *> devices_{};
};

}  // namespace homey_model
}  // namespace esphome
