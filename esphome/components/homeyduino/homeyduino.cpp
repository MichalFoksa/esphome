#include "homeyduino.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/util.h"
#include "esphome/components/json/json_util.h"

#include "HTTPClient.h"

#ifdef USE_LOGGER
#include <esphome/components/logger/logger.h>
#endif

namespace esphome {
namespace homeyduino {

static const char *TAG = "homeyduino";
static const char* APPLICATION_JSON = "application/json";

UrlMatch match_url(const std::string &url, bool only_domain = false) {
  UrlMatch match;
  match.valid = false;
  size_t domain_end = url.find('/', 1);
  if (domain_end == std::string::npos)
    return match;
  match.domain = url.substr(1, domain_end - 1);
  if (only_domain) {
    match.valid = true;
    return match;
  }
  if (url.length() == domain_end - 1)
    return match;
  size_t id_begin = domain_end + 1;
  size_t id_end = url.find('/', id_begin);
  match.valid = true;
  if (id_end == std::string::npos) {
    match.id = url.substr(id_begin, url.length() - id_begin);
    return match;
  }
  match.id = url.substr(id_begin, id_end - id_begin);
  size_t method_begin = id_end + 1;
  match.method = url.substr(method_begin, url.length() - method_begin);
  return match;
}

std::string bool_response(bool state) {
  return json::build_json([state](JsonObject &root) {
    root["t"] = "Boolean";
    root["r"] = state;
  });
}

std::string number_response(float state) {
  return json::build_json([state](JsonObject &root) {
    root["t"] = "Number";
    root["r"] = state;
  });
}

std::string emit_request(const char* type, float state) {
  return json::build_json([type, state](JsonObject &root) {
    root["type"] = type;
    root["argument"] = state;
  });
}

void Homeyduino::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Homeyduino web server ...");
  this->setup_controller();

  this->server_ = new AsyncWebServer(this->port_);
  this->server_->addHandler(this);
  this->server_->begin();
}

void Homeyduino::dump_config() {
  ESP_LOGCONFIG(TAG, "Homeyduino:");
  ESP_LOGCONFIG(TAG, "  Address: %s:%u", network_get_address().c_str(), port_);
  ESP_LOGCONFIG(TAG, "  Master address: %s:%u", this->master_host_.c_str(), this->master_port_);
  this->device_->dump_config();
}

float Homeyduino::get_setup_priority() const { return setup_priority::WIFI - 1.0f; }

bool Homeyduino::isRequestHandlerTrivial() { return false; }

bool Homeyduino::canHandle(AsyncWebServerRequest *request) {
  ESP_LOGI(TAG, "HTTP Request: %s %s %s", request->methodToString(),
      request->url().c_str(), request->contentType().c_str());

  if (request->url() == "/") {
    return true;
  }

  if (request->method() == HTTP_POST && request->url() == "/sys/setmaster") {
    return true;
  }

  UrlMatch match = match_url(request->url().c_str(), true);
  if (!match.valid) {
    return false;
  }

  if (request->method() == HTTP_GET && match.domain == "cap") {
    return true;
  }

  // TODO Add suport for `act` and `con` endpoins
  return false;
}

void Homeyduino::handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len,
    size_t index, size_t total) {
  ESP_LOGD(TAG, "handleBody [len=%u, index=%u, total=%u]", len, index, total);

  // If whole data is not available, do not botter making sense of it.
  if (!data || len != total) {
    return;
  }

  // `POST /sys/master`
  if (request->method() == HTTP_POST && request->url() == "/sys/setmaster") {
    if (this->set_master_(data, len)) {
      // Return 200 {"t":"Boolean","r":true}
      request->send(200, APPLICATION_JSON, bool_response(true).c_str());
    } else  {
      request->send(500);
    }
    return;
  }
}

