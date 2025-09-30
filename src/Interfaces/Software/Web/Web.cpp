#include "Web.h"
#include "../../../SystemController/SystemController.h"
#include <functional>

// ------- HTML (unchanged) -------
const char Web::INDEX_HTML[] PROGMEM = R"rawliteral(
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

const char Web::SET_STATE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Save Shortcut</title>
<style>body{background:#1a1a1a;color:#f0f0f0;font-family:system-ui,sans-serif;display:flex;flex-direction:column;align-items:center;justify-content:center;height:100vh;text-align:center;padding:1rem}h1{margin-bottom:1rem}p{line-height:1.5}</style>
</head><body><h1>Save This Preset</h1><p>Tap the <strong>Share</strong> button, then <strong>Add to Home Screen</strong> to create a shortcut that applies these settings.</p></body></html>
)rawliteral";

// ------- Implementation -------
Web::Web(SystemController& controller_ref)
  : Interface(controller_ref, "web", "web", true, true, true) {}

void Web::begin(const ModuleConfig& cfg) {
    const auto& c = static_cast<const WebConfig&>(cfg);
    wifi_enabled_ = c.wifi_enabled;
    device_name   = c.device_name;

    DBG_PRINTLN(Web, "begin(): setting up HTTP + WS servers (owned internally).");

    // Register HTTP routes on our own server
    httpServer.on("/",        HTTP_GET, std::bind(&Web::serveMainPage,        this));
    httpServer.on("/set",     HTTP_GET, std::bind(&Web::handleSetRequest,     this));
    httpServer.on("/set_state", HTTP_GET, std::bind(&Web::handleSetStateShortcut, this));
    httpServer.on("/state",   HTTP_GET, std::bind(&Web::handleGetStateRequest, this));

    Module::begin(cfg);
    // Start servers
    httpServer.begin();   // <-- our own HTTP server
    webSocket.begin();
    webSocket.onEvent(std::bind(&Web::webSocketEvent, this,
                                std::placeholders::_1, std::placeholders::_2,
                                std::placeholders::_3, std::placeholders::_4));

    DBG_PRINTLN(Web, "begin(): Web ready on :80 (HTTP) and :81 (WS).");
}

void Web::loop() {
    httpServer.handleClient();  // <-- make HTTP responsive
    webSocket.loop();           // WS pump
}

void Web::reset(bool /*verbose*/) {
    DBG_PRINTLN(Web, "reset(): Disconnecting all WebSocket clients.");
    webSocket.disconnect();
}

// --- HTTP handlers ---
void Web::serveMainPage() {
    httpServer.send_P(200, "text/html", INDEX_HTML);
}

void Web::handleSetRequest() {
    if (httpServer.hasArg("color")) {
        long colorValue = strtol(httpServer.arg("color").c_str(), nullptr, 16);
        uint8_t r = (colorValue >> 16) & 0xFF;
        uint8_t g = (colorValue >> 8) & 0xFF;
        uint8_t b =  colorValue        & 0xFF;
        controller.sync_color({r, g, b}, {true, true, false, true, true});
    } else if (httpServer.hasArg("brightness")) {
        controller.sync_brightness(httpServer.arg("brightness").toInt(), {true, true, false, true, true});
    } else if (httpServer.hasArg("state")) {
        controller.sync_state(httpServer.arg("state").toInt() == 1, {true, true, false, true, true});
    } else if (httpServer.hasArg("mode_id")) {
        controller.sync_mode(httpServer.arg("mode_id").toInt(), {true, true, false, true, true});
    }
    httpServer.send(200, "text/plain", "OK");
}

void Web::handleSetStateShortcut() {
    handleSetRequest();
    httpServer.send_P(200, "text/html", SET_STATE_HTML);
}

