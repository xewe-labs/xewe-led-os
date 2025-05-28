// WebServer.cpp
#include "WebServer.h"
#include "../../SystemController/SystemController.h"
#include <Ticker.h>  // for periodic heartbeat

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>LED Control</title>
  <style>
    :root {
      --bg: #000;
      --fg: #fff;
      --accent: #0ff;
      --green: #0f0;
      --red: #f00;
      --font: system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', sans-serif;
    }
    *, *::before, *::after {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }
    body {
      background: var(--bg);
      color: var(--fg);
      font-family: var(--font);
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 1rem;
      min-height: 100vh;
    }
    h1 {
      margin-bottom: 0.5rem;
      font-size: 1.75rem;
      font-weight: 500;
    }
    #status {
      display: flex;
      align-items: center;
      margin-bottom: 1.5rem;
    }
    #status-indicator {
      width: 12px;
      height: 12px;
      border-radius: 50%;
      background: var(--red);
      margin-right: 0.5rem;
    }
    .control {
      width: 100%;
      max-width: 400px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 1rem;
    }
    label {
      flex: 1;
      display: flex;
      justify-content: space-between;
      align-items: center;
      font-size: 1rem;
    }
    input[type="color"],
    input[type="range"],
    select {
      appearance: none;
      background: var(--bg);
      border: 1px solid var(--accent);
      border-radius: 4px;
      padding: 0.25rem;
      color: var(--fg);
      margin-left: 0.5rem;
      flex: 1;
    }
    .buttons {
      display: flex;
      width: 100%;
      max-width: 400px;
      margin-top: 1rem;
    }
    button {
      flex: 1;
      padding: 0.75rem;
      margin: 0.25rem;
      background: var(--accent);
      border: none;
      border-radius: 4px;
      color: var(--bg);
      font-size: 1rem;
      cursor: pointer;
      transition: opacity 0.2s ease;
    }
    button:disabled {
      opacity: 0.4;
      cursor: not-allowed;
    }
  </style>
</head>
<body>
  <h1>LED Strip Control</h1>
  <div id="status">
    <div id="status-indicator"></div>
    <div id="status-text">Offline</div>
  </div>
  <div class="control">
    <label>Color
      <input type="color" id="color"/>
    </label>
  </div>
  <div class="control">
    <label>Brightness
      <input type="range" id="brightness" min="1" max="255"/>
    </label>
  </div>
  <div class="control">
    <label>Mode
      <select id="mode">
        <option value="0">Color Solid</option>
        <!-- more modes here with value="1", "2", ... -->
      </select>
    </label>
  </div>
  <div class="buttons">
    <button id="btnOn">On</button>
    <button id="btnOff">Off</button>
  </div>

  <script>
    function debounce(fn, d) { let t; return (...a) => { clearTimeout(t); t = setTimeout(() => fn(...a), d); }; }
    const ws = new WebSocket(`ws://${location.host}/ws`);

    let offlineTimer;
    let reloadTimer;

    function setOnline() {
      clearTimeout(offlineTimer);
      clearTimeout(reloadTimer);
      document.getElementById('status-indicator').style.background = 'var(--green)';
      document.getElementById('status-text').textContent = 'Online';
      offlineTimer = setTimeout(setOffline, 7000);
    }
    function setOffline() {
      document.getElementById('status-indicator').style.background = 'var(--red)';
      document.getElementById('status-text').textContent = 'Offline';
      reloadTimer = setTimeout(() => location.reload(), 5000);
    }

    ws.onopen = () => {
      console.log('WebSocket opened');
      setOnline();
    };
    ws.onmessage = evt => {
      setOnline();
      const msg = evt.data;
      const tag = msg[0], data = msg.slice(1);
      switch (tag) {
        case 'C':
          document.getElementById('color').value = '#' + data;
          break;
        case 'B':
          document.getElementById('brightness').value = parseInt(data, 10);
          break;
        case 'S':
          updateButtons(data === '1');
          break;
        case 'M':
          document.getElementById('mode').value = data;
          break;
        case 'F': {
          const [hex, b, s, m] = data.split(',');
          document.getElementById('color').value      = '#' + hex;
          document.getElementById('brightness').value = parseInt(b, 10);
          updateButtons(s === '1');
          document.getElementById('mode').value       = m;
          break;
        }
        case 'H':
          // heartbeat, already reset above
          break;
      }
    };
    ws.onclose = () => {
      console.log('WebSocket closed');
      setOffline();
    };
    ws.onerror = () => {
      console.error('WebSocket error');
      setOffline();
    };

    function send(k, v) { fetch(`/set?${k}=${encodeURIComponent(v)}`); }
    const sendColor      = debounce(() => send('color', document.getElementById('color').value), 100);
    const sendBrightness = debounce(() => send('brightness', document.getElementById('brightness').value), 100);

    window.addEventListener('load', () => {
      fetch('/state').then(r => r.text()).then(msg => {
        if (msg[0] === 'F') {
          setOnline();
          const [hex, b, s, m] = msg.slice(1).split(',');
          document.getElementById('color').value      = '#' + hex;
          document.getElementById('brightness').value = parseInt(b, 10);
          updateButtons(s === '1');
          document.getElementById('mode').value       = m;
        }
      });
      document.getElementById('color')     .addEventListener('input', sendColor);
      document.getElementById('brightness').addEventListener('input', sendBrightness);
      document.getElementById('mode')      .addEventListener('change', () => send('mode_id', document.getElementById('mode').value));
      document.getElementById('btnOn')     .addEventListener('click', () => { send('state','1'); updateButtons(true); });
      document.getElementById('btnOff')    .addEventListener('click', () => { send('state','0'); updateButtons(false); });
    });

    function updateButtons(on) {
      document.getElementById('btnOn').disabled  = on;
      document.getElementById('btnOff').disabled = !on;
    }
  </script>