boolean Homeyduino::set_master_(uint8_t *data, size_t len) {
  String tmp = String((char*) data);
  int i = tmp.indexOf(':');

  // TODO Guess this (String....c_str()) must be terribly wrong.
  this->master_host_ = tmp.substring(0, i).c_str();
  this->master_port_ = (uint16_t) atoi(tmp.substring(++i).c_str());

  ESP_LOGI(TAG, "Set master [host=%s, port=%u]", this->master_host_.c_str(),
      this->master_port_);
  return master_set_ = true;
}

void Homeyduino::handleRequest(AsyncWebServerRequest *request) {
  // TODO Delete
  ESP_LOGI(TAG, "Handle HTTP Request: %s %s %s", request->methodToString(),
      request->url().c_str(), request->contentType().c_str());

  if (request->url() == "/") {
    this->handle_index_request_(request);
    return;
  }

  // `POST /sys/master`
  if (request->method() == HTTP_POST && request->url() == "/sys/setmaster") {
    // Shoudl be already handled by handleBody(..) at this point.
    return;
  }

  UrlMatch match = match_url(request->url().c_str());
  if (request->method() == HTTP_GET && match.domain == "cap") {
    this->handle_capability_request_(request, match);
    return;
  }
}

void Homeyduino::handle_index_request_(AsyncWebServerRequest *request) {
  ESP_LOGD(TAG, "Handle index request");
  std::string data = this->index_json_();
  request->send(200, APPLICATION_JSON, data.c_str());
}

std::string Homeyduino::index_json_() {
  return json::build_json([this](JsonObject &root) {
    root["id"] = device_->get_name();
    root["version"] = "1.0.2";
    root["type"] = "homeyduino";
    root["class"] = device_->get_class();

    // master{}
    JsonObject& master = root.createNestedObject("master");
    master["host"] = master_host_;
    master["port"] = master_port_;

    // api[]
    JsonArray& apis = root.createNestedArray("api");
    for (homey_model::DeviceProperty *p : device_->get_properties()) {
      JsonObject& api = apis.createNestedObject();
      api["name"] = p->get_name();

      // TODO Add mapping for additional endpoints
      const char* type;
      if (strcmp (p->get_type(), "capability") == 0) {
          type = "cap";
      } else {
        type = p->get_type();
      }
      api["type"] = type;
    }
  });
}

void Homeyduino::handle_capability_request_(AsyncWebServerRequest *request, UrlMatch match) {
  homey_model::DeviceProperty *property = device_->get_property(match.id.c_str(), "capability");
  if (property == nullptr) {
      request->send(404);
      return;
  }

  float state = property->get_sensor()->get_state();
  if (isnan(state)) {
    request->send(503);
    return;
  }
  request->send(200, APPLICATION_JSON, number_response(state).c_str());
}

#ifdef USE_SENSOR
void Homeyduino::on_sensor_update(sensor::Sensor *sensor, float state) {
  if (!this->master_set_) {
    return;
  }

  for (homey_model::DeviceProperty *device_property : device_->get_properties()) {
    if (device_property->get_sensor() != sensor) {
      continue;
    }
    // TODO Delete
    std::string msg = "sensor-" + sensor->get_object_id();
    msg += " " + value_accuracy_to_string(state, sensor->get_accuracy_decimals());
    if (!sensor->get_unit_of_measurement().empty())
      msg += " " + sensor->get_unit_of_measurement();
    // TODO Delete
    ESP_LOGI(TAG, "Sensor updated %s", msg.c_str());

    char url[96];
    // TODO fix type mapping
    sprintf(url, "http://%s:%u/emit/%s/%s", this->master_host_.c_str(), this->master_port_,
        "cap" /*device_property->get_type()*/, device_property->get_name());

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", APPLICATION_JSON);
    int responseCode = http.POST(
      emit_request("cap", device_property->get_sensor()->get_state()).c_str());
    http.end();

    if (responseCode != 200) {
      ESP_LOGW(TAG, "Posting new state to Homey failed. [status=%d, url=%s]", responseCode, url);
    } else {
      ESP_LOGD(TAG, "New state posted to Homey [name=%s]", device_property->get_name());
    }

    return;
  }
}
#endif

}  // namespace homeyduino
}  // namespace esphome
