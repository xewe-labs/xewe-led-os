// src/Software/WebServer/WebServer.h

#pragma once

#include <ESPAsyncWebServer.h>   // for AsyncWebServer, AsyncWebSocket

// forward‐declare to avoid cycle; full definition in WebServer.cpp
class SystemController;

class WebServer {
public:
  WebServer(SystemController& controller, AsyncWebServer& server);
  void begin();
  void handle();
  void broadcast_led_state();

private:
  SystemController& controller_;
  AsyncWebServer&   server_;
  AsyncWebSocket    ws_{ "/ws" };

  void serve_main_page(AsyncWebServerRequest* request);
  void handle_set(AsyncWebServerRequest* request);
  void handle_get_state(AsyncWebServerRequest* request);
};
