// WebServer.h
#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "../../Debug.h"

// forward declaration to avoid circular include
class SystemController;

class WebServer {
public:
    WebServer(SystemController& controller, AsyncWebServer& server);
    void begin();           // register routes and start server
    void handle();          // no-op for AsyncWebServer

private:
    SystemController& controller_;
    AsyncWebServer&   server_;

    void serve_main_page(AsyncWebServerRequest* request);
    void handle_set(AsyncWebServerRequest* request);
    void handle_get_state(AsyncWebServerRequest* request);
};

#endif // WEB_SERVER_H
