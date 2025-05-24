
// WebServer.cpp
#include "WebServer.h"
#include "../../SystemController/SystemController.h"  // full definition

WebServer::WebServer(SystemController& controller, AsyncWebServer& server)
  : controller_(controller)
  , server_(server)
{}

void WebServer::begin() {
    // 1) Register HTTP routes
    server_.on("/", HTTP_GET, [this](AsyncWebServerRequest* req){
        serve_main_page(req);
    });
    server_.on("/set", HTTP_GET, [this](AsyncWebServerRequest* req){
        handle_set(req);
    });
    server_.on("/state", HTTP_GET, [this](AsyncWebServerRequest* req){
        handle_get_state(req);
    });

    // 2) Start the server
    server_.begin();
}

void WebServer::handle() {
    // AsyncWebServer is interrupt-driven; nothing to do here
}

void WebServer::serve_main_page(AsyncWebServerRequest* request) {
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
}

void WebServer::handle_set(AsyncWebServerRequest* request) {
    if (auto* p = request->getParam("color")) {
        String col = p->value();
        long   val = strtol(col.substring(1).c_str(), nullptr, 16);
        uint8_t r  = (val >> 16) & 0xFF;
        uint8_t g  = (val >>  8) & 0xFF;
        uint8_t b  =  val        & 0xFF;
        controller_.led_strip_set_rgb(
            String(r) + " " + String(g) + " " + String(b)
        );
    }
    if (auto* p = request->getParam("brightness")) {
        controller_.led_strip_set_brightness(p->value());
    }
    if (auto* p = request->getParam("state")) {
        controller_.led_strip_set_state(p->value());
    }
    if (auto* p = request->getParam("mode")) {
        controller_.led_strip_set_mode(p->value());
    }
    request->send(200, "text/plain", "ok");
}

void WebServer::handle_get_state(AsyncWebServerRequest* request) {
    DynamicJsonDocument doc(256);
    doc["color"]      = controller_.led_strip_get_color_hex();
    doc["brightness"] = controller_.led_strip_get_brightness();
    doc["state"]      = controller_.led_strip_get_state();
    doc["mode"]       = controller_.led_strip_get_mode();
    String body;
    serializeJson(doc, body);
    request->send(200, "application/json", body);
}