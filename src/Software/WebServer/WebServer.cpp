// src/Software/WebServer/WebServer.cpp

#include "WebServer.h"
#include "../../SystemController/SystemController.h"  // brings in full SystemController

WebServer::WebServer(SystemController& controller, AsyncWebServer& server)
  : controller_(controller)
  , server_(server)
{}

void WebServer::begin() {
  DBG_PRINTLN(WebServer, "begin() - registering routes");

  // HTTP routes
  server_.on("/", HTTP_GET, [this](AsyncWebServerRequest* req){
    serve_main_page(req);
  });
  server_.on("/set", HTTP_GET, [this](AsyncWebServerRequest* req){
    handle_set(req);
  });
  server_.on("/state", HTTP_GET, [this](AsyncWebServerRequest* req){
    handle_get_state(req);
  });

  // WebSocket endpoint
  server_.addHandler(&ws_);
  ws_.onEvent([this](AsyncWebSocket* /*ws*/, AsyncWebSocketClient* client,
                     AwsEventType type, void* /*arg*/, uint8_t* /*data*/, size_t /*len*/){
    if (type == WS_EVT_CONNECT) {
      // new client—push current state immediately
      broadcast_led_state();
    }
  });

  server_.begin();
  DBG_PRINTLN(WebServer, "begin() - server started");
}

void WebServer::serve_main_page(AsyncWebServerRequest* request) {
  const char* html = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8"><title>LED Control</title>
    <style>
      button:disabled { opacity: 0.5; cursor: not-allowed; }
      .control { margin-bottom: 8px; }
    </style>
  </head>
  <body>
    <h1>LED Strip Control</h1>

    <div class="control">
      <label>Color: <input type="color" id="color"/></label>
    </div>
    <div class="control">
      <label>Brightness: <input type="range" id="brightness" min="0" max="255"/></label>
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
      // open WebSocket for real-time updates
      const ws = new WebSocket(`ws://${location.host}/ws`);
      ws.onmessage = evt => {
        const js = JSON.parse(evt.data);
        document.getElementById('color').value      = js.color;
        document.getElementById('brightness').value = js.brightness;
        updateButtons(js.state === 1);
      };

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
        // initial REST load
        fetch('/state').then(r=>r.json()).then(js=>{
          document.getElementById('color').value      = js.color;
          document.getElementById('brightness').value = js.brightness;
          updateButtons(js.state===1);
        });
        // auto-send on input
        document.getElementById('color')     .addEventListener('input', sendSettings);
        document.getElementById('brightness').addEventListener('input', sendSettings);
        document.getElementById('mode')      .addEventListener('change', sendSettings);
        // On/Off
        document.getElementById('btnOn') .addEventListener('click', ()=>sendState('1'));
        document.getElementById('btnOff').addEventListener('click', ()=>sendState('0'));
      });
    </script>
  </body>
</html>
)rawliteral";

  request->send(200, "text/html", html);
}

void WebServer::handle_set(AsyncWebServerRequest* request) {
  DBG_PRINTLN(WebServer, "handle_set");

  // color
  if (auto* p = request->getParam("color")) {
    char buf[8]; p->value().toCharArray(buf, sizeof(buf));
    long v = strtol(buf+1, nullptr, 16);
    char cmd[16]; snprintf(cmd, sizeof(cmd), "%u %u %u",
      (unsigned)((v>>16)&0xFF), (unsigned)((v>>8)&0xFF), (unsigned)(v&0xFF));
    controller_.led_strip_set_rgb(cmd);
  }
  // brightness
  if (auto* p = request->getParam("brightness")) {
    char buf[4]; p->value().toCharArray(buf, sizeof(buf));
    controller_.led_strip_set_brightness(buf);
  }
  // state
  if (auto* p = request->getParam("state")) {
    char buf[2]; p->value().toCharArray(buf, sizeof(buf));
    controller_.led_strip_set_state(buf);
  }
  // mode
  if (auto* p = request->getParam("mode")) {
    char buf[16]; p->value().toCharArray(buf, sizeof(buf));
    controller_.led_strip_set_mode(buf);
  }

  request->send(200, "text/plain", "ok");
}

void WebServer::handle_get_state(AsyncWebServerRequest* request) {
  DBG_PRINTLN(WebServer, "handle_get_state");

  String c = controller_.led_strip_get_color_hex();
  uint8_t b = controller_.led_strip_get_brightness();
  bool    s = controller_.led_strip_get_state();
  String m = controller_.led_strip_get_mode();

  char buf[128];
  int len = snprintf(buf, sizeof(buf),
                     "{\"color\":\"%s\",\"brightness\":%u,"
                     "\"state\":%u,\"mode\":\"%s\"}",
                     c.c_str(), (unsigned)b, (unsigned)s, m.c_str());
  if (len<0) {
    request->send(500, "text/plain", "Error");
  } else {
    request->send(200, "application/json", buf);
  }
}

void WebServer::broadcast_led_state() {
  String c = controller_.led_strip_get_color_hex();
  uint8_t b = controller_.led_strip_get_brightness();
  bool    s = controller_.led_strip_get_state();
  String m = controller_.led_strip_get_mode();

  char buf[128];
  int len = snprintf(buf, sizeof(buf),
                     "{\"color\":\"%s\",\"brightness\":%u,"
                     "\"state\":%u,\"mode\":\"%s\"}",
                     c.c_str(), (unsigned)b, (unsigned)s, m.c_str());
  if (len>0) {
    ws_.textAll(buf);
  }
}
