#include "WebInterface.h"
#include "../../SystemController/SystemController.h"
#include <functional> // For std::bind

// --- Main Control Page (HTML, CSS, JavaScript) ---
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
  <title>LED Control</title>
  <style>
    :root { --bg: #1a1a1a; --fg: #f0f0f0; --accent: #0af; --green: #0f0; --red: #f00; --font: system-ui, sans-serif; }
    *, *::before, *::after { box-sizing: border-box; margin:0; padding:0; }
    body { background: var(--bg); color: var(--fg); font-family: var(--font); display: flex; flex-direction: column; align-items: center; padding: 1rem; min-height: 100vh; gap: 1.25rem; }
    h1 { font-weight: 500; }
    #status { display: flex; align-items: center; gap: 0.5rem; }
    #status-indicator { width: 12px; height: 12px; border-radius: 50%; background: var(--red); transition: background 0.5s ease; }
    .controls-grid { display: grid; grid-template-columns: 1fr; gap: 1rem; width: 100%; max-width: 400px; }
    .control { display: grid; grid-template-columns: 100px 1fr; align-items: center; gap: 1rem; }
    label { font-size: 1rem; }
    input[type="color"], input[type="range"], select { width: 100%; -webkit-appearance: none; appearance: none; background: transparent; border: 1px solid var(--fg); border-radius: 5px; color: var(--fg); }
    input[type="range"] { padding: 0; }
    input[type="color"] { height: 40px; padding: 0.25rem; }
    select { padding: 0.5rem; }
    .buttons { display: grid; grid-template-columns: repeat(auto-fit, minmax(100px, 1fr)); gap: 0.5rem; width: 100%; max-width: 400px; }
    button { padding: .75rem; background: var(--accent); border: none; border-radius: 5px; color: var(--bg); font-size: 1rem; font-weight: 500; cursor: pointer; transition: opacity .2s ease; }
    button:disabled { opacity: .4; cursor: not-allowed; }
  </style>
</head>
<body>
  <h1>LED Strip Control</h1>
  <div id="status"><div id="status-indicator"></div><span id="status-text">Offline</span></div>

  <div class="controls-grid">
    <div class="control"><label for="color">Color</label><input type="color" id="color"/></div>
    <div class="control"><label for="brightness">Brightness</label><input type="range" id="brightness" min="1" max="255"/></div>
    <div class="control"><label for="mode">Mode</label><select id="mode"><option value="0">Color Solid</option></select></div>
  </div>

  <div class="buttons">
    <button id="btnOn">On</button>
    <button id="btnOff">Off</button>
    <button id="btnShortcut">Shortcut</button>
  </div>

  <script>
    const DEBOUNCE_MS = 200;
    const elements = {
      color: document.getElementById('color'), brightness: document.getElementById('brightness'),
      mode: document.getElementById('mode'), btnOn: document.getElementById('btnOn'), btnOff: document.getElementById('btnOff'),
      statusIndicator: document.getElementById('status-indicator'), statusText: document.getElementById('status-text')
    };
    let ws, reconnectTimer;

    const setStatus = (online) => {
      elements.statusIndicator.style.background = online ? 'var(--green)' : 'var(--red)';
      elements.statusText.textContent = online ? 'Online' : 'Offline';
    };

    const updateButtons = (isOn) => { elements.btnOn.disabled = isOn; elements.btnOff.disabled = !isOn; };
    const debounce = (fn, d) => { let t; return (...a) => { clearTimeout(t); t = setTimeout(() => fn(...a), d); }; };

    function connect() {
      if (ws && (ws.readyState === ws.CONNECTING || ws.readyState === ws.OPEN)) return;
      ws = new WebSocket(`ws://${location.hostname}:81/`);
      ws.onopen = () => { setStatus(true); console.log('WebSocket connected.'); };
      ws.onclose = () => { setStatus(false); clearTimeout(reconnectTimer); reconnectTimer = setTimeout(connect, 5000); };
      ws.onerror = (err) => { console.error('WebSocket error:', err); ws.close(); };
      ws.onmessage = (e) => {
        const tag = e.data[0], data = e.data.slice(1);
        switch (tag) {
          case 'C': elements.color.value = '#' + data; break;
          case 'B': elements.brightness.value = parseInt(data, 10); break;
          case 'S': updateButtons(data === '1'); break;
          case 'M': elements.mode.value = data; break;
          case 'F': {
            const [hex, b, s, m] = data.split(',');
            elements.color.value = '#' + hex;
            elements.brightness.value = parseInt(b, 10);
            updateButtons(s === '1');
            elements.mode.value = m;
          } break;
        }
      };
    }

    const sendCommand = (k, v) => fetch(`/set?${k}=${encodeURIComponent(v)}`).catch(err => console.error("Send failed:", err));
    const sendColor = debounce(() => sendCommand('color', elements.color.value.substring(1)), DEBOUNCE_MS);
    const sendBrightness = debounce(() => sendCommand('brightness', elements.brightness.value), DEBOUNCE_MS);

    window.addEventListener('load', () => {
      elements.btnOn.addEventListener('click', () => { sendCommand('state', '1'); updateButtons(true); });
      elements.btnOff.addEventListener('click', () => { sendCommand('state', '0'); updateButtons(false); });
      elements.color.addEventListener('input', sendColor);
      elements.brightness.addEventListener('input', sendBrightness);
      elements.mode.addEventListener('change', () => sendCommand('mode_id', elements.mode.value));
      document.getElementById('btnShortcut').addEventListener('click', () => {
        const params = new URLSearchParams({ color: elements.color.value.substring(1), brightness: elements.brightness.value, state: elements.btnOn.disabled ? '1' : '0', mode_id: elements.mode.value });
        window.location.href = `/set_state?${params.toString()}`;
      });
      connect();
    });
  </script>
</body>
</html>)rawliteral";

