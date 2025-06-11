#pragma once

#include "../../Debug.h"
#include <WebServer.h>         // Use the standard synchronous WebServer
#include <WebSocketsServer.h>  // Use the synchronous WebSocket Server library
#include <pgmspace.h>
#include <cstdio>
#include <cstdlib>

// Forward-declare to avoid pulling in the full controller header
class SystemController;

class WebInterface {
public:
  // Constructor now takes the synchronous WebServer
  WebInterface(SystemController& controller, WebServer& server);

  void begin();
  void loop(); // Critical method to process WebSocket events
  void broadcast_led_state(const char* field);

private:
  SystemController&  controller_;
  WebServer&         server_;
  WebSocketsServer   ws_{81}; // WebSocket server on port 81

  static constexpr size_t kBufSize = 64;
  char                    payload_[kBufSize];
  size_t                  payload_len_ = 0;

  // --- WebSocket Event Handler ---
  void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);

  // --- HTTP Route Handlers ---
  void serve_main_page();
  void handle_set();
  void handle_get_state();
  void handle_set_state();

  // --- Helper ---
  void update_state_payload(const char* field);
};
