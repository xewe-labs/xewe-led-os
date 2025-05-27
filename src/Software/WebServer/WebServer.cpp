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

void WebServer::serve_main_page(AsyncWebServerRequest* request) {
    DBG_PRINTF(WebServer, "Function: serve_main_page - URL: %s\n", request->url().c_str());
    const char* html = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <title>LED Control</title>
    <style>
      button:disabled { opacity: 0.5; cursor: not-allowed; }
      .control { margin-bottom: 8px; }
    </style>
  </head>
  <body>
    <h1>LED Strip Control</h1>

    <div class="control">
      <label>Color:
        <input type="color" id="color" />
      </label>
    </div>

    <div class="control">
      <label>Brightness:
        <input type="range" id="brightness" min="0" max="255" />
      </label>
    </div>

    <div class="control">
      <label>Mode:
        <select id="mode">
          <option value="Color Solid">Color Solid</option>
        </select>
      </label>
    </div>

    <div class="control">
      <button id="btnOn">On</button>
      <button id="btnOff">Off</button>
    </div>

    <script>
      function sendSettings(extra = '') {
        const c = document.getElementById('color').value;
        const b = document.getElementById('brightness').value;
        const m = document.getElementById('mode').value;
        fetch(`/set?color=${encodeURIComponent(c)}&brightness=${b}&mode=${encodeURIComponent(m)}${extra}`);
      }

      function sendState(s) {
        sendSettings(`&state=${s}`);
        updateButtons(s === '1');
      }

      function updateButtons(isOn) {
        document.getElementById('btnOn').disabled  = isOn;
        document.getElementById('btnOff').disabled = !isOn;
      }

      window.addEventListener('load', () => {
        fetch('/state')
          .then(res => res.json())
          .then(js => {
            document.getElementById('color').value      = js.color;
            document.getElementById('brightness').value = js.brightness;
            document.getElementById('mode').value       = js.mode;
            updateButtons(js.state === 1);
          });

        document.getElementById('color')     .addEventListener('input', sendSettings);
        document.getElementById('brightness').addEventListener('input', sendSettings);
        document.getElementById('mode')      .addEventListener('change', sendSettings);

        document.getElementById('btnOn') .addEventListener('click', () => sendState('1'));
        document.getElementById('btnOff').addEventListener('click', () => sendState('0'));
      });
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
    DBG_PRINTLN(WebServer, "Function: handle_get_state - start");
    String color       = controller_.led_strip_get_color_hex();
    uint8_t brightness = controller_.led_strip_get_brightness();
    bool    state      = controller_.led_strip_get_state();
    String mode        = controller_.led_strip_get_mode();

    char buf[128];
    int len = snprintf(buf, sizeof(buf),
                       "{\"color\":\"%s\","
                       "\"brightness\":%u,"
                       "\"state\":%u,"
                       "\"mode\":\"%s\"}",
                       color.c_str(),
                       (unsigned)brightness,
                       (unsigned)state,
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
