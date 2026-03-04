/*********************************************************************************
 * SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 * Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 * See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 * Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 * https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/



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
// make sure they have
// if (is_disabled()) return;
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
        (unsigned)controller.led_strip.get_mode_id()
    );
    httpServer.send(200, "text/plain", buffer);
}

void WebInterface::handleGetModesRequest() {
    if (is_disabled()) return;

    std::string modes_json = controller.led_strip.get_all_modes();
    httpServer.send(200, "application/json", modes_json.c_str());
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
                controller.led_strip.get_mode_id(),
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
    /* slider theming */
    --radius:12px; --thumb-size:26px; --track-height:12px; --outline:#3a3a3a;
  }
  *, *::before, *::after { box-sizing: border-box; margin:0; padding:0; }
  body { background: var(--bg); color: var(--fg); font-family: var(--font);
         display:flex; flex-direction:column; align-items:center; padding:1rem; min-height:100vh; gap:1.25rem; }
  /* 80vw panel wrapper */
  .panel { width:80vw; display:flex; flex-direction:column; align-items:stretch; gap:1rem; }
  h1 { font-weight: 500; }
  #status { display:flex; align-items:center; gap:.5rem; }
  #status-indicator { width:12px; height:12px; border-radius:50%; background:var(--red); transition:background .5s ease; }

  .controls-grid { display:grid; grid-template-columns:1fr; gap:1rem; width:100%; max-width:none; }
  .control { display:grid; grid-template-columns:1fr; align-items:center; gap:1rem; }
  label { font-size:1rem; color:#cfd2d8; }
  select { width:100%; appearance:none; background:transparent; border:1px solid var(--fg);
           border-radius:5px; color:var(--fg); padding:.5rem; }

  .buttons { display:grid; grid-template-columns:repeat(auto-fit, minmax(100px, 1fr)); gap:.5rem; width:100%; max-width:none; }
  button { padding:.75rem; background:var(--accent); border:none; border-radius:5px; color:var(--bg); font-size:1rem; font-weight:500; cursor:pointer; transition:opacity .2s ease; }
  button:disabled { opacity:.4; cursor:not-allowed; }

  /* === Fancy range sliders (from reference style) === */
  .range-wrap{ position:relative; display:grid; align-items:center; }
  .bubble{ position:absolute; right:0; top:-22px; font-size:.8rem; color:#b7bdc9; pointer-events:none; }
  input[type=range].range{ -webkit-appearance:none; appearance:none; width:100%;
    height:var(--thumb-size); background:transparent; margin:6px 0; touch-action:none; border:none; }
  input[type=range].range::-webkit-slider-runnable-track{
    height:var(--track-height); background:var(--track-bg,linear-gradient(90deg,#3b3f52,#3b3f52));
    border-radius:999px; border:1px solid var(--outline); }
  input[type=range].range::-webkit-slider-thumb{
    -webkit-appearance:none; appearance:none; width:var(--thumb-size); height:var(--thumb-size);
    border-radius:50%; border:2px solid rgba(0,0,0,.25);
    background:var(--thumb-bg,#fff); box-shadow:0 4px 10px rgba(0,0,0,.45);
    margin-top:calc((var(--track-height) - var(--thumb-size))/2);
  }
  input[type=range].range::-moz-range-track{
    height:var(--track-height); background:var(--track-bg,linear-gradient(90deg,#3b3f52,#3b3f52));
    border-radius:999px; border:1px solid var(--outline); }
  input[type=range].range::-moz-range-thumb{
    width:var(--thumb-size); height:var(--thumb-size); border-radius:50%;
    border:2px solid rgba(0,0,0,.25); background:var(--thumb-bg,#fff); box-shadow:0 4px 10px rgba(0,0,0,.45);
  }
  /* Hue rainbow track */
  input[type=range].hue{
    --track-bg:linear-gradient(to right,
      hsl(0,100%,50%) 0%,
      hsl(60,100%,50%) 16.6%,
      hsl(120,100%,45%) 33.3%,
      hsl(180,100%,45%) 50%,
      hsl(240,100%,50%) 66.6%,
      hsl(300,100%,50%) 83.3%,
      hsl(360,100%,50%) 100%);
  }
  /* Sat & Brightness track dynamically updated in JS */
  input[type=range].sat{ }
  input[type=range].brightness{ }
</style>

</head>
<body>
  <section class="panel">
    <h1 id="device-title">Loading…</h1>
    <div id="status"><div id="status-indicator"></div><span id="status-text">Offline</span></div>

    <div class="controls-grid">
      <div class="control">
        <div class="range-wrap">
          <input type="range" id="hue" class="range hue" min="0" max="255" step="1" aria-label="Hue"/>
          <output id="hueValue" class="bubble">0</output>
        </div>
      </div>

      <div class="control">
        <div class="range-wrap">
          <input type="range" id="sat" class="range sat" min="0" max="255" step="1" aria-label="Saturation"/>
          <output id="satValue" class="bubble">255</output>
        </div>
      </div>

      <div class="control">
        <div class="range-wrap">
          <input type="range" id="brightness" class="range brightness" min="0" max="255" step="1" aria-label="Brightness"/>
          <output id="brightnessValue" class="bubble">0</output>
        </div>
      </div>

      <div class="control">
        <select id="mode" aria-label="Mode">
          <option value="0">Color Solid</option>
        </select>
      </div>
    </div>

    <div class="buttons">
      <button id="btnOn">On</button>
      <button id="btnOff">Off</button>
    </div>
  </section>


  <script>
  "use strict";
  const DEBOUNCE_MS = 200;
  const elements = {
    hue: document.getElementById('hue'),
    hueValue: document.getElementById('hueValue'),
    sat: document.getElementById('sat'),
    satValue: document.getElementById('satValue'),
    brightness: document.getElementById('brightness'),
    brightnessValue: document.getElementById('brightnessValue'),
    mode: document.getElementById('mode'),
    btnOn: document.getElementById('btnOn'),
    btnOff: document.getElementById('btnOff'),
    statusIndicator: document.getElementById('status-indicator'),
    statusText: document.getElementById('status-text'),
    deviceTitle: document.getElementById('device-title')
  };

  let ws, reconnectTimer;
  const STATE = { hue: 0, sat: 255, brightness: 128 };
  let isOnline = false;
  let reloadTimer = null;

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

  // --- Heartbeat watchdog (2.2s timeout) ---
  const HEARTBEAT_TIMEOUT_MS = 2200;
  let lastHeartbeat = 0;
  setInterval(() => {
    if (Date.now() - lastHeartbeat > HEARTBEAT_TIMEOUT_MS) {
      setStatus(false);
    }
  }, 500);

  // --- Helpers (HSV/RGB + styling) ---
  const clamp255 = (x) => Math.max(0, Math.min(255, x|0));

  function hsvToRgb255(h255, s255, v255){
    const h = ((h255 % 256)/255)*360, s = clamp255(s255)/255, v = clamp255(v255)/255;
    if (s <= 0){ const c=(v*255)|0; return [c,c,c]; }
    const i=Math.floor(h/60)%6, f=h/60 - Math.floor(h/60);
    const p=v*(1-s), q=v*(1-f*s), t=v*(1-(1-f)*s);
    let r,g,b;
    switch(i){case 0:r=v;g=t;b=p;break;case 1:r=q;g=v;b=p;break;case 2:r=p;g=v;b=t;break;case 3:r=p;g=q;b=v;break;case 4:r=t;g=p;b=v;break;default:r=v;g=p;b=q;}
    return [clamp255(Math.round(r*255)), clamp255(Math.round(g*255)), clamp255(Math.round(b*255))];
  }

  function rgbToHsv255(r,g,b){
    const rf=r/255,gf=g/255,bf=b/255; const max=Math.max(rf,gf,bf), min=Math.min(rf,gf,bf), d=max-min;
    let h=0, s=max===0?0:d/max, v=max;
    if (d!==0){
      switch(max){
        case rf: h=((gf-bf)/d + (gf<bf?6:0)); break;
        case gf: h=((bf-rf)/d + 2); break;
        default: h=((rf-gf)/d + 4);
      }
      h*=60;
    }
    return [clamp255(Math.round(h/360*255)), clamp255(Math.round(s*255)), clamp255(Math.round(v*255))];
  }

  const rgbToHex = (r,g,b) => [r,g,b].map(x=>x.toString(16).padStart(2,"0")).join("").toUpperCase();
  const hexToRgb = (hex)=>[ parseInt(hex.slice(0,2),16), parseInt(hex.slice(2,4),16), parseInt(hex.slice(4,6),16) ];

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
  const debounce = (fn, d) => { let t; return (...a) => { clearTimeout(t); t=setTimeout(()=>fn(...a), d); }; };

  function updateVisuals() {
    const h = STATE.hue, s = STATE.sat;

    // Saturation track: white to full hue
    const [rF, gF, bF] = hsvToRgb255(h, 255, 255);
    elements.sat.style.setProperty("--track-bg", `linear-gradient(to right, #ffffff, rgb(${rF}, ${gF}, ${bF}))`);

    // Brightness track: dim color to full color
    const MIN_V = 8;
    const [r0, g0, b0] = hsvToRgb255(h, s, MIN_V);
    const [r1, g1, b1] = hsvToRgb255(h, s, 255);
    elements.brightness.style.setProperty("--track-bg", `linear-gradient(to right, rgb(${r0}, ${g0}, ${b0}), rgb(${r1}, ${g1}, ${b1}))`);

    // Thumbs
    const [rT, gT, bT] = hsvToRgb255(h, s, 255);
    const thumbBg = `radial-gradient(circle at 35% 35%, rgba(255,255,255,.9), rgba(255,255,255,.1)), rgb(${rT}, ${gT}, ${bT})`;
    elements.hue.style.setProperty("--thumb-bg", thumbBg);
    elements.sat.style.setProperty("--thumb-bg", thumbBg);
  }

  // --- Modes: fetch from server and populate select ---
  async function loadModes(){
    try {
      const res = await fetch(`/modes`, { cache: 'no-store' });
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const modes = await res.json();
      elements.mode.innerHTML = "";
      modes.forEach(modeObj => {
        const opt = document.createElement('option');
        opt.value = modeObj.id;
        opt.textContent = modeObj.name || `Mode ${modeObj.id}`;
        elements.mode.appendChild(opt);
      });
    } catch (e) {
      console.error("Failed to load modes:", e);
    }
  }

  // --- Networking (same endpoints) ---
  function connect(){
    if (ws && (ws.readyState === ws.CONNECTING || ws.readyState === ws.OPEN)) return;
    ws = new WebSocket(`ws://${location.hostname}:81/`);

    ws.onopen = () => {
      lastHeartbeat = Date.now();
      setStatus(true);
    };

    ws.onclose = () => {
      setStatus(false);
      clearTimeout(reconnectTimer);
      reconnectTimer = setTimeout(connect, 5000);
    };

    ws.onerror = (err) => {
      console.error('WebSocket error:', err);
      try { ws.close(); } catch(e) {}
    };

    ws.onmessage = (e) => {
      const tag = e.data[0], data = e.data.slice(1);

      if (tag === 'H') {
        lastHeartbeat = Date.now();
        setStatus(true);
        return;
      }

      switch(tag){
        case 'C': {
          const [r,g,b] = hexToRgb(data);
          let [h, s] = rgbToHsv255(r,g,b);
          if (h === 0 && STATE.hue === 255) h = 255; // Prevent hue slider jumping from max to min on red
          if (s > 0) STATE.hue = h; // Keep previous hue if color is completely desaturated
          STATE.sat = s;
          elements.hue.value = String(STATE.hue);
          elements.hueValue.value = STATE.hue;
          elements.sat.value = String(STATE.sat);
          elements.satValue.value = STATE.sat;
          updateVisuals();
        } break;
        case 'B': {
          const v = clamp255(parseInt(data,10) || 0);
          STATE.brightness = v;
          elements.brightness.value = String(v);
          elements.brightnessValue.value = v;
        } break;
        case 'S': updateButtons(data === '1'); break;
        case 'M': elements.mode.value = data; break;
        case 'F': {
          const [hex, bStr, sStr, mStr] = data.split(',');
          const [r,g,bb] = hexToRgb(hex);
          let [h, s] = rgbToHsv255(r,g,bb);
          if (h === 0 && STATE.hue === 255) h = 255; // Prevent hue slider jumping from max to min on red
          if (s > 0) STATE.hue = h; // Keep previous hue if color is completely desaturated
          STATE.sat = s;
          STATE.brightness = clamp255(parseInt(bStr,10)||0);
          elements.hue.value = String(STATE.hue);
          elements.hueValue.value = STATE.hue;
          elements.sat.value = String(STATE.sat);
          elements.satValue.value = STATE.sat;
          elements.brightness.value = String(STATE.brightness);
          elements.brightnessValue.value = STATE.brightness;
          updateButtons(sStr === '1');
          elements.mode.value = mStr;
          updateVisuals();
        } break;
      }
    };
  }

  const sendCommand = (k, v) => fetch(`/set?${k}=${encodeURIComponent(v)}`).catch(err => console.error("Send failed:", err));

  const sendColor = debounce(() => {
    const [r,g,b] = hsvToRgb255(STATE.hue, STATE.sat, 255);
    const hex = rgbToHex(r,g,b);
    sendCommand('color', hex);
  }, DEBOUNCE_MS);

  const sendBrightness = debounce(() => sendCommand('brightness', elements.brightness.value), DEBOUNCE_MS);

  // --- Wire up UI ---
  window.addEventListener('load', () => {
    elements.btnOn.addEventListener('click', () => { sendCommand('state', '1'); updateButtons(true); });
    elements.btnOff.addEventListener('click', () => { sendCommand('state', '0'); updateButtons(false); });

    elements.hue.addEventListener('input', () => {
      const v = clamp255(parseInt(elements.hue.value,10)||0);
      STATE.hue = v;
      elements.hueValue.value = v;
      updateVisuals();
      sendColor();
    });

    elements.sat.addEventListener('input', () => {
      const v = clamp255(parseInt(elements.sat.value,10)||0);
      STATE.sat = v;
      elements.satValue.value = v;
      updateVisuals();
      sendColor();
    });

    elements.brightness.addEventListener('input', () => {
      const v = clamp255(parseInt(elements.brightness.value,10)||0);
      STATE.brightness = v;
      elements.brightnessValue.value = v;
      sendBrightness();
    });

    elements.mode.addEventListener('change', () => sendCommand('mode_id', elements.mode.value));

    // initial visuals
    elements.hue.value = String(STATE.hue);
    elements.hueValue.value = STATE.hue;
    elements.sat.value = String(STATE.sat);
    elements.satValue.value = STATE.sat;
    elements.brightness.value = String(STATE.brightness);
    elements.brightnessValue.value = STATE.brightness;
    updateVisuals();

    loadName();
    loadModes();
    connect();
  });
</script>
</body>
</html>)rawliteral";