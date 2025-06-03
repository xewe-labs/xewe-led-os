// WebInterface.cpp

#include "WebInterface.h"
#include "../../SystemController/SystemController.h"
#include <Ticker.h>            // for periodic heartbeat
#include <ESPAsyncWebServer.h> // AsyncWebServer, AsyncWebSocket, AsyncWebHeader

// ─── Main Control Page ───────────────────────────────────────────────────────────

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>LED Control</title>
  <style>
    :root {
      --bg: #000; --fg: #fff; --accent: #0ff;
      --green: #0f0; --red: #f00;
      --font: system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', sans-serif;
    }
    *, *::before, *::after { box-sizing: border-box; margin:0; padding:0; }
    body {
      background: var(--bg); color: var(--fg);
      font-family: var(--font);
      display: flex; flex-direction: column; align-items: center;
      padding: 1rem; min-height: 100vh;
    }
    h1 { margin-bottom: .5rem; font-size: 1.75rem; font-weight: 500; }
    #status { display: flex; align-items: center; margin-bottom: 1.5rem; }
    #status-indicator {
      width: 12px; height: 12px; border-radius: 50%;
      background: var(--red); margin-right: .5rem;
    }
    .control {
      width: 100%; max-width: 400px;
      display: flex; justify-content: space-between; align-items: center;
      margin-bottom: 1rem;
    }
    label { flex: 1; display: flex; justify-content: space-between; align-items: center; }
    input[type="color"], input[type="range"], select {
      appearance: none; background: var(--bg);
      border: 1px solid var(--accent); border-radius: 4px;
      padding: .25rem; color: var(--fg); margin-left: .5rem; flex: 1;
    }
    .buttons {
      display: flex; width: 100%; max-width: 400px; margin-top: 1rem;
    }
    button {
      flex: 1; padding: .75rem; margin: .25rem;
      background: var(--accent); border: none; border-radius: 4px;
      color: var(--bg); font-size: 1rem; cursor: pointer;
      transition: opacity .2s ease;
    }
    button:disabled { opacity: .4; cursor: not-allowed; }
  </style>
</head>
<body>
  <h1>LED Strip Control</h1>
  <div id="status">
    <div id="status-indicator"></div>
    <div id="status-text">Offline</div>
  </div>

  <div class="control">
    <label>Color<input type="color" id="color"/></label>
  </div>
  <div class="control">
    <label>Brightness<input type="range" id="brightness" min="1" max="255"/></label>
  </div>
  <div class="control">
    <label>Mode
      <select id="mode">
        <option value="0">Color Solid</option>
        <!-- more modes here -->
      </select>
    </label>
  </div>

  <div class="buttons">
    <button id="btnOn">On</button>
    <button id="btnOff">Off</button>
    <button id="btnShortcut">Create Shortcut</button>
  </div>

  <script>
    function debounce(fn, d) { let t; return (...a) => { clearTimeout(t); t = setTimeout(() => fn(...a), d); }; }

    const ws = new WebSocket(`ws://${location.host}/ws`);
    let offlineTimer, reloadTimer;

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

    ws.onopen = () => { console.log('WS open'); setOnline(); };
    ws.onmessage = e => {
      setOnline();
      const tag = e.data[0], data = e.data.slice(1);
      switch (tag) {
        case 'C': document.getElementById('color').value = '#' + data; break;
        case 'B': document.getElementById('brightness').value = parseInt(data,10); break;
        case 'S': updateButtons(data === '1'); break;
        case 'M': document.getElementById('mode').value = data; break;
        case 'F': {
          const [hex,b,s,m] = data.split(',');
          document.getElementById('color').value      = '#' + hex;
          document.getElementById('brightness').value = parseInt(b,10);
          updateButtons(s === '1');
          document.getElementById('mode').value       = m;
        } break;
      }
    };
    ws.onclose = ws.onerror = () => { console.log('WS closed/error'); setOffline(); };

    function send(k, v) { fetch(`/set?${k}=${encodeURIComponent(v)}`); }
    const sendColor  = debounce(() => send('color',      document.getElementById('color').value),      100);
    const sendBright = debounce(() => send('brightness', document.getElementById('brightness').value), 100);

    window.addEventListener('load', () => {
      fetch('/state').then(r => r.text()).then(msg => {
        if (msg[0] === 'F') {
          setOnline();
          const [hex,b,s,m] = msg.slice(1).split(',');
          document.getElementById('color').value      = '#' + hex;
          document.getElementById('brightness').value = parseInt(b,10);
          updateButtons(s === '1');
          document.getElementById('mode').value       = m;
        }
      });

      document.getElementById('color')     .addEventListener('input', sendColor);
      document.getElementById('brightness').addEventListener('input', sendBright);
      document.getElementById('mode')      .addEventListener('change', () => send('mode_id', document.getElementById('mode').value));
      document.getElementById('btnOn')     .addEventListener('click', () => { send('state','1'); updateButtons(true); });
      document.getElementById('btnOff')    .addEventListener('click', () => { send('state','0'); updateButtons(false); });

      document.getElementById('btnShortcut').addEventListener('click', () => {
        const params = new URLSearchParams({
          color:      document.getElementById('color').value,
          brightness: document.getElementById('brightness').value,
          state:      document.getElementById('btnOn').disabled ? '1' : '0',
          mode_id:    document.getElementById('mode').value
        });
        window.location.href = `/set_state?${params.toString()}`;
      });
    });

    function updateButtons(on) {
      document.getElementById('btnOn').disabled  = on;
      document.getElementById('btnOff').disabled = !on;
    }
  </script>
