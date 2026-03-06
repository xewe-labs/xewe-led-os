// src/Interfaces/WebInterface/WebInterface.cpp

#include "WebInterface.h"
#include "../../../SystemController/SystemController.h"

// required
WebInterface::WebInterface(SystemController& controller)
      : Interface(controller,
               /* module_name         */ "Web_Interface",
               /* module_description  */ "Allows to control LED in web browser from\nany local device",
               /* nvs_key             */ "web",
               /* requires_init_setup */ false,
               /* can_be_disabled     */ true,
               /* has_cli_cmds        */ true)
{}

void WebInterface::sync_color(std::array<uint8_t,3> color) {
    if (is_disabled()) return;
    char payload[8];
    size_t len = snprintf(payload, sizeof(payload), "C%02X%02X%02X", color[0], color[1], color[2]);
    broadcast(payload, len);
}

void WebInterface::sync_brightness(uint8_t brightness) {
    if (is_disabled()) return;
    char payload[6];
    size_t len = snprintf(payload, sizeof(payload), "B%u", (unsigned)brightness);
    broadcast(payload, len);
}

void WebInterface::sync_state(uint8_t state) {
    if (is_disabled()) return;
    char payload[4];
    size_t len = snprintf(payload, sizeof(payload), "S%u", (unsigned)(state ? 1 : 0));
    broadcast(payload, len);
}

void WebInterface::sync_mode(uint8_t mode) {
    if (is_disabled()) return;
    char payload[6];
    size_t len = snprintf(payload, sizeof(payload), "M%u", (unsigned)mode);
    broadcast(payload, len);
}

void WebInterface::sync_length(uint16_t length) {
    if (is_disabled()) return;
    // received new value, propagate it in the module
}

// optional
void WebInterface::sync_all(std::array<uint8_t,3> color,
                   uint8_t brightness,
                   uint8_t state,
                   uint8_t mode,
                   uint16_t length) {

    if (is_disabled()) return;
    char payload[64];
    size_t len = snprintf(payload, sizeof(payload), "F%02X%02X%02X,%u,%u,%u",
        color[0], color[1], color[2],
        (unsigned)brightness,
        (unsigned)(state ? 1 : 0),
        (unsigned)mode
    );
    broadcast(payload, len);
}

void WebInterface::begin_routines_required (const ModuleConfig& cfg) {
    httpServer.on("/",        HTTP_GET, std::bind(&WebInterface::serveMainPage,        this));
    httpServer.on("/set",     HTTP_GET, std::bind(&WebInterface::handleSetRequest,     this));
    httpServer.on("/state",   HTTP_GET, std::bind(&WebInterface::handleGetStateRequest,this));
    httpServer.on("/modes",   HTTP_GET, std::bind(&WebInterface::handleGetModesRequest,this));
    httpServer.on("/name",    HTTP_GET, std::bind(&WebInterface::handleGetNameRequest, this));
}

void WebInterface::begin_routines_regular (const ModuleConfig& cfg) {
    controller.serial_port.print("Web Interface now available for the devices\non the " + controller.wifi.get_ssid() +  " WiFi network\nhttp://" + controller.wifi.get_local_ip());
}

void WebInterface::begin_routines_common (const ModuleConfig& cfg) {
    httpServer.begin();
    webSocket.begin();
    webSocket.onEvent(std::bind(&WebInterface::webSocketEvent, this,
                                std::placeholders::_1, std::placeholders::_2,
                                std::placeholders::_3, std::placeholders::_4));
}

void WebInterface::loop () {
    if (is_disabled()) return;

    httpServer.handleClient();
    webSocket.loop();

    if (connected_clients && (millis() - last_heartbeat_ms >= HEARTBEAT_INTERVAL_MS)) {
        broadcast("H", 1);
        last_heartbeat_ms = millis();
    }
}

void WebInterface::reset (const bool verbose, const bool do_restart, const bool keep_enabled) {
    webSocket.disconnect();
    Module::reset(verbose, do_restart, keep_enabled);
}

