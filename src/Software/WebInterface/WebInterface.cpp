#include "WebInterface.h"
#include "../../SystemController/SystemController.h"
#include <Ticker.h>
#include <functional> // For std::bind

// --- Main Control Page (JavaScript rewritten for robust WebSocket and Optimistic UI) ---
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
    const DEBOUNCE_DELAY_MS = 250;

    const colorInput = document.getElementById('color');
    const brightnessInput = document.getElementById('brightness');
    const modeSelect = document.getElementById('mode');
    const btnOn = document.getElementById('btnOn');
    const btnOff = document.getElementById('btnOff');
    const statusIndicator = document.getElementById('status-indicator');
    const statusText = document.getElementById('status-text');

    let ws;
    let offlineTimer;
    let reloadTimer;

    function debounce(fn, d) { let t; return (...a) => { clearTimeout(t); t = setTimeout(() => fn(...a), d); }; }

    function setOnline() {
      clearTimeout(offlineTimer);
      clearTimeout(reloadTimer);
      statusIndicator.style.background = 'var(--green)';
      statusText.textContent = 'Online';
      offlineTimer = setTimeout(setOffline, 7000); // If no message for 7s, go offline
    }

    function setOffline() {
      statusIndicator.style.background = 'var(--red)';
      statusText.textContent = 'Offline';
      ws.close();
      reloadTimer = setTimeout(connect, 5000); // Attempt to reconnect after 5s
    }

    function updateButtons(on) {
      btnOn.disabled = on;
      btnOff.disabled = !on;
    }

    function connect() {
        console.log("Attempting WebSocket connection...");
        ws = new WebSocket(`ws://${location.hostname}:81/`);

        ws.onopen = () => {
            console.log('WebSocket connection established.');
            setOnline();
        };

        ws.onmessage = e => {
            setOnline(); // A message is our heartbeat, reset the offline timer
            const tag = e.data[0], data = e.data.slice(1);
            switch (tag) {
                case 'C': colorInput.value = '#' + data; break;
                case 'B': brightnessInput.value = parseInt(data,10); break;
                case 'S': updateButtons(data === '1'); break;
                case 'M': modeSelect.value = data; break;
                case 'F': {
                    const [hex,b,s,m] = data.split(',');
                    colorInput.value = '#' + hex;
                    brightnessInput.value = parseInt(b,10);
                    updateButtons(s === '1');
                    modeSelect.value = m;
                } break;
            }
        };

        ws.onclose = () => {
            console.log('WebSocket connection closed.');
            setOffline();
        };

        ws.onerror = (error) => {
            console.error('WebSocket error:', error);
            setOffline();
        };
    }

    function sendCommand(k, v) {
      // All commands are sent via simple, fire-and-forget HTTP requests
      fetch(`/set?${k}=${encodeURIComponent(v)}`).catch(err => console.error("Send failed:", err));
    }

    const sendColorCommand  = debounce(() => sendCommand('color', colorInput.value.substring(1)), DEBOUNCE_DELAY_MS);
    const sendBrightCommand = debounce(() => sendCommand('brightness', brightnessInput.value), DEBOUNCE_DELAY_MS);

    window.addEventListener('load', () => {
      // Optimistic UI updates. Update the UI *immediately* on click.
      // The WebSocket message from the server will confirm the state.
      btnOn.addEventListener('click', () => {
        sendCommand('state','1');
        updateButtons(true);
      });
      btnOff.addEventListener('click', () => {
        sendCommand('state','0');
        updateButtons(false);
      });

      colorInput.addEventListener('input', sendColorCommand);
      brightnessInput.addEventListener('input', sendBrightCommand);
      modeSelect.addEventListener('change', () => sendCommand('mode_id', modeSelect.value));

      document.getElementById('btnShortcut').addEventListener('click', () => {
        const params = new URLSearchParams({
          color:      colorInput.value.substring(1),
          brightness: brightnessInput.value,
          state:      btnOn.disabled ? '1' : '0',
          mode_id:    modeSelect.value
        });
        window.location.href = `/set_state?${params.toString()}`;
      });

      connect(); // Initial connection
    });
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

  // Heartbeat to ensure clients get updates and stay "online"
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
      broadcast_led_state("full");
      break;
    }
    case WStype_TEXT:
      DBG_PRINTF(WebInterface, "[WSc] Received text from #%u: %s\n", num, payload);
      break;
    default:
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
  // Logic for setting state from shortcut...
  if (server_.hasArg("color")) {
    String color_val = server_.arg("color");
    long v = strtol(color_val.c_str(), nullptr, 16);
    char buf[12];
    snprintf(buf, sizeof(buf), "%u %u %u", (unsigned)((v >> 16) & 0xFF), (unsigned)((v >> 8) & 0xFF), (unsigned)(v & 0xFF));
    controller_.led_strip_set_rgb(buf);
  }
  if (server_.hasArg("brightness")) {
    controller_.led_strip_set_brightness(server_.arg("brightness"));
  }
  if (server_.hasArg("state")) {
    controller_.led_strip_set_state(server_.arg("state"));
  }
  if (server_.hasArg("mode_id")) {
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