</body>
</html>
)rawliteral";

// ─── Instruction Page ────────────────────────────────────────────────────────────

static const char SET_STATE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Save LED Shortcut</title>
  <style>
    body {
      background:#000; color:#fff; font-family:system-ui;
      display:flex; flex-direction:column; align-items:center;
      justify-content:center; height:100vh;
      text-align:center; padding:1rem;
    }
    h1 { margin-bottom:1rem; font-size:1.5rem; }
    p  { line-height:1.5; }
  </style>
</head>
<body>
  <h1>Save This Preset</h1>
  <p>
    Tap the <strong>Share</strong> button,<br>
    then <strong>Add to Home Screen</strong>.<br>
    Launching from your home screen will apply the saved state.
  </p>
</body>
</html>
)rawliteral";

static Ticker heartbeatTicker;

WebInterface::WebInterface(SystemController& controller, AsyncWebServer& server)
  : controller_(controller), server_(server)
{
  DBG_PRINTF(WebInterface,
             "WebInterface: constructed (this=%p, controller=%p, server=%p)\n",
             this, &controller, &server);
}

void WebInterface::begin() {
  DBG_PRINTLN(WebInterface, "begin(): registering routes and WebSocket handler");

  server_.on("/",          HTTP_GET, [this](auto* req){ serve_main_page(req);    });
  server_.on("/set",       HTTP_GET, [this](auto* req){ handle_set(req);         });
  server_.on("/set_state", HTTP_GET, [this](auto* req){ handle_set_state(req);    });
  server_.on("/state",     HTTP_GET, [this](auto* req){ handle_get_state(req);    });

  server_.addHandler(&ws_);
  ws_.onEvent([this](AsyncWebSocket* /*ws*/, AsyncWebSocketClient* /*client*/,
                     AwsEventType type, void* /*arg*/, uint8_t* /*data*/, size_t /*len*/) {
    if (type == WS_EVT_CONNECT) {
      DBG_PRINTLN(WebInterface, "WebSocket client connected, sending full state");
      broadcast_led_state("full");
    }
  });

  heartbeatTicker.attach(5, [this]() {
    DBG_PRINTLN(WebInterface, "heartbeat: broadcasting full state");
    broadcast_led_state("full");
  });

  server_.begin();
  DBG_PRINTLN(WebInterface, "begin(): server started");
}

void WebInterface::serve_main_page(AsyncWebServerRequest* req) {
  DBG_PRINTF(WebInterface, "serve_main_page(): url=%s\n", req->url().c_str());
  req->send_P(200, "text/html", INDEX_HTML);
  DBG_PRINTLN(WebInterface, "serve_main_page(): response sent");
}

void WebInterface::handle_set(AsyncWebServerRequest* req) {
  DBG_PRINTF(WebInterface, "handle_set(): url=%s\n", req->url().c_str());
  char buf[12];
  if (auto* p = req->getParam("color")) {
    char hexbuf[8];
    p->value().toCharArray(hexbuf, sizeof(hexbuf));
    long v = strtol(hexbuf + 1, nullptr, 16);
    snprintf(buf, sizeof(buf), "%u %u %u",
             (unsigned)((v >> 16) & 0xFF),
             (unsigned)((v >>  8) & 0xFF),
             (unsigned)( v        & 0xFF));
    DBG_PRINTF(WebInterface, "handle_set: parsed RGB='%s'\n", buf);
    controller_.led_strip_set_rgb(buf);
  }
  else if (auto* p = req->getParam("brightness")) {
    p->value().toCharArray(buf, sizeof(buf));
    DBG_PRINTF(WebInterface, "handle_set: brightness='%s'\n", buf);
    controller_.led_strip_set_brightness(buf);
  }
  else if (auto* p = req->getParam("state")) {
    p->value().toCharArray(buf, sizeof(buf));
    DBG_PRINTF(WebInterface, "handle_set: state='%s'\n", buf);
    controller_.led_strip_set_state(buf);
  }
  else if (auto* p = req->getParam("mode_id")) {
    p->value().toCharArray(buf, sizeof(buf));
    DBG_PRINTF(WebInterface, "handle_set: mode_id='%s'\n", buf);
    controller_.led_strip_set_mode(buf);
  }
  req->send(200, "text/plain", "ok");
  DBG_PRINTLN(WebInterface, "handle_set(): response sent");
}