std::string WebInterface::status (const bool verbose) const {
    std::ostringstream out;

    unsigned long uptime_s = millis() / 1000UL;
    int days  = static_cast<int>(uptime_s / 86400UL);
    int hours = static_cast<int>((uptime_s % 86400UL) / 3600UL);
    int mins  = static_cast<int>((uptime_s % 3600UL) / 60UL);
    int secs  = static_cast<int>(uptime_s % 60UL);

    uint32_t freeHeap  = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t usedHeap  = totalHeap - freeHeap;
    float heapUsage    = (totalHeap ? (usedHeap * 100.0f) / totalHeap : 0.0f);

    out << "--- Web Server Status ---\n";
    out << "  - Uptime:            "
        << days << " days, "
        << std::setw(2) << std::setfill('0') << hours << ':'
        << std::setw(2) << std::setfill('0') << mins  << ':'
        << std::setw(2) << std::setfill('0') << secs  << '\n';

    out << "  - Memory Usage:      "
        << std::fixed << std::setprecision(2) << heapUsage << "% ("
        << usedHeap << " / " << totalHeap << " bytes)\n";

    out << "  - WebSocket Clients: " << connected_clients << '\n';
    out << "-------------------------";

    if (verbose) controller.serial_port.print(out.str());
    return out.str();
}

// other methods
void WebInterface::serveMainPage() {
    if (is_disabled()) return;

    httpServer.send_P(200, "text/html", INDEX_HTML);
}

void WebInterface::handleSetRequest() {
    if (is_disabled()) return;

    if (httpServer.hasArg("color")) {
        long colorValue = strtol(httpServer.arg("color").c_str(), nullptr, 16);
        uint8_t r = (colorValue >> 16) & 0xFF;
        uint8_t g = (colorValue >> 8) & 0xFF;
        uint8_t b =  colorValue        & 0xFF;
        controller.sync_color({r, g, b}, {true, true, true, true, true});
    } else if (httpServer.hasArg("brightness")) {
        controller.sync_brightness(httpServer.arg("brightness").toInt(), {true, true, true, true, true});
    } else if (httpServer.hasArg("state")) {
        controller.sync_state(httpServer.arg("state").toInt() == 1, {true, true, true, true, true});
    } else if (httpServer.hasArg("mode_id")) {
        controller.sync_mode(httpServer.arg("mode_id").toInt(), {true, true, true, true, true});
    } else if (httpServer.hasArg("param_key") && httpServer.hasArg("param_val")) {
        // --- NEW: DYNAMIC PARAMETER HANDLING ---
        std::string param_key = httpServer.arg("param_key").c_str();
        uint16_t param_val = httpServer.arg("param_val").toInt();

        // Pass the generic string_view and value right to your LedStrip facade
        controller.led_strip.set_mode_param(param_key, param_val);
    }

    httpServer.send(200, "text/plain", "OK");
}

void WebInterface::handleGetStateRequest() {
    if (is_disabled()) return;

    char buffer[64];
    auto rgb = controller.led_strip.get_rgb();
    snprintf(buffer, sizeof(buffer), "F%02X%02X%02X,%u,%u,%u",
        rgb[0], rgb[1], rgb[2],
        (unsigned)controller.led_strip.get_brightness(),
        (unsigned)(controller.led_strip.get_state() ? 1 : 0),
        (unsigned)controller.led_strip.get_current_mode_id()
    );
    httpServer.send(200, "text/plain", buffer);
}

void WebInterface::handleGetModesRequest() {
    if (is_disabled()) return;

    std::string json_payload = controller.led_strip.get_all_modes_json();
    httpServer.send(200, "application/json", json_payload.c_str());
}

void WebInterface::handleGetNameRequest() {
    if (is_disabled()) return;
    httpServer.send(200, "text/plain", controller.system.get_device_name().c_str());
}

