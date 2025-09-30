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
    :root {
      --bg:#1a1a1a; --fg:#f0f0f0; --accent:#0af; --green:#0f0; --red:#f00; --font:system-ui, sans-serif;
      /* slider theming */
      --radius:12px; --thumb-size:26px; --track-height:12px; --outline:#3a3a3a;
    }
    *, *::before, *::after { box-sizing: border-box; margin:0; padding:0; }
    body { background: var(--bg); color: var(--fg); font-family: var(--font);
           display:flex; flex-direction:column; align-items:center; padding:1rem; min-height:100vh; gap:1.25rem; }
    h1 { font-weight: 500; }
    #status { display:flex; align-items:center; gap:.5rem; }
    #status-indicator { width:12px; height:12px; border-radius:50%; background:var(--red); transition:background .5s ease; }
    .controls-grid { display:grid; grid-template-columns:1fr; gap:1rem; width:100%; max-width:420px; }
    .control { display:grid; grid-template-columns:120px 1fr; align-items:center; gap:1rem; }
    label { font-size:1rem; color:#cfd2d8; }
    select { width:100%; appearance:none; background:transparent; border:1px solid var(--fg);
             border-radius:5px; color:var(--fg); padding:.5rem; }
    .buttons { display:grid; grid-template-columns:repeat(auto-fit, minmax(100px, 1fr)); gap:.5rem; width:100%; max-width:420px; }
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
    /* Brightness track is set dynamically: black → current hue color */
    input[type=range].brightness{ /* --track-bg is set in JS */ }
  </style>
</head>
<body>
  <h1>LED Strip Control</h1>
  <div id="status"><div id="status-indicator"></div><span id="status-text">Offline</span></div>

  <div class="controls-grid">
    <!-- REPLACED: Color picker → HSV Hue slider -->
    <div class="control">
      <label for="hue">Hue</label>
      <div class="range-wrap">
        <input type="range" id="hue" class="range hue" min="0" max="255" step="1" />
        <output id="hueValue" class="bubble">0</output>
      </div>
    </div>

    <!-- UPGRADED: Brightness slider appearance -->
    <div class="control">
      <label for="brightness">Brightness</label>
      <div class="range-wrap">
        <input type="range" id="brightness" class="range brightness" min="0" max="255" step="1" />
        <output id="brightnessValue" class="bubble">0</output>
      </div>
    </div>

    <div class="control"><label for="mode">Mode</label>
      <select id="mode"><option value="0">Color Solid</option></select>
    </div>
  </div>

  <div class="buttons">
    <button id="btnOn">On</button>
    <button id="btnOff">Off</button>
    <button id="btnShortcut">Shortcut</button>
  </div>

  <script>
    "use strict";
    const DEBOUNCE_MS = 200;
    const elements = {
      hue: document.getElementById('hue'),
      hueValue: document.getElementById('hueValue'),
      brightness: document.getElementById('brightness'),
      brightnessValue: document.getElementById('brightnessValue'),
      mode: document.getElementById('mode'),
      btnOn: document.getElementById('btnOn'),
      btnOff: document.getElementById('btnOff'),
      statusIndicator: document.getElementById('status-indicator'),
      statusText: document.getElementById('status-text')
    };
    let ws, reconnectTimer;
    const STATE = { hue: 0, brightness: 128 };

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
      elements.statusIndicator.style.background = online ? 'var(--green)' : 'var(--red)';
      elements.statusText.textContent = online ? 'Online' : 'Offline';
    };
    const updateButtons = (isOn) => { elements.btnOn.disabled = isOn; elements.btnOff.disabled = !isOn; };
    const debounce = (fn, d) => { let t; return (...a) => { clearTimeout(t); t=setTimeout(()=>fn(...a), d); }; };
    // Replace the existing setBrightnessTrack with this version:
    function setBrightnessTrack(h255){
      const MIN_V = 8; // very dim (not black)
      const [r0,g0,b0] = hsvToRgb255(h255, 255, MIN_V);
      const [r1,g1,b1] = hsvToRgb255(h255, 255, 255);
      elements.brightness.style.setProperty(
        "--track-bg",
        `linear-gradient(to right, rgb(${r0}, ${g0}, ${b0}), rgb(${r1}, ${g1}, ${b1}))`
      );
    }
    function setHueThumb(h255){
      const [r,g,b] = hsvToRgb255(h255,255,255);
      elements.hue.style.setProperty("--thumb-bg",
        `radial-gradient(circle at 35% 35%, rgba(255,255,255,.9), rgba(255,255,255,.1)), rgb(${r}, ${g}, ${b})`);
    }

    // --- Networking (same endpoints) ---
    function connect(){
      if (ws && (ws.readyState === ws.CONNECTING || ws.readyState === ws.OPEN)) return;
      ws = new WebSocket(`ws://${location.hostname}:81/`);
      ws.onopen = () => { setStatus(true); };
      ws.onclose = () => { setStatus(false); clearTimeout(reconnectTimer); reconnectTimer = setTimeout(connect, 5000); };
      ws.onerror = (err) => { console.error('WebSocket error:', err); try{ws.close();}catch(e){} };
      ws.onmessage = (e) => {
        const tag = e.data[0], data = e.data.slice(1);
        switch(tag){
          case 'C': { // RGB hex → update hue track
            const [r,g,b] = hexToRgb(data);
            const [h] = rgbToHsv255(r,g,b);
            STATE.hue = h;
            elements.hue.value = String(h);
            elements.hueValue.value = h;
            setBrightnessTrack(h);
            setHueThumb(h);
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
            const [hex, b, s, m] = data.split(',');
            const [r,g,bb] = hexToRgb(hex);
            const [h] = rgbToHsv255(r,g,bb);
            STATE.hue = h;
            STATE.brightness = clamp255(parseInt(b,10)||0);
            elements.hue.value = String(h);
            elements.hueValue.value = h;
            elements.brightness.value = String(STATE.brightness);
            elements.brightnessValue.value = STATE.brightness;
            updateButtons(s === '1');
            elements.mode.value = m;
            setBrightnessTrack(h);
            setHueThumb(h);
          } break;
        }
      };
    }
    const sendCommand = (k, v) => fetch(`/set?${k}=${encodeURIComponent(v)}`).catch(err => console.error("Send failed:", err));

    // hue → send RGB (S=255, V=255) to keep protocol RGB-only
    const sendHue = debounce(() => {
      const [r,g,b] = hsvToRgb255(STATE.hue, 255, 255);
      const hex = rgbToHex(r,g,b);
      sendCommand('color', hex);
    }, DEBOUNCE_MS);
    const sendBrightness = debounce(() => sendCommand('brightness', elements.brightness.value), DEBOUNCE_MS);

    // --- Wire up UI (unchanged where possible) ---
    window.addEventListener('load', () => {
      elements.btnOn.addEventListener('click', () => { sendCommand('state', '1'); updateButtons(true); });
      elements.btnOff.addEventListener('click', () => { sendCommand('state', '0'); updateButtons(false); });

      elements.hue.addEventListener('input', () => {
        const v = clamp255(parseInt(elements.hue.value,10)||0);
        STATE.hue = v;
        elements.hueValue.value = v;
        setBrightnessTrack(v);
        setHueThumb(v);
        sendHue();
      });

      elements.brightness.addEventListener('input', () => {
        const v = clamp255(parseInt(elements.brightness.value,10)||0);
        STATE.brightness = v;
        elements.brightnessValue.value = v;
        sendBrightness();
      });

      elements.mode.addEventListener('change', () => sendCommand('mode_id', elements.mode.value));

      document.getElementById('btnShortcut').addEventListener('click', () => {
        const [r,g,b] = hsvToRgb255(STATE.hue, 255, 255);
        const hex = rgbToHex(r,g,b);
        const params = new URLSearchParams({
          color: hex,
          brightness: elements.brightness.value,
          state: elements.btnOn.disabled ? '1' : '0',
          mode_id: elements.mode.value
        });
        window.location.href = `/set_state?${params.toString()}`;
      });

      // initial visuals
      elements.hue.value = String(STATE.hue);
      elements.hueValue.value = STATE.hue;
      elements.brightness.value = String(STATE.brightness);
      elements.brightnessValue.value = STATE.brightness;
      setBrightnessTrack(STATE.hue);
      setHueThumb(STATE.hue);

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
