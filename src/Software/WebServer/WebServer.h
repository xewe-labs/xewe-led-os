//WebServer.h
#pragma once
#include "../../Debug.h"  // for DBG_PRINT and DBG_PRINTF
#include <ESPAsyncWebServer.h>
#include <pgmspace.h>
#include <cstdio>   // snprintf
#include <cstdlib>  // strtoul

// forward-declare to avoid pulling in full controller
class SystemController;

class WebServer {
public:
  WebServer(SystemController& controller, AsyncWebServer& server);
  void begin();
  // field: "color","brightness","state","mode","full"
  void broadcast_led_state(const char* field);

private:
  SystemController& controller_;
  AsyncWebServer&   server_;
  AsyncWebSocket    ws_{"/ws"};

  static constexpr size_t kBufSize = 64;
  char               payload_[kBufSize];
  size_t             payload_len_ = 0;

  void serve_main_page(AsyncWebServerRequest* req);
  void handle_set(AsyncWebServerRequest* req);
  void handle_get_state(AsyncWebServerRequest* req);
  void update_state_payload(const char* field);
};