</body>
</html>
)rawliteral";



// Add a Ticker instance for heartbeat
static Ticker heartbeatTicker;

WebServer::WebServer(SystemController& controller, AsyncWebServer& server)
  : controller_(controller), server_(server)
{
  DBG_PRINTF(WebServer, "WebServer: constructed (this=%p, controller=%p, server=%p)\n",
             this, &controller, &server);
}

void WebServer::begin() {
  DBG_PRINTLN(WebServer, "begin(): registering routes and WebSocket handler");
  server_.on("/",      HTTP_GET, [this](auto* r){ serve_main_page(r); });
  server_.on("/set",   HTTP_GET, [this](auto* r){ handle_set(r);     });
  server_.on("/state", HTTP_GET, [this](auto* r){ handle_get_state(r); });
  server_.addHandler(&ws_);
  ws_.onEvent([this](auto*, auto*, AwsEventType t, auto*, auto*, auto){
    if (t == WS_EVT_CONNECT) {
      DBG_PRINTLN(WebServer, "WebSocket client connected, sending full state");
      broadcast_led_state("full");
    }
  });

  // Start heartbeat: broadcast state every 3 seconds
  heartbeatTicker.attach(5, [this]() {
    DBG_PRINTLN(WebServer, "heartbeat: broadcasting full state");
    broadcast_led_state("full");
  });

  server_.begin();
  DBG_PRINTLN(WebServer, "begin(): server started");
}

void WebServer::serve_main_page(AsyncWebServerRequest* req) {
  DBG_PRINTF(WebServer, "serve_main_page(): url=%s\n", req->url().c_str());
  req->send_P(200, "text/html", INDEX_HTML);
  DBG_PRINTLN(WebServer, "serve_main_page(): response sent");
}

