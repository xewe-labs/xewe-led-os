#pragma once

#include "../../Debug.h"  // for DBG_PRINT and DBG_PRINTF
#include <WebServer.h>    // Use the ESP32 core WebServer
#include <pgmspace.h>
#include <cstdio>         // snprintf
#include <cstdlib>        // strtoul

// Forward-declare to avoid pulling in full controller
class SystemController;

class WebInterface {
public:
  // Constructor takes a reference to the ESP32 core WebServer
  WebInterface(SystemController& controller, WebServer& server);

  void begin();
  void loop(); // Method to be called from the main loop

  // This method will now update the internal payload for polling
  void broadcast_led_state(const char* field);

private:
  SystemController& controller_;
  WebServer&        server_; // Reference to the ESP32 core WebServer

  static constexpr size_t kBufSize = 64;
  char                    payload_[kBufSize];
  size_t                  payload_len_ = 0;

  // Handler methods
  void serve_main_page();
  void handle_set();
  void handle_get_state();
  void handle_set_state();

  void update_state_payload(const char* field);
};
