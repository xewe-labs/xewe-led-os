#include "WebInterface.h"
#include "../../SystemController/SystemController.h"
#include <Ticker.h>
#include <functional> // For std::bind

// --- Main Control Page (JavaScript reverted to WebSocket implementation) ---
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
      background: var(--red); margin-right: .5rem; transition: background 0.5s ease;
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

    // WebSocket connects to port 81, while HTTP requests go to port 80.
    const ws = new WebSocket(`ws://${location.hostname}:81/`);
    let offlineTimer, reloadTimer;

    function setOnline() {
      clearTimeout(offlineTimer);
      clearTimeout(reloadTimer);
      document.getElementById('status-indicator').style.background = 'var(--green)';
      document.getElementById('status-text').textContent = 'Online';
      // Consider offline if no message is received for 7 seconds
      offlineTimer = setTimeout(setOffline, 7000);
    }

    function setOffline() {
      document.getElementById('status-indicator').style.background = 'var(--red)';
      document.getElementById('status-text').textContent = 'Offline';
      // Attempt to reload the page to re-establish connection
      reloadTimer = setTimeout(() => location.reload(), 5000);
    }

    ws.onopen = () => { console.log('WebSocket connection established.'); setOnline(); };

    ws.onmessage = e => {
      setOnline(); // Reset the offline timer on every message
      const tag = e.data[0], data = e.data.slice(1);
      switch (tag) {
        case 'C': document.getElementById('color').value = '#' + data; break;
        case 'B': document.getElementById('brightness').value = parseInt(data,10); break;
        case 'S': updateButtons(data === '1'); break;
        case 'M': document.getElementById('mode').value = data; break;
        case 'F': {
          const [hex,b,s,m] = data.split(',');
          document.getElementById('color').value = '#' + hex;
          document.getElementById('brightness').value = parseInt(b,10);
          updateButtons(s === '1');
          document.getElementById('mode').value = m;
        } break;
      }
    };

    ws.onclose = ws.onerror = () => { console.log('WebSocket connection closed or error.'); setOffline(); };

    // HTTP requests are still used to send commands to the ESP
    function send(k, v) { fetch(`/set?${k}=${encodeURIComponent(v)}`); }

    const sendColor  = debounce(() => send('color', document.getElementById('color').value.substring(1)), 250);
    const sendBright = debounce(() => send('brightness', document.getElementById('brightness').value), 250);

    window.addEventListener('load', () => {
      document.getElementById('color').addEventListener('input', sendColor);
      document.getElementById('brightness').addEventListener('input', sendBright);
      document.getElementById('mode').addEventListener('change', () => send('mode_id', document.getElementById('mode').value));
      document.getElementById('btnOn').addEventListener('click', () => { send('state','1'); });
      document.getElementById('btnOff').addEventListener('click', () => { send('state','0'); });

      document.getElementById('btnShortcut').addEventListener('click', () => {
        const params = new URLSearchParams({
          color:      document.getElementById('color').value.substring(1),
          brightness: document.getElementById('brightness').value,
          state:      document.getElementById('btnOn').disabled ? '1' : '0',
          mode_id:    document.getElementById('mode').value
        });
        window.location.href = `/set_state?${params.toString()}`;
      });
    });

    function updateButtons(on) {
      document.getElementById('btnOn').disabled = on;
      document.getElementById('btnOff').disabled = !on;
    }
  </script>
</body>
</html>
)rawliteral";

// --- Instruction Page ---
static const char SET_STATE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Save LED Shortcut</title><style>body{background:#000;color:#fff;font-family:system-ui;display:flex;flex-direction:column;align-items:center;justify-content:center;height:100vh;text-align:center;padding:1rem}h1{margin-bottom:1rem;font-size:1.5rem}p{line-height:1.5}</style></head><body><h1>Save This Preset</h1><p>Tap the <strong>Share</strong> button,<br>then <strong>Add to Home Screen</strong>.<br>Launching from your home screen will apply the saved state.</p></body></html>
)rawliteral";

static Ticker heartbeatTicker;

WebInterface::WebInterface(SystemController& controller, WebServer& server)
  : controller_(controller), server_(server) {
  DBG_PRINTF(WebInterface, "WebInterface: constructed (this=%p, controller=%p, server=%p)\n", this, &controller, &server);
}