void WebInterface::webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t /*length*/) {
    if (is_disabled()) return;

    switch (type) {
        case WStype_DISCONNECTED:
            if (connected_clients > 0) connected_clients--;
            DBG_PRINTF(WebInterface, "[WSc] Client #%u disconnected.\n", num);
            break;
        case WStype_CONNECTED: {
            connected_clients++;
            IPAddress ip = webSocket.remoteIP(num);
            DBG_PRINTF(WebInterface, "[WSc] Client #%u connected from %s.\n", num, ip.toString().c_str());
            sync_all(
                controller.led_strip.get_rgb(),
                controller.led_strip.get_brightness(),
                static_cast<uint8_t>(controller.led_strip.get_state() ? 1 : 0),
                controller.led_strip.get_current_mode_id(),
                0
            );
            break;
        }
        case WStype_TEXT:
            DBG_PRINTF(WebInterface, "[WSc] Received text from #%u: %s\n", num, payload);
            break;
        default: break;
    }
}

void WebInterface::broadcast(const char* payload, size_t length) {
    if (is_disabled()) return;

    if (length > 0) webSocket.broadcastTXT(payload, length);
}


// ------- HTML -------
const char WebInterface::INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
  <title>LED Control</title>
<style>
  :root {
    --bg:#1a1a1a; --fg:#f0f0f0; --accent:#0af; --green:#0f0; --red:#f00; --font:system-ui, sans-serif;
    --radius:12px; --thumb-size:26px; --track-height:12px; --outline:#3a3a3a;
  }
  *, *::before, *::after { box-sizing: border-box; margin:0; padding:0; }
  body { background: var(--bg); color: var(--fg); font-family: var(--font);
         display:flex; flex-direction:column; align-items:center; padding:1rem; min-height:100vh; gap:1.25rem; }
  .panel { width:80vw; display:flex; flex-direction:column; align-items:stretch; gap:1rem; }
  h1 { font-weight: 500; }
  #status { display:flex; align-items:center; gap:.5rem; }
  #status-indicator { width:12px; height:12px; border-radius:50%; background:var(--red); transition:background .5s ease; }

  .controls-grid { display:grid; grid-template-columns:1fr; gap:1rem; width:100%; max-width:none; }
  .control { display:grid; grid-template-columns:1fr; align-items:center; gap:0.2rem; }
  .control-header { display:flex; justify-content:space-between; font-size:0.9rem; color:#cfd2d8; padding-bottom: 0.5rem; }
  select { width:100%; appearance:none; background:transparent; border:1px solid var(--fg);
           border-radius:5px; color:var(--fg); padding:.5rem; }

  .buttons { display:grid; grid-template-columns:repeat(auto-fit, minmax(100px, 1fr)); gap:.5rem; width:100%; max-width:none; }
  button { padding:.75rem; background:var(--accent); border:none; border-radius:5px; color:var(--bg); font-size:1rem; font-weight:500; cursor:pointer; transition:opacity .2s ease; }
  button:disabled { opacity:.4; cursor:not-allowed; }

  /* Range sliders */
  .range-wrap{ position:relative; display:grid; align-items:center; }
  input[type=range].range{ -webkit-appearance:none; appearance:none; width:100%;
    height:var(--thumb-size); background:transparent; margin:0; touch-action:none; border:none; }
  input[type=range].range::-webkit-slider-runnable-track{
    height:var(--track-height); background:var(--track-bg,linear-gradient(90deg,#3b3f52,#3b3f52));
    border-radius:999px; border:1px solid var(--outline); }
  input[type=range].range::-webkit-slider-thumb{
    -webkit-appearance:none; appearance:none; width:var(--thumb-size); height:var(--thumb-size);
    border-radius:50%; border:2px solid rgba(0,0,0,.25);
    background:var(--thumb-bg,#fff); box-shadow:0 4px 10px rgba(0,0,0,.45);
    margin-top:calc((var(--track-height) - var(--thumb-size))/2);
  }
  input[type=range].hue{
    --track-bg:linear-gradient(to right, hsl(0,100%,50%) 0%, hsl(60,100%,50%) 16.6%, hsl(120,100%,45%) 33.3%, hsl(180,100%,45%) 50%, hsl(240,100%,50%) 66.6%, hsl(300,100%,50%) 83.3%, hsl(360,100%,50%) 100%);
  }
</style>
</head>
<body>
  <section class="panel">
    <h1 id="device-title">Loading…</h1>
    <div id="status"><div id="status-indicator"></div><span id="status-text">Offline</span></div>

    <div class="control">
      <select id="mode" aria-label="Mode">
        <option value="0">Loading Modes...</option>
      </select>
    </div>

    <div id="dynamic-controls" class="controls-grid"></div>

    <div class="controls-grid" style="margin-top: 1rem;">
      <div class="control">
        <div class="control-header">
          <span>Brightness</span>
          <span id="brightnessValue">0</span>
        </div>
        <div class="range-wrap">
          <input type="range" id="brightness" class="range brightness" min="0" max="255" step="1" aria-label="Brightness"/>
        </div>
      </div>
    </div>

    <div class="buttons" style="margin-top: 1rem;">
      <button id="btnOn">On</button>
      <button id="btnOff">Off</button>
    </div>
  </section>

  <script>
  "use strict";
  const DEBOUNCE_MS = 200;
  let MODES_DATA = [];
  let ws, reconnectTimer;
  let isOnline = false;
  let reloadTimer = null;
  let STATE = { brightness: 128, mode: 0 };

  const elements = {
    mode: document.getElementById('mode'),
    dynamicControls: document.getElementById('dynamic-controls'),
    brightness: document.getElementById('brightness'),
    brightnessValue: document.getElementById('brightnessValue'),
    btnOn: document.getElementById('btnOn'),
    btnOff: document.getElementById('btnOff'),
    statusIndicator: document.getElementById('status-indicator'),
    statusText: document.getElementById('status-text'),
    deviceTitle: document.getElementById('device-title')
  };

  const debounce = (fn, d) => { let t; return (...a) => { clearTimeout(t); t=setTimeout(()=>fn(...a), d); }; };
  const sendCommand = (k, v) => fetch(`/set?${k}=${encodeURIComponent(v)}`).catch(err => console.error(err));

  const setStatus = (online) => {
    if (isOnline !== online) {
      if (!online) {
        if (!reloadTimer) reloadTimer = setTimeout(() => location.reload(), 1000);
      } else {
        if (reloadTimer) { clearTimeout(reloadTimer); reloadTimer = null; }
      }
      isOnline = online;
    }
    elements.statusIndicator.style.background = online ? 'var(--green)' : 'var(--red)';
    elements.statusText.textContent = online ? 'Online' : 'Offline';
  };

  const updateButtons = (isOn) => { elements.btnOn.disabled = isOn; elements.btnOff.disabled = !isOn; };

  async function loadName(){
    try {
      const res = await fetch('/name', { cache: 'no-store' });
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const txt = (await res.text()).trim();
      elements.deviceTitle.textContent = txt || 'LED Strip Control';
    } catch (e) {
      console.warn('Failed to load name:', e);
      elements.deviceTitle.textContent = 'LED Strip Control';
    }
  }

  // RENDER DYNAMIC SLIDERS
  function renderSlidersForMode(modeId) {
    elements.dynamicControls.innerHTML = '';
    const mode = MODES_DATA.find(m => m.id == modeId);
    if (!mode || !mode.params) return;

    mode.params.forEach(p => {
      const wrapper = document.createElement('div');
      wrapper.className = 'control';

      const isHue = p.key.toLowerCase() === 'hue';
      const inputClass = isHue ? 'range hue' : 'range';

      // Fallbacks added in case JSON generator isn't fully updated yet
      const dispName = p.display_name ?? p.key;
      const minVal = p.min ?? p.min_value ?? 0;
      const maxVal = p.max ?? p.max_value ?? 255;
      const stepVal = p.step ?? p.step_value ?? 1;
      const currentVal = p.value ?? p.default_value ?? 0;

      wrapper.innerHTML = `
        <div class="control-header">
          <span>${dispName}</span>
          <span id="val-${p.key}">${currentVal}</span>
        </div>
        <div class="range-wrap">
          <input type="range" class="${inputClass}" id="param-${p.key}" min="${minVal}" max="${maxVal}" step="${stepVal}" value="${currentVal}" />
        </div>
      `;
      elements.dynamicControls.appendChild(wrapper);

      const slider = document.getElementById(`param-${p.key}`);
      const output = document.getElementById(`val-${p.key}`);

      slider.addEventListener('input', () => {
        output.textContent = slider.value;
        p.value = slider.value; // Store locally so it remembers while tab is open
        sendParamDebounced(p.key, slider.value);
      });
    });
  }

  // Send generic parameter updates to ESP32
  const sendParamDebounced = debounce((key, value) => {
    fetch(`/set?param_key=${encodeURIComponent(key)}&param_val=${encodeURIComponent(value)}`)
      .catch(err => console.error(err));
  }, DEBOUNCE_MS);

  async function loadModes() {
    try {
      const res = await fetch(`/modes`, { cache: 'no-store' });
      MODES_DATA = await res.json();

      elements.mode.innerHTML = "";
      MODES_DATA.forEach(modeObj => {
        const opt = document.createElement('option');
        opt.value = modeObj.id;
        opt.textContent = modeObj.name;
        elements.mode.appendChild(opt);
      });

      // Maintain sync if websocket told us what mode we are on before fetch finished
      if (STATE.mode !== undefined) {
         elements.mode.value = STATE.mode;
      }
      renderSlidersForMode(elements.mode.value);
    } catch (e) {
      console.error("Failed to load modes:", e);
    }
  }

  function connect(){
    if (ws && (ws.readyState === ws.CONNECTING || ws.readyState === ws.OPEN)) return;
    ws = new WebSocket(`ws://${location.hostname}:81/`);

    ws.onopen = () => { setStatus(true); };
    ws.onclose = () => { setStatus(false); clearTimeout(reconnectTimer); reconnectTimer = setTimeout(connect, 5000); };
    ws.onerror = (err) => { console.error('WebSocket error:', err); try { ws.close(); } catch(e) {} };

    ws.onmessage = (e) => {
      const tag = e.data[0], data = e.data.slice(1);
      if (tag === 'H') { setStatus(true); return; }

      switch(tag){
        case 'B': {
          STATE.brightness = parseInt(data,10) || 0;
          elements.brightness.value = STATE.brightness;
          elements.brightnessValue.textContent = STATE.brightness;
        } break;
        case 'S': updateButtons(data === '1'); break;
        case 'M':
          if (STATE.mode != data) {
              STATE.mode = data;
              elements.mode.value = data;
              renderSlidersForMode(data);
          }
          break;
        case 'F': {
          const [hex, bStr, sStr, mStr] = data.split(',');
          STATE.brightness = parseInt(bStr,10)||0;
          elements.brightness.value = STATE.brightness;
          elements.brightnessValue.textContent = STATE.brightness;
          updateButtons(sStr === '1');

          if (STATE.mode != mStr) {
              STATE.mode = mStr;
              elements.mode.value = mStr;
              renderSlidersForMode(mStr);
          }
        } break;
      }
    };
  }

  // --- CORE EVENT LISTENERS ---
  window.addEventListener('load', () => {

    elements.mode.addEventListener('change', () => {
      STATE.mode = elements.mode.value;
      sendCommand('mode_id', STATE.mode);
      renderSlidersForMode(STATE.mode);
    });

    elements.brightness.addEventListener('input', () => {
      elements.brightnessValue.textContent = elements.brightness.value;
    });

    elements.brightness.addEventListener('change', () => {
      sendCommand('brightness', elements.brightness.value);
    });

    elements.btnOn.addEventListener('click', () => { sendCommand('state', '1'); updateButtons(true); });
    elements.btnOff.addEventListener('click', () => { sendCommand('state', '0'); updateButtons(false); });

    loadName();
    loadModes();
    connect();
  });
  </script>
</body>
</html>
)rawliteral";