static const char SET_STATE_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Save Shortcut</title><style>body{background:#1a1a1a;color:#f0f0f0;font-family:system-ui,sans-serif;display:flex;flex-direction:column;align-items:center;justify-content:center;height:100vh;text-align:center;padding:1rem}h1{margin-bottom:1rem}p{line-height:1.5}</style></head><body><h1>Save This Preset</h1><p>Tap the <strong>Share</strong> button, then <strong>Add to Home Screen</strong> to create a shortcut that applies these settings.</p></body></html>)rawliteral";


// ~~~~~~~~~~~~~~~~~~ WebInterface Class Implementation ~~~~~~~~~~~~~~~~~~

WebInterface::WebInterface(SystemController& controller_ref) : ControllerModule(controller_ref) {}

void WebInterface::begin(void* context, const String& device_name) {
    (void) device_name;
    if (!context) {
        DBG_PRINTLN(WebInterface, "begin(): ERROR - WebServer context is null! Cannot start WebInterface.");
        return;
    }
    // Safely store the WebServer instance provided by the context
    this->webServer = static_cast<WebServer*>(context);

    DBG_PRINTLN(WebInterface, "begin(): Registering routes and starting WebSocket server.");

    // Register HTTP Routes using the stored server pointer
    webServer->on("/", HTTP_GET, std::bind(&WebInterface::serveMainPage, this));
    webServer->on("/set", HTTP_GET, std::bind(&WebInterface::handleSetRequest, this));
    webServer->on("/set_state", HTTP_GET, std::bind(&WebInterface::handleSetStateShortcut, this));
    webServer->on("/state", HTTP_GET, std::bind(&WebInterface::handleGetStateRequest, this));

    // Start WebSocket Server and assign event handler
    webSocket.begin();
    webSocket.onEvent(std::bind(&WebInterface::webSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

    DBG_PRINTLN(WebInterface, "begin(): Web interface setup complete.");
}

void WebInterface::loop() {
    webSocket.loop();
}

void WebInterface::webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            if (connected_clients > 0) connected_clients--;
            DBG_PRINTF(WebInterface, "[WSc] Client #%u disconnected.\n", num);
            break;
        case WStype_CONNECTED: {
            connected_clients++;
            IPAddress ip = webSocket.remoteIP(num);
            DBG_PRINTF(WebInterface, "[WSc] Client #%u connected from %s.\n", num, ip.toString().c_str());
            // Send the current full state to the newly connected client
            sync_all(
                controller.led_strip_get_target_rgb(),
                controller.led_strip_get_target_brightness(),
                controller.led_strip_get_target_state(),
                controller.led_strip_get_target_mode_id(),
                "", // Mode name is not needed for web UI
                0   // Length is not needed for web UI
            );
            break;
        }
        case WStype_TEXT:
            DBG_PRINTF(WebInterface, "[WSc] Received text from #%u: %s\n", num, payload);
            break;
        default:
            break;
    }
}

// --- HTTP Route Handlers ---

void WebInterface::serveMainPage() {
    webServer->send_P(200, "text/html", INDEX_HTML);
}

void WebInterface::handleSetRequest() {
    if (webServer->hasArg("color")) {
        long colorValue = strtol(webServer->arg("color").c_str(), nullptr, 16);
        uint8_t r = (colorValue >> 16) & 0xFF;
        uint8_t g = (colorValue >> 8) & 0xFF;
        uint8_t b = colorValue & 0xFF;
        controller.led_strip_set_rgb({r, g, b}, {true, false, true, true});
    } else if (webServer->hasArg("brightness")) {
        controller.led_strip_set_brightness(webServer->arg("brightness").toInt(), {true, false, true, true});
    } else if (webServer->hasArg("state")) {
        controller.led_strip_set_state(webServer->arg("state").toInt() == 1, {true, false, true, true});
    } else if (webServer->hasArg("mode_id")) {
        controller.led_strip_set_mode(webServer->arg("mode_id").toInt(), {true, false, true, true});
    }
    webServer->send(200, "text/plain", "OK");
}

void WebInterface::handleSetStateShortcut() {
    // This applies the state from the shortcut URL query parameters
    handleSetRequest();

    // Show the "Add to Home Screen" instructions page
    webServer->send_P(200, "text/html", SET_STATE_HTML);
}

void WebInterface::handleGetStateRequest() {
    char buffer[64];
    auto rgb = controller.led_strip_get_target_rgb();
    size_t len = snprintf(buffer, sizeof(buffer), "F%02X%02X%02X,%u,%u,%u",
        rgb[0], rgb[1], rgb[2],
        (unsigned)controller.led_strip_get_target_brightness(),
        (unsigned)controller.led_strip_get_target_state(),
        (unsigned)controller.led_strip_get_target_mode_id()
    );
    webServer->send(200, "text/plain", buffer);
}

// --- Sync Methods (System -> Web UI) ---

void WebInterface::broadcast(const char* payload, size_t length) {
    if (length > 0) {
        webSocket.broadcastTXT(payload, length);
    }
}

void WebInterface::sync_color(std::array<uint8_t, 3> color) {
    char payload[8];
    size_t len = snprintf(payload, sizeof(payload), "C%02X%02X%02X", color[0], color[1], color[2]);
    broadcast(payload, len);
}

void WebInterface::sync_brightness(uint8_t brightness) {
    char payload[5];
    size_t len = snprintf(payload, sizeof(payload), "B%u", brightness);
    broadcast(payload, len);
}

void WebInterface::sync_state(bool state) {
    char payload[3];
    size_t len = snprintf(payload, sizeof(payload), "S%u", state);
    broadcast(payload, len);
}

void WebInterface::sync_mode(uint8_t mode_id, String mode_name) {
    (void)mode_name; // Unused for the web UI
    char payload[5];
    size_t len = snprintf(payload, sizeof(payload), "M%u", mode_id);
    broadcast(payload, len);
}

void WebInterface::sync_length(uint16_t length) {
    // Not used by the web interface.
    (void)length;
}

void WebInterface::sync_all(std::array<uint8_t, 3> color, uint8_t brightness, bool state,
                          uint8_t mode_id, String mode_name, uint16_t length) {
    (void)mode_name; (void)length; // Unused
    char payload[64];
    size_t len = snprintf(payload, sizeof(payload), "F%02X%02X%02X,%u,%u,%u",
        color[0], color[1], color[2],
        (unsigned)brightness,
        (unsigned)state,
        (unsigned)mode_id
    );
    broadcast(payload, len);
    DBG_PRINTF(WebInterface, "[WSc] Broadcasting full state: %s\n", payload);
}

void WebInterface::reset() {
    DBG_PRINTLN(WebInterface, "reset(): Disconnecting all WebSocket clients.");
    webSocket.disconnect();
}

void WebInterface::status () {
    DBG_PRINTLN(WebInterface, "--- Web Server Status ---");

    // --- Uptime ---
    unsigned long uptime_s = (millis()) / 1000;
    int days = uptime_s / 86400;
    int hours = (uptime_s % 86400) / 3600;
    int mins = (uptime_s % 3600) / 60;
    int secs = uptime_s % 60;
    char uptime_buf[25];
    snprintf(uptime_buf, sizeof(uptime_buf), "%d days, %02d:%02d:%02d", days, hours, mins, secs);
    DBG_PRINTF(WebInterface, "  - Uptime:            %s\n", uptime_buf);

    // --- Memory ---
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t usedHeap = totalHeap - freeHeap;
    float heapUsage = (usedHeap * 100.0) / totalHeap;
    DBG_PRINTF(WebInterface, "  - Memory Usage:      %.2f%% (%u / %u bytes)\n", heapUsage, usedHeap, totalHeap);

    // --- Activity ---
    DBG_PRINTF(WebInterface, "  - WebSocket Clients: %u\n", connected_clients);
//    DBG_PRINTF(WebInterface, "  - HTTP Requests:     %lu\n", _httpRequests);


    DBG_PRINTLN(WebInterface, "-------------------------");
}