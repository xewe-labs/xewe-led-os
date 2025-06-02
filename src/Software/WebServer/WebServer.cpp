// WebServer.cpp
#include "WebServer.h"
#include "../../SystemController/SystemController.h" // For controller_ interaction

// --- Main Control Page (Modified JavaScript for Polling) ---
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

    let isOnline = false;
    let pollInterval;
    const POLLING_RATE_MS = 2000; // Poll every 2 seconds
    const OFFLINE_TIMEOUT_MS = POLLING_RATE_MS * 3; // Consider offline after 3 failed polls
    let offlineDetectionTimer;

    function setOnlineStatus(online) {
      isOnline = online;
      document.getElementById('status-indicator').style.background = online ? 'var(--green)' : 'var(--red)';
      document.getElementById('status-text').textContent = online ? 'Online' : 'Offline';
      if (!online) {
        // Consider reloading if offline for too long, or just show offline
        console.log('Device appears offline.');
      }
    }

    function processStateUpdate(msg) {
      if (!msg || msg.length < 1) return;
      setOnlineStatus(true); // Got a message, so we are online
      clearTimeout(offlineDetectionTimer);
      offlineDetectionTimer = setTimeout(() => setOnlineStatus(false), OFFLINE_TIMEOUT_MS);

      const tag = msg[0], data = msg.slice(1);
      switch (tag) {
        case 'C': document.getElementById('color').value = '#' + data; break;
        case 'B': document.getElementById('brightness').value = parseInt(data,10); break;
        case 'S': updateButtons(data === '1'); break;
        case 'M': document.getElementById('mode').value = data; break;
        case 'F': { // Full state
          const [hex,b,s,m] = data.split(',');
          document.getElementById('color').value      = '#' + hex;
          document.getElementById('brightness').value = parseInt(b,10);
          updateButtons(s === '1');
          document.getElementById('mode').value       = m;
        } break;
      }
    }

    async function fetchState() {
      try {
        const response = await fetch('/state');
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        const data = await response.text();
        processStateUpdate(data);
      } catch (error) {
        console.error('Error fetching state:', error);
        // If fetch fails, we might be offline
        // The offlineDetectionTimer will handle actually setting the status to offline
      }
    }

    function sendCommand(k, v) {
      fetch(`/set?${k}=${encodeURIComponent(v)}`)
        .then(response => {
          if (response.ok) {
            fetchState(); // Fetch new state immediately after successful command
          } else {
            console.error('Command failed:', response.status);
            setOnlineStatus(false); // Command failed, potentially offline
          }
        })
        .catch(error => {
          console.error('Error sending command:', error);
          setOnlineStatus(false); // Network error, potentially offline
        });
    }

    const sendColorDebounced  = debounce(() => sendCommand('color', document.getElementById('color').value), 200);
    const sendBrightDebounced = debounce(() => sendCommand('brightness', document.getElementById('brightness').value), 200);

    window.addEventListener('load', () => {
      fetchState(); // Initial state load
      pollInterval = setInterval(fetchState, POLLING_RATE_MS); // Start polling
      // Initial offline timer until first successful poll
      offlineDetectionTimer = setTimeout(() => setOnlineStatus(false), OFFLINE_TIMEOUT_MS);


      document.getElementById('color').addEventListener('input', sendColorDebounced);
      document.getElementById('brightness').addEventListener('input', sendBrightDebounced);
      document.getElementById('mode').addEventListener('change', () => sendCommand('mode_id', document.getElementById('mode').value));
      document.getElementById('btnOn').addEventListener('click', () => { sendCommand('state','1'); updateButtons(true); });
      document.getElementById('btnOff').addEventListener('click', () => { sendCommand('state','0'); updateButtons(false); });

      document.getElementById('btnShortcut').addEventListener('click', () => {
        const params = new URLSearchParams({
          color:      document.getElementById('color').value.substring(1), // remove # for URL
          brightness: document.getElementById('brightness').value,
          state:      document.getElementById('btnOn').disabled ? '1' : '0', // State is if light is ON
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

// --- Instruction Page (no changes needed to this HTML itself) ---
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

// Constructor uses ::WebServer (ESP32 core WebServer)
WebServer::WebServer(SystemController& controller, ::WebServer& server)
  : controller_(controller), server_(server) {
  DBG_PRINTF(WebServer,
             "WebServer: constructed (this=%p, controller=%p, server=%p)\n",
             this, &controller, &server);
}

void WebServer::begin() {
  DBG_PRINTLN(WebServer, "begin(): registering routes");

  // Note: Lambdas now capture 'this' to call member functions.
  // No AsyncWebServerRequest* parameter is passed.
  server_.on("/", HTTP_GET, [this]() { this->serve_main_page(); });
  server_.on("/set", HTTP_GET, [this]() { this->handle_set(); });
  server_.on("/set_state", HTTP_GET, [this]() { this->handle_set_state(); });
  server_.on("/state", HTTP_GET, [this]() { this->handle_get_state(); });

  // Add a not found handler (optional, but good practice)
  server_.onNotFound([this]() {
    server_.send(404, "text/plain", "Not found");
  });

  // Ticker still updates the payload for polling clients
  heartbeatTicker.attach_ms(5000, [this]() { // Changed to attach_ms for clarity if Ticker supports it, else use 5 for seconds
    DBG_PRINTLN(WebServer, "heartbeat: updating full state payload for polling");
    this->broadcast_led_state("full"); // This now just updates the payload_
  });

  server_.begin(); // Start the synchronous server
  DBG_PRINTLN(WebServer, "begin(): server started");
}

void WebServer::update() {
  server_.handleClient(); // Process incoming client requests
}

void WebServer::serve_main_page() {
  DBG_PRINTF(WebServer, "serve_main_page(): url=%s\n", server_.uri().c_str());
  server_.send_P(200, "text/html", INDEX_HTML);
  DBG_PRINTLN(WebServer, "serve_main_page(): response sent");
}

void WebServer::handle_set() {
  DBG_PRINTF(WebServer, "handle_set(): url=%s\n", server_.uri().c_str());
  char buf[12]; // Buffer for string conversion

  if (server_.hasArg("color")) {
    String colorVal = server_.arg("color");
    // colorVal is expected as "#RRGGBB"
    long v = strtol(colorVal.c_str() + 1, nullptr, 16); // Skip '#'
    snprintf(buf, sizeof(buf), "%lu %lu %lu",
             (unsigned long)((v >> 16) & 0xFF),
             (unsigned long)((v >>  8) & 0xFF),
             (unsigned long)( v        & 0xFF));
    DBG_PRINTF(WebServer, "handle_set: parsed RGB='%s'\n", buf);
    controller_.led_strip_set_rgb(buf);
  } else if (server_.hasArg("brightness")) {
    String brightnessVal = server_.arg("brightness");
    brightnessVal.toCharArray(buf, sizeof(buf));
    DBG_PRINTF(WebServer, "handle_set: brightness='%s'\n", buf);
    controller_.led_strip_set_brightness(buf);
  } else if (server_.hasArg("state")) {
    String stateVal = server_.arg("state");
    stateVal.toCharArray(buf, sizeof(buf));
    DBG_PRINTF(WebServer, "handle_set: state='%s'\n", buf);
    controller_.led_strip_set_state(buf);
  } else if (server_.hasArg("mode_id")) {
    String modeVal = server_.arg("mode_id");
    modeVal.toCharArray(buf, sizeof(buf));
    DBG_PRINTF(WebServer, "handle_set: mode_id='%s'\n", buf);
    controller_.led_strip_set_mode(buf);
  }
  server_.send(200, "text/plain", "ok");
  DBG_PRINTLN(WebServer, "handle_set(): response sent");
}

void WebServer::handle_set_state() {
  DBG_PRINTF(WebServer, "handle_set_state(): url=%s\n", server_.uri().c_str());
  char buf[12];

  if (server_.hasArg("color")) {
    String colorVal = server_.arg("color");
    long v = strtol(colorVal.c_str(), nullptr, 16); // Assuming hex without # from URL params
    snprintf(buf, sizeof(buf), "%lu %lu %lu",
             (unsigned long)((v >> 16) & 0xFF),
             (unsigned long)((v >>  8) & 0xFF),
             (unsigned long)( v        & 0xFF));
    DBG_PRINTF(WebServer, "handle_set_state: RGB='%s'\n", buf);
    controller_.led_strip_set_rgb(buf);
  }
  if (server_.hasArg("brightness")) {
    String brightnessVal = server_.arg("brightness");
    brightnessVal.toCharArray(buf, sizeof(buf));
    DBG_PRINTF(WebServer, "handle_set_state: brightness='%s'\n", buf);
    controller_.led_strip_set_brightness(buf);
  }
  if (server_.hasArg("state")) {
    String stateVal = server_.arg("state");
    stateVal.toCharArray(buf, sizeof(buf));
    DBG_PRINTF(WebServer, "handle_set_state: state='%s'\n", buf);
    controller_.led_strip_set_state(buf);
  }
  if (server_.hasArg("mode_id")) {
    String modeVal = server_.arg("mode_id");
    modeVal.toCharArray(buf, sizeof(buf));
    DBG_PRINTF(WebServer, "handle_set_state: mode_id='%s'\n", buf);
    controller_.led_strip_set_mode(buf);
  }

  // Referer-based branching:
  if (server_.hasHeader("Referer") && !server_.header("Referer").isEmpty()) {
    DBG_PRINTLN(WebServer, "handle_set_state: initial click, serving instructions");
    server_.send_P(200, "text/html", SET_STATE_HTML);
    DBG_PRINTLN(WebServer, "handle_set_state: instruction page sent");
  } else {
    DBG_PRINTLN(WebServer, "handle_set_state: home-screen launch, redirecting to /");
    server_.sendHeader("Location", "/", true); // Add "Location" header
    server_.send(302, "text/plain", "");      // Send 302 redirect
  }
}

void WebServer::handle_get_state() {
  DBG_PRINTF(WebServer, "handle_get_state(): url=%s\n", server_.uri().c_str());
  update_state_payload("full"); // Ensure payload is current
  DBG_PRINTF(WebServer, "handle_get_state: payload='%s'\n", payload_);
  server_.send(200, "text/plain", payload_);
  DBG_PRINTLN(WebServer, "handle_get_state(): response sent");
}

void WebServer::update_state_payload(const char* field) {
  DBG_PRINTF(WebServer, "update_state_payload(): field='%s'\n", field);
  if (strcmp(field, "color") == 0) {
    String c = controller_.led_strip_get_color_hex(); // e.g., "#RRGGBB"
    // Ensure payload is just the hex part for 'C' tag consistency
    payload_len_ = snprintf(payload_, kBufSize, "C%s", c.c_str() + 1); // Skip '#'
    DBG_PRINTF(WebServer, "update_state_payload: color='%s' -> '%s'\n", c.c_str(), payload_);
  } else if (strcmp(field, "brightness") == 0) {
    uint8_t b = controller_.led_strip_get_brightness();
    payload_len_ = snprintf(payload_, kBufSize, "B%u", (unsigned)b);
    DBG_PRINTF(WebServer, "update_state_payload: brightness=%u -> '%s'\n", b, payload_);
  } else if (strcmp(field, "state") == 0) {
    bool s = controller_.led_strip_get_state();
    payload_len_ = snprintf(payload_, kBufSize, "S%u", (unsigned)s);
    DBG_PRINTF(WebServer, "update_state_payload: state=%u -> '%s'\n", (unsigned)s, payload_);
  } else if (strcmp(field, "mode") == 0) {
    uint8_t m = controller_.led_strip_get_mode_id();
    payload_len_ = snprintf(payload_, kBufSize, "M%u", (unsigned)m);
    DBG_PRINTF(WebServer, "update_state_payload: mode=%u -> '%s'\n", m, payload_);
  } else { // "full"
    String c = controller_.led_strip_get_color_hex(); // e.g., "#RRGGBB"
    uint8_t b = controller_.led_strip_get_brightness();
    bool    s = controller_.led_strip_get_state();
    uint8_t m = controller_.led_strip_get_mode_id();
    // Ensure payload format is F<HEX>,b,s,m where HEX has no #
    payload_len_ = snprintf(
      payload_, kBufSize, "F%s,%u,%u,%u",
      c.c_str() + 1, // Skip '#'
      (unsigned)b, (unsigned)s, (unsigned)m
    );
    DBG_PRINTF(WebServer, "update_state_payload: full -> '%s'\n", payload_);
  }

  if (payload_len_ >= kBufSize) {
    payload_len_ = kBufSize - 1; // Ensure null termination if truncated
    DBG_PRINTLN(WebServer, "update_state_payload: payload truncated");
  }
  payload_[payload_len_] = '\0'; // Explicitly null-terminate
}

void WebServer::broadcast_led_state(const char* field) {
  DBG_PRINTF(WebServer, "broadcast_led_state(): field='%s'\n", field);
  update_state_payload(field); // Update the payload_
  // With polling, there's no active broadcast here.
  // The updated payload_ will be picked up by the next client poll to /state.
  DBG_PRINTF(WebServer, "broadcast_led_state: payload updated to '%s' for next poll\n", payload_);
}