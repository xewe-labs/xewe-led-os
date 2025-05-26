

// WebServer.cpp
#include "WebServer.h"
#include "../../SystemController/SystemController.h"  // full definition

WebServer::WebServer(SystemController& controller, AsyncWebServer& server)
  : controller_(controller)
  , server_(server)
{}

void WebServer::begin() {
    DBG_PRINTLN(WebServer, "Function: begin() - registering routes");
    // 1) Register HTTP routes
    server_.on("/", HTTP_GET, [this](AsyncWebServerRequest* req){
        DBG_PRINTLN(WebServer, "Route '/' hit");
        serve_main_page(req);
    });
    server_.on("/set", HTTP_GET, [this](AsyncWebServerRequest* req){
        DBG_PRINTLN(WebServer, "Route '/set' hit");
        handle_set(req);
    });
    server_.on("/state", HTTP_GET, [this](AsyncWebServerRequest* req){
        DBG_PRINTLN(WebServer, "Route '/state' hit");
        handle_get_state(req);
    });

    // 2) Start the server
    server_.begin();
    DBG_PRINTLN(WebServer, "Function: begin() - server started");
}

void WebServer::handle() {
    // AsyncWebServer is interrupt-driven; nothing to do here
}

void WebServer::serve_main_page(AsyncWebServerRequest* request) {
    DBG_PRINTF(WebServer, "Function: serve_main_page - URL: %s\n", request->url().c_str());
    const char* html = R"rawliteral(
<!DOCTYPE html>
<html>
  <head><meta charset="utf-8"><title>LED Control</title></head>
  <body>
    <h1>LED Strip Control</h1>
    <label>Color: <input type="color" id="color" /></label><br/>
    <label>Brightness: <input type="range" id="brightness" min="0" max="255"/></label><br/>
    <label>On/Off: <input type="checkbox" id="state"/></label><br/>
    <label>Mode:
      <select id="mode">
        <option value="solid">Solid</option>
        <option value="rainbow">Rainbow</option>
        <option value="blink">Blink</option>
      </select>
    </label><br/>
    <button onclick="send_settings()">Apply</button>
    <script>
      window.onload = () => {
        fetch('/state')
          .then(res => res.json())
          .then(js => {
            document.getElementById('color').value      = js.color;
            document.getElementById('brightness').value = js.brightness;
            document.getElementById('state').checked    = js.state === 1;
            document.getElementById('mode').value       = js.mode;
          });
      };
      function send_settings() {
        const c = document.getElementById('color').value;
        const b = document.getElementById('brightness').value;
        const s = document.getElementById('state').checked ? '1' : '0';
        const m = document.getElementById('mode').value;
        fetch(`/set?color=${encodeURIComponent(c)}&brightness=${b}&state=${s}&mode=${encodeURIComponent(m)}`)
          .then(() => location.reload());
      }
    </script>
  </body>
</html>
)rawliteral";
    request->send(200, "text/html", html);
    DBG_PRINTLN(WebServer, "serve_main_page: response sent");
}

void WebServer::handle_set(AsyncWebServerRequest* request) {
    DBG_PRINTLN(WebServer, "Function: handle_set - start");
    // Extract and process query parameters using fixed buffers
    char colBuf[8] = {0};
    if (auto* p = request->getParam("color")) {
        p->value().toCharArray(colBuf, sizeof(colBuf));
        DBG_PRINTF(WebServer, "handle_set: color param = %s\n", colBuf);
        long val = strtol(colBuf + 1, nullptr, 16);
        uint8_t r = (val >> 16) & 0xFF;
        uint8_t g = (val >>  8) & 0xFF;
        uint8_t b =  val        & 0xFF;
        char rgbCmd[16];
        snprintf(rgbCmd, sizeof(rgbCmd), "%u %u %u", r, g, b);
        controller_.led_strip_set_rgb(rgbCmd);
    }

    char briBuf[4] = {0};
    if (auto* p = request->getParam("brightness")) {
        p->value().toCharArray(briBuf, sizeof(briBuf));
        DBG_PRINTF(WebServer, "handle_set: brightness param = %s\n", briBuf);
        controller_.led_strip_set_brightness(briBuf);
    }

    char stateBuf[2] = {0};
    if (auto* p = request->getParam("state")) {
        p->value().toCharArray(stateBuf, sizeof(stateBuf));
        DBG_PRINTF(WebServer, "handle_set: state param = %s\n", stateBuf);
        controller_.led_strip_set_state(stateBuf);
    }

    char modeBuf[16] = {0};
    if (auto* p = request->getParam("mode")) {
        p->value().toCharArray(modeBuf, sizeof(modeBuf));
        DBG_PRINTF(WebServer, "handle_set: mode param = %s\n", modeBuf);
        controller_.led_strip_set_mode(modeBuf);
    }

    request->send(200, "text/plain", "ok");
    DBG_PRINTLN(WebServer, "handle_set: response sent");
}

void WebServer::handle_get_state(AsyncWebServerRequest* request) {
    // handle_get_state implementation remains as rewritten earlier
    DBG_PRINTLN(WebServer, "Function: handle_get_state - start");
    String color     = controller_.led_strip_get_color_hex();
    DBG_PRINTLN(WebServer, "String color     = controller_.led_strip_get_color_hex();");
    uint8_t brightness = controller_.led_strip_get_brightness();
    DBG_PRINTLN(WebServer, "uint8_t brightness = controller_.led_strip_get_brightness();");
    bool    st        = controller_.led_strip_get_state();
    DBG_PRINTLN(WebServer, "bool    st        = controller_.led_strip_get_state();");
    uint8_t state_val = st ? 1 : 0;
    String mode       = controller_.led_strip_get_mode();
    DBG_PRINTLN(WebServer, "String mode       = controller_.led_strip_get_mode();");
    char buf[128];
    DBG_PRINTLN(WebServer, "char buf[128];");
    int len = snprintf(buf, sizeof(buf),
                       "{\"color\":\"%3s\","
                       "\"brightness\":%u,"
                       "\"state\":%u,"
                       "\"mode\":\"%s\"}",
                       color.c_str(),
                       (unsigned)brightness,
                       (unsigned)state_val,
                       mode.c_str());
    if (len < 0) {
        DBG_PRINTLN(WebServer, "handle_get_state: snprintf error");
        request->send(500, "text/plain", "Internal Server Error");
        return;
    }
    DBG_PRINTF(WebServer, "handle_get_state: JSON body = %s\n", buf);
    request->send(200, "application/json", buf);
    DBG_PRINTLN(WebServer, "handle_get_state: response sent");
}