void Web::handleGetStateRequest() {
    char buffer[64];
    auto rgb = controller.led_strip.get_target_rgb();
    snprintf(buffer, sizeof(buffer), "F%02X%02X%02X,%u,%u,%u",
        rgb[0], rgb[1], rgb[2],
        (unsigned)controller.led_strip.get_brightness(),
        (unsigned)(controller.led_strip.get_target_state() ? 1 : 0),
        (unsigned)controller.led_strip.get_target_mode_id()
    );
    httpServer.send(200, "text/plain", buffer);
}

// --- WebSocket handler ---
void Web::webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t /*length*/) {
    switch (type) {
        case WStype_DISCONNECTED:
            if (connected_clients > 0) connected_clients--;
            DBG_PRINTF(Web, "[WSc] Client #%u disconnected.\n", num);
            break;
        case WStype_CONNECTED: {
            connected_clients++;
            IPAddress ip = webSocket.remoteIP(num);
            DBG_PRINTF(Web, "[WSc] Client #%u connected from %s.\n", num, ip.toString().c_str());
            sync_all(
                controller.led_strip.get_target_rgb(),
                controller.led_strip.get_brightness(),
                static_cast<uint8_t>(controller.led_strip.get_target_state() ? 1 : 0),
                controller.led_strip.get_target_mode_id(),
                0
            );
            break;
        }
        case WStype_TEXT:
            DBG_PRINTF(Web, "[WSc] Received text from #%u: %s\n", num, payload);
            break;
        default: break;
    }
}

// --- Broadcast + syncs ---
void Web::broadcast(const char* payload, size_t length) {
    if (length > 0) webSocket.broadcastTXT(payload, length);
}

void Web::sync_color(std::array<uint8_t,3> color) {
    char payload[8];
    size_t len = snprintf(payload, sizeof(payload), "C%02X%02X%02X", color[0], color[1], color[2]);
    broadcast(payload, len);
}

void Web::sync_brightness(uint8_t brightness) {
    char payload[6];
    size_t len = snprintf(payload, sizeof(payload), "B%u", (unsigned)brightness);
    broadcast(payload, len);
}

void Web::sync_state(uint8_t state) {
    char payload[4];
    size_t len = snprintf(payload, sizeof(payload), "S%u", (unsigned)(state ? 1 : 0));
    broadcast(payload, len);
}

void Web::sync_mode(uint8_t mode) {
    char payload[6];
    size_t len = snprintf(payload, sizeof(payload), "M%u", (unsigned)mode);
    broadcast(payload, len);
}

void Web::sync_length(uint16_t /*length*/) {
    // Not used by the web UI
}

void Web::sync_all(std::array<uint8_t,3> color,
                   uint8_t brightness,
                   uint8_t state,
                   uint8_t mode,
                   uint16_t /*length*/) {
    char payload[64];
    size_t len = snprintf(payload, sizeof(payload), "F%02X%02X%02X,%u,%u,%u",
        color[0], color[1], color[2],
        (unsigned)brightness,
        (unsigned)(state ? 1 : 0),
        (unsigned)mode
    );
    broadcast(payload, len);
    DBG_PRINTF(Web, "[WSc] Broadcasting full state: %s\n", payload);
}

void Web::status() {
    DBG_PRINTLN(Web, "--- Web Server Status ---");
    unsigned long uptime_s = (millis()) / 1000;
    int days = uptime_s / 86400;
    int hours = (uptime_s % 86400) / 3600;
    int mins = (uptime_s % 3600) / 60;
    int secs = uptime_s % 60;
    char uptime_buf[25];
    snprintf(uptime_buf, sizeof(uptime_buf), "%d days, %02d:%02d:%02d", days, hours, mins, secs);
    DBG_PRINTF(Web, "  - Uptime:            %s\n", uptime_buf);

    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t usedHeap = totalHeap - freeHeap;
    float heapUsage = (usedHeap * 100.0f) / totalHeap;
    DBG_PRINTF(Web, "  - Memory Usage:      %.2f%% (%u / %u bytes)\n", heapUsage, usedHeap, totalHeap);
    DBG_PRINTF(Web, "  - WebSocket Clients: %u\n", connected_clients);
    DBG_PRINTLN(Web, "-------------------------");
}