void WebServer::handle_set(AsyncWebServerRequest* req) {
  DBG_PRINTF(WebServer, "handle_set(): url=%s\n", req->url().c_str());
  char buf[12];
  if (auto* p = req->getParam("color")) {
    char hexbuf[8];
    p->value().toCharArray(hexbuf, sizeof(hexbuf));  // "#RRGGBB"
    long v = strtol(hexbuf + 1, nullptr, 16);
    snprintf(buf, sizeof(buf), "%u %u %u",
             (unsigned)((v >> 16) & 0xFF),
             (unsigned)((v >>  8) & 0xFF),
             (unsigned)( v        & 0xFF));
    DBG_PRINTF(WebServer, "handle_set: parsed RGB='%s'\n", buf);
    controller_.led_strip_set_rgb(buf);
  }
  else if (auto* p = req->getParam("brightness")) {
    p->value().toCharArray(buf, sizeof(buf));
    DBG_PRINTF(WebServer, "handle_set: brightness='%s'\n", buf);
    controller_.led_strip_set_brightness(buf);
  }
  else if (auto* p = req->getParam("state")) {
    p->value().toCharArray(buf, sizeof(buf));
    DBG_PRINTF(WebServer, "handle_set: state='%s'\n", buf);
    controller_.led_strip_set_state(buf);
  }
  else if (auto* p = req->getParam("mode_id")) {
    p->value().toCharArray(buf, sizeof(buf));
    DBG_PRINTF(WebServer, "handle_set: mode_id='%s'\n", buf);
    controller_.led_strip_set_mode(buf);
  }
  req->send(200, "text/plain", "ok");
  DBG_PRINTLN(WebServer, "handle_set(): response sent");
}

void WebServer::handle_get_state(AsyncWebServerRequest* req) {
  DBG_PRINTF(WebServer, "handle_get_state(): url=%s\n", req->url().c_str());
  update_state_payload("full");
  DBG_PRINTF(WebServer, "handle_get_state: payload='%s'\n", payload_);
  req->send(200, "text/plain", payload_);
  DBG_PRINTLN(WebServer, "handle_get_state(): response sent");
}

void WebServer::update_state_payload(const char* field) {
  DBG_PRINTF(WebServer, "update_state_payload(): field='%s'\n", field);
  if (strcmp(field, "color") == 0) {
    String c = controller_.led_strip_get_color_hex();
    unsigned long rgb = strtoul(c.c_str() + 1, nullptr, 16);
    payload_len_ = snprintf(payload_, kBufSize, "C%06lX", rgb);
    DBG_PRINTF(WebServer, "update_state_payload: color hex='%s' -> payload='%s'\n", c.c_str(), payload_);
  }
  else if (strcmp(field, "brightness") == 0) {
    uint8_t b = controller_.led_strip_get_brightness();
    payload_len_ = snprintf(payload_, kBufSize, "B%u", (unsigned)b);
    DBG_PRINTF(WebServer, "update_state_payload: brightness=%u -> payload='%s'\n", b, payload_);
  }
  else if (strcmp(field, "state") == 0) {
    bool s = controller_.led_strip_get_state();
    payload_len_ = snprintf(payload_, kBufSize, "S%u", (unsigned)s);
    DBG_PRINTF(WebServer, "update_state_payload: state=%u -> payload='%s'\n", (unsigned)s, payload_);
  }
  else if (strcmp(field, "mode") == 0) {
    uint8_t m = controller_.led_strip_get_mode_id();
    payload_len_ = snprintf(payload_, kBufSize, "M%u", (unsigned)m);
    DBG_PRINTF(WebServer, "update_state_payload: mode=%u -> payload='%s'\n", m, payload_);
  }
  else {  // full
    String c = controller_.led_strip_get_color_hex();
    unsigned long rgb = strtoul(c.c_str() + 1, nullptr, 16);
    uint8_t b = controller_.led_strip_get_brightness();
    bool    s = controller_.led_strip_get_state();
    uint8_t m = controller_.led_strip_get_mode_id();
    payload_len_ = snprintf(
      payload_, kBufSize, "F%06lX,%u,%u,%u",
      rgb, (unsigned)b, (unsigned)s, (unsigned)m
    );
    DBG_PRINTF(WebServer, "update_state_payload: full -> payload='%s'\n", payload_);
  }
  if (payload_len_ >= kBufSize) {
    payload_len_ = kBufSize - 1;
    DBG_PRINTLN(WebServer, "update_state_payload: payload truncated to buffer size");
  }
  payload_[payload_len_] = '\0';
}

void WebServer::broadcast_led_state(const char* field) {
  DBG_PRINTF(WebServer, "broadcast_led_state(): field='%s'\n", field);
  update_state_payload(field);
  DBG_PRINTF(WebServer, "broadcast_led_state: sending '%s' to all clients (%u bytes)\n",
             payload_, (unsigned)payload_len_);
  ws_.textAll(payload_, payload_len_);
}
