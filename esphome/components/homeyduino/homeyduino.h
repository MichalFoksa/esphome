#pragma once

#include "esphome/core/component.h"
#include "esphome/core/controller.h"
#include "esphome/components/homey_model/homey_model.h"
#include "esphome/components/json/json_util.h"

#include <ESPAsyncWebServer.h>
#include <vector>

namespace esphome {
namespace homeyduino {

static const uint16_t HOMEYDUINO_HTTP_PORT = 46639;

/// Internal helper struct that is used to parse incoming URLs
struct UrlMatch {
  std::string domain;  ///< The domain of the component, for example "sensor"
  std::string id;      ///< The id of the device that's being accessed, for example "living_room_fan"
  std::string method;  ///< The method that's being called, for example "turn_on"
  bool valid;          ///< Whether this match is valid
};

// TODO: Fix description
/**
 * XXXXXXX
 *
 * A full documentation for this API can be found under
 * - https://github.com/athombv/com.athom.homeyduino/blob/master/docs/technical_details.md
 * - https://github.com/athombv/homey-arduino-library/blob/master/docs/protocol.md
 */
class Homeyduino : public Controller, public Component, public AsyncWebHandler {
 public:
  Homeyduino(homey_model::HomeyDevice *device) {
    device_ = device;
  }

  void set_port(uint16_t port) { port_ = port; }
  uint16_t get_port() const { return port_; }

  // ========== INTERNAL METHODS ==========
  void dump_config() override;

  // Setup web server and register handlers.
  void setup() override;

  // MQTT setup priority.
  float get_setup_priority() const override;

  // This web handle is not trivial.
  bool isRequestHandlerTrivial() override;
  // Override the web handler's canHandle method.
  bool canHandle(AsyncWebServerRequest *request) override;
  // Parse body
  void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len,
    size_t index, size_t total) override;
  // Override the web handler's handleRequest method.
  void handleRequest(AsyncWebServerRequest *request) override;

 protected:
  // Set maste. Action upon 'POST /sys/setmaster' request.
  boolean set_master_(uint8_t *data, size_t len);

  // Handle get index `GET /`.
  void handle_index_request_(AsyncWebServerRequest *request);

  // Create index response body
  std::string index_json_();

  // `GET /cap/{id}` request handler
  void handle_capability_request_(AsyncWebServerRequest *request, UrlMatch match);

#ifdef USE_SENSOR
  void on_sensor_update(sensor::Sensor *sensor, float state) override;
#endif

  // Properties
  uint16_t port_{HOMEYDUINO_HTTP_PORT};
  AsyncWebServer *server_{nullptr};
  boolean master_set_ = false;
  std::string master_host_{"0.0.0.0"};
  uint16_t master_port_ = 9999;
  homey_model::HomeyDevice *device_{nullptr};
};

}  // namespace homeyduino
}  // namespace esphome