void WebInterface::begin() {
  DBG_PRINTLN(WebInterface, "begin(): registering routes and WebSocket handler");

  // Register HTTP Routes
  server_.on("/", HTTP_GET, std::bind(&WebInterface::serve_main_page, this));
  server_.on("/set", HTTP_GET, std::bind(&WebInterface::handle_set, this));
  server_.on("/set_state", HTTP_GET, std::bind(&WebInterface::handle_set_state, this));
  server_.on("/state", HTTP_GET, std::bind(&WebInterface::handle_get_state, this));

  // Start WebSocket Server and assign event handler
  ws_.begin();
  ws_.onEvent(std::bind(&WebInterface::webSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

  // Heartbeat to ensure clients get updates even if no changes are made
  heartbeatTicker.attach(5, [this]() {
    this->broadcast_led_state("full");
  });

  DBG_PRINTLN(WebInterface, "begin(): Web interface setup complete.");
}

// This MUST be called from the main sketch loop()
void WebInterface::loop() {
  ws_.loop();
}

void WebInterface::webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      DBG_PRINTF(WebInterface, "[WSc] Client #%u disconnected.\n", num);
      break;
    case WStype_CONNECTED: {
      IPAddress ip = ws_.remoteIP(num);
      DBG_PRINTF(WebInterface, "[WSc] Client #%u connected from %s.\n", num, ip.toString().c_str());

      // Send the current full state to the newly connected client
      update_state_payload("full");
      ws_.sendTXT(num, payload_, payload_len_);
      break;
    }
    case WStype_TEXT:
      // Not expecting any text from client, but log if received
      DBG_PRINTF(WebInterface, "[WSc] Received text from #%u: %s\n", num, payload);
      break;
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }
}

void WebInterface::serve_main_page() {
  server_.send_P(200, "text/html", INDEX_HTML);
}

void WebInterface::handle_set() {
  if (server_.hasArg("color")) {
    String color_val = server_.arg("color");
    long v = strtol(color_val.c_str(), nullptr, 16);
    char buf[12];
    snprintf(buf, sizeof(buf), "%u %u %u", (unsigned)((v >> 16) & 0xFF), (unsigned)((v >> 8) & 0xFF), (unsigned)(v & 0xFF));
    controller_.led_strip_set_rgb(buf);
  } else if (server_.hasArg("brightness")) {
    controller_.led_strip_set_brightness(server_.arg("brightness"));
  } else if (server_.hasArg("state")) {
    controller_.led_strip_set_state(server_.arg("state"));
  } else if (server_.hasArg("mode_id")) {
    controller_.led_strip_set_mode(server_.arg("mode_id"));
  }
  server_.send(200, "text/plain", "ok");
}

void WebInterface::handle_set_state() {
  if (server_.hasArg("color")) {
    String color_val = server_.arg("color");
    long v = strtol(color_val.c_str(), nullptr, 16);
    char buf[12];
    snprintf(buf, sizeof(buf), "%u %u %u", (unsigned)((v >> 16) & 0xFF), (unsigned)((v >> 8) & 0xFF), (unsigned)(v & 0xFF));
    controller_.led_strip_set_rgb(buf);
  } else if (server_.hasArg("brightness")) {
    controller_.led_strip_set_brightness(server_.arg("brightness"));
  } else if (server_.hasArg("state")) {
    controller_.led_strip_set_state(server_.arg("state"));
  } else if (server_.hasArg("mode_id")) {
    controller_.led_strip_set_mode(server_.arg("mode_id"));
  }

  if (server_.hasHeader("Referer")) {
    server_.send_P(200, "text/html", SET_STATE_HTML);
  } else {
    server_.sendHeader("Location", "/", true);
    server_.send(302, "text/plain", "");
  }
}

void WebInterface::handle_get_state() {
  update_state_payload("full");
  server_.send(200, "text/plain", payload_);
}

void WebInterface::update_state_payload(const char* field) {
  if (strcmp(field, "color") == 0) {
    String c = controller_.led_strip_get_color_hex();
    payload_len_ = snprintf(payload_, kBufSize, "C%s", c.c_str() + 1); // Skip '#'
  } else if (strcmp(field, "brightness") == 0) {
    uint8_t b = controller_.led_strip_get_brightness();
    payload_len_ = snprintf(payload_, kBufSize, "B%u", (unsigned)b);
  } else if (strcmp(field, "state") == 0) {
    bool s = controller_.led_strip_get_state();
    payload_len_ = snprintf(payload_, kBufSize, "S%u", (unsigned)s);
  } else if (strcmp(field, "mode") == 0) {
    uint8_t m = controller_.led_strip_get_mode_id();
    payload_len_ = snprintf(payload_, kBufSize, "M%u", (unsigned)m);
  } else { // "full"
    String c = controller_.led_strip_get_color_hex();
    uint8_t b = controller_.led_strip_get_brightness();
    bool    s = controller_.led_strip_get_state();
    uint8_t m = controller_.led_strip_get_mode_id();
    payload_len_ = snprintf(payload_, kBufSize, "F%s,%u,%u,%u", c.c_str() + 1, (unsigned)b, (unsigned)s, (unsigned)m);
  }

  if (payload_len_ >= kBufSize) payload_len_ = kBufSize - 1;
  payload_[payload_len_] = '\0';
}

void WebInterface::broadcast_led_state(const char* field) {
  update_state_payload(field);
  ws_.broadcastTXT(payload_, payload_len_);
  DBG_PRINTF(WebInterface, "[WSc] Broadcasting state ('%s'): %s\n", field, payload_);
}
