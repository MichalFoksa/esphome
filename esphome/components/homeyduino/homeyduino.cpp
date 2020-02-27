#include "homeyduino.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/util.h"
#include "esphome/components/json/json_util.h"

#include "StreamString.h"

#include <cstdlib>

#ifdef USE_LOGGER
#include <esphome/components/logger/logger.h>
#endif

namespace esphome {
namespace homeyduino {

static const char *TAG = "homeyduino";

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

void Homeyduino::setup() {
  ESP_LOGCONFIG(TAG, "Setting up web server for Homeyduino ...");
  this->base_->init();

  this->base_->add_handler(this);
}
void Homeyduino::dump_config() {
  ESP_LOGCONFIG(TAG, "Homeyduino:");
  ESP_LOGCONFIG(TAG, "  Address: %s:%u", network_get_address().c_str(), this->base_->get_port());
  ESP_LOGCONFIG(TAG, "  Master address: %s:%u", this->master_host_.c_str(), this->master_port_);
  this->device_->dump_config();
}

float Homeyduino::get_setup_priority() const { return setup_priority::WIFI - 1.0f; }

bool Homeyduino::isRequestHandlerTrivial() { return false; }

bool Homeyduino::canHandle(AsyncWebServerRequest *request) {
  ESP_LOGI(TAG, "HTTP Request: %s %s %s", request->methodToString(),
      request->url().c_str(), request->contentType().c_str());

  if (request->url() == "/")
    return true;

  if (request->method() == HTTP_POST && request->url() == "/sys/setmaster") {
    return true;
  }

  return false;
}

// void Homeyduino::handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len,
//     size_t index, size_t total) {
//   ESP_LOGD(TAG, "HandleBody [len=%u, index=%u, total=%u]", len, index, total);
//   if (len == total &&  request->contentType() == "text/plain" ) {
//     request->_addParam(new AsyncWebParameter("body", String((char*) data), false, false, total));
//   }
// }

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
      std::string data = json::build_json([](JsonObject &root) {
        root["t"] = "Boolean";
        root["r"] = true;
      });
      request->send(200, "application/json", data.c_str());
    } else  {
      request->send(500, "application/json");
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
  return true;
}

void Homeyduino::handleRequest(AsyncWebServerRequest *request) {
  if (request->url() == "/") {
    this->handle_index_request_(request);
    return;
  }

  // `POST /sys/master`
  if (request->method() == HTTP_POST && request->url() == "/sys/setmaster") {
    // Shoudl be already handled by handleBody(..) at this point.
    return;
  }
}

void Homeyduino::handle_index_request_(AsyncWebServerRequest *request) {
  ESP_LOGD(TAG, "Handle index request");
  std::string data = this->index_json_();
  request->send(200, "application/json", data.c_str());
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

}  // namespace homeyduino
}  // namespace esphome