void WebInterface::handle_set_state(AsyncWebServerRequest* req) {
  DBG_PRINTF(WebInterface, "handle_set_state(): url=%s\n", req->url().c_str());
  char buf[12];

  // Apply all parameters
  if (auto* p = req->getParam("color")) {
    char hexbuf[8];
    p->value().toCharArray(hexbuf, sizeof(hexbuf));
    long v = strtol(hexbuf + 1, nullptr, 16);
    snprintf(buf, sizeof(buf), "%u %u %u",
             (unsigned)((v >> 16) & 0xFF),
             (unsigned)((v >>  8) & 0xFF),
             (unsigned)( v        & 0xFF));
    DBG_PRINTF(WebInterface, "handle_set_state: RGB='%s'\n", buf);
    controller_.led_strip_set_rgb(buf);
  }
  if (auto* p = req->getParam("brightness")) {
    p->value().toCharArray(buf, sizeof(buf));
    DBG_PRINTF(WebInterface, "handle_set_state: brightness='%s'\n", buf);
    controller_.led_strip_set_brightness(buf);
  }
  if (auto* p = req->getParam("state")) {
    p->value().toCharArray(buf, sizeof(buf));
    DBG_PRINTF(WebInterface, "handle_set_state: state='%s'\n", buf);
    controller_.led_strip_set_state(buf);
  }
  if (auto* p = req->getParam("mode_id")) {
    p->value().toCharArray(buf, sizeof(buf));
    DBG_PRINTF(WebInterface, "handle_set_state: mode_id='%s'\n", buf);
    controller_.led_strip_set_mode(buf);
  }

  // Referer-based branching:
  const AsyncWebHeader* ref = req->getHeader("Referer");
  if (ref && !ref->value().isEmpty()) {
    DBG_PRINTLN(WebInterface, "handle_set_state: initial click, serving instructions");
    req->send_P(200, "text/html", SET_STATE_HTML);
    DBG_PRINTLN(WebInterface, "handle_set_state: instruction page sent");
  } else {
    DBG_PRINTLN(WebInterface, "handle_set_state: home-screen launch, redirecting to /");
    req->redirect("/");
  }
}

void WebInterface::handle_get_state(AsyncWebServerRequest* req) {
  DBG_PRINTF(WebInterface, "handle_get_state(): url=%s\n", req->url().c_str());
  update_state_payload("full");
  DBG_PRINTF(WebInterface, "handle_get_state: payload='%s'\n", payload_);
  req->send(200, "text/plain", payload_);
  DBG_PRINTLN(WebInterface, "handle_get_state(): response sent");
}

void WebInterface::update_state_payload(const char* field) {
  DBG_PRINTF(WebInterface, "update_state_payload(): field='%s'\n", field);
  if (strcmp(field, "color") == 0) {
    String c = controller_.led_strip_get_color_hex();
    unsigned long rgb = strtoul(c.c_str() + 1, nullptr, 16);
    payload_len_ = snprintf(payload_, kBufSize, "C%06lX", rgb);
    DBG_PRINTF(WebInterface, "update_state_payload: color='%s' -> '%s'\n", c.c_str(), payload_);
  } else if (strcmp(field, "brightness") == 0) {
    uint8_t b = controller_.led_strip_get_brightness();
    payload_len_ = snprintf(payload_, kBufSize, "B%u", (unsigned)b);
    DBG_PRINTF(WebInterface, "update_state_payload: brightness=%u -> '%s'\n", b, payload_);
  } else if (strcmp(field, "state") == 0) {
    bool s = controller_.led_strip_get_state();
    payload_len_ = snprintf(payload_, kBufSize, "S%u", (unsigned)s);
    DBG_PRINTF(WebInterface, "update_state_payload: state=%u -> '%s'\n", (unsigned)s, payload_);
  } else if (strcmp(field, "mode") == 0) {
    uint8_t m = controller_.led_strip_get_mode_id();
    payload_len_ = snprintf(payload_, kBufSize, "M%u", (unsigned)m);
    DBG_PRINTF(WebInterface, "update_state_payload: mode=%u -> '%s'\n", m, payload_);
  } else {
    String c = controller_.led_strip_get_color_hex();
    unsigned long rgb = strtoul(c.c_str() + 1, nullptr, 16);
    uint8_t b = controller_.led_strip_get_brightness();
    bool    s = controller_.led_strip_get_state();
    uint8_t m = controller_.led_strip_get_mode_id();
    payload_len_ = snprintf(
      payload_, kBufSize, "F%06lX,%u,%u,%u",
      rgb, (unsigned)b, (unsigned)s, (unsigned)m
    );
    DBG_PRINTF(WebInterface, "update_state_payload: full -> '%s'\n", payload_);
  }
  if (payload_len_ >= kBufSize) {
    payload_len_ = kBufSize - 1;
    DBG_PRINTLN(WebInterface, "update_state_payload: payload truncated");
  }
  payload_[payload_len_] = '\0';
}

void WebInterface::broadcast_led_state(const char* field) {
  DBG_PRINTF(WebInterface, "broadcast_led_state(): field='%s'\n", field);
  update_state_payload(field);
  DBG_PRINTF(WebInterface, "broadcast_led_state: sending '%s' (%u bytes)\n",
             payload_, (unsigned)payload_len_);
  ws_.textAll(payload_, payload_len_);
}
