// Software/Web/Web.cpp
#include "Web.h"
#include "../../../SystemController/SystemController.h"

#if !defined(ESP32)
  #error "This Web interface builds only for ESP32."
#endif

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <array>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace {

// ------------------------- server and globals -------------------------
AsyncWebServer server(80);
AsyncEventSource events("/events");   // SSE for pushing patches to all clients
Web* g_web = nullptr;

// Thread-safe cache of current state exposed to UI (canonical: RGB + brightness 0..255)
struct Cache {
    std::array<uint8_t,3> rgb {0,0,0};  // color (canonical)
    uint8_t brightness_255 = 0;         // 0..255 (canonical)
    bool    power          = false;
    uint8_t mode_id        = 0;         // internal mode id
} cache;

SemaphoreHandle_t cache_mutex = nullptr;
inline void lock()   { if (cache_mutex) xSemaphoreTake(cache_mutex, portMAX_DELAY); }
inline void unlock() { if (cache_mutex) xSemaphoreGive(cache_mutex); }

// ------------------------- small utils -------------------------
static inline uint8_t clamp_u8(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return static_cast<uint8_t>(v);
}
static inline uint16_t clamp_u16(int v, uint16_t maxv=65535) {
    if (v < 0) return 0;
    if (v > maxv) return maxv;
    return static_cast<uint16_t>(v);
}

// (Kept for debugging/utility; conversions now happen on the client)
std::array<uint8_t,3> hsv_to_rgb_deg(uint16_t hue_deg, uint8_t s, uint8_t v) {
    DBG_PRINTF(Web, "-> hsv_to_rgb_deg(hue=%u, s=%u, v=%u)\n", hue_deg, s, v);
    hue_deg %= 360;
    float h = hue_deg / 60.0f;
    int   i = int(floorf(h));
    float f = h - i;

    float sf = s / 255.0f;
    float vf = v / 255.0f;

    float p = vf * (1.0f - sf);
    float q = vf * (1.0f - sf * f);
    float t = vf * (1.0f - sf * (1.0f - f));

    float rf=0, gf=0, bf=0;
    switch (i) {
        case 0: rf=vf; gf=t;  bf=p;  break;
        case 1: rf=q;  gf=vf; bf=p;  break;
        case 2: rf=p;  gf=vf; bf=t;  break;
        case 3: rf=p;  gf=q;  bf=vf; break;
        case 4: rf=t;  gf=p;  bf=vf; break;
        default: rf=vf; gf=p;  bf=q;  break; // case 5
    }
    std::array<uint8_t,3> out = { uint8_t(rf*255 + 0.5f), uint8_t(gf*255 + 0.5f), uint8_t(bf*255 + 0.5f) };
    DBG_PRINTF(Web, "<- hsv_to_rgb_deg() -> rgb={%u,%u,%u}\n", out[0], out[1], out[2]);
    return out;
}

uint16_t rgb_to_hue_deg(std::array<uint8_t,3> rgb) {
    DBG_PRINTF(Web, "-> rgb_to_hue_deg(rgb={%u,%u,%u})\n", rgb[0], rgb[1], rgb[2]);
    float r = rgb[0]/255.0f, g = rgb[1]/255.0f, b = rgb[2]/255.0f;
    float maxc = std::max({r,g,b});
    float minc = std::min({r,g,b});
    float d = maxc - minc;
    if (d == 0.0f) {
        DBG_PRINTLN(Web, "<- rgb_to_hue_deg() -> 0 (gray)");
        return 0;
    }
    float h;
    if (maxc == r)      h = fmodf((g - b) / d, 6.0f);
    else if (maxc == g) h = ((b - r) / d) + 2.0f;
    else                h = ((r - g) / d) + 4.0f;
    h *= 60.0f;
    if (h < 0) h += 360.0f;
    uint16_t out = uint16_t(h + 0.5f);
    DBG_PRINTF(Web, "<- rgb_to_hue_deg() -> %u\n", out);
    return out;
}

// Replace all occurrences of `from` with `to` in s
void replace_all(std::string& s, const std::string& from, const std::string& to) {
    DBG_PRINTF(Web, "replace_all('%s' -> '%s') begin\n", from.c_str(), to.c_str());
    if (from.empty()) { DBG_PRINTLN(Web, "replace_all() skipped: from empty"); return; }
    size_t pos = 0, count = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
        ++count;
    }
    DBG_PRINTF(Web, "replace_all() done, replacements=%u\n", (unsigned)count);
}

// Trim/lowercase helper
std::string lc(std::string s) {
    DBG_PRINTF(Web, "lc() input='%s'\n", s.c_str());
    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c){return std::isspace(c);}), s.end());
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return char(std::tolower(c)); });
    DBG_PRINTF(Web, "lc() output='%s'\n", s.c_str());
    return s;
}

// Extract number: {"brightness":123}
bool extract_number(const std::string& body, const std::string& key, int& out) {
    DBG_PRINTF(Web, "extract_number(key='%s') body_len=%u\n", key.c_str(), (unsigned)body.size());
    const std::string pat = "\"" + key + "\"";
    size_t k = body.find(pat);
    if (k == std::string::npos) { DBG_PRINTLN(Web, "extract_number: key not found"); return false; }
    size_t colon = body.find(':', k + pat.size());
    if (colon == std::string::npos) { DBG_PRINTLN(Web, "extract_number: colon not found"); return false; }
    size_t i = colon + 1;
    while (i < body.size() && std::isspace((unsigned char)body[i])) ++i;
    bool neg = false;
    if (i < body.size() && (body[i] == '-' || body[i] == '+')) { neg = (body[i]=='-'); ++i; }
    long val = 0;
    bool any = false;
    while (i < body.size() && std::isdigit((unsigned char)body[i])) { any = true; val = val*10 + (body[i]-'0'); ++i; }
    if (!any) { DBG_PRINTLN(Web, "extract_number: no digits"); return false; }
    out = neg ? -int(val) : int(val);
    DBG_PRINTF(Web, "extract_number: parsed %d\n", out);
    return true;
}

// Extract string: {"mode":"solid"}
bool extract_string(const std::string& body, const std::string& key, std::string& out) {
    DBG_PRINTF(Web, "extract_string(key='%s') body_len=%u\n", key.c_str(), (unsigned)body.size());
    const std::string pat = "\"" + key + "\"";
    size_t k = body.find(pat);
    if (k == std::string::npos) { DBG_PRINTLN(Web, "extract_string: key not found"); return false; }
    size_t colon = body.find(':', k + pat.size());
    if (colon == std::string::npos) { DBG_PRINTLN(Web, "extract_string: colon not found"); return false; }
    size_t i = colon + 1;
    while (i < body.size() && std::isspace((unsigned char)body[i])) ++i;
    if (i >= body.size() || body[i] != '\"') { DBG_PRINTLN(Web, "extract_string: not a string"); return false; }
    ++i;
    std::string val;
    while (i < body.size() && body[i] != '\"') { val.push_back(body[i++]); }
    if (i >= body.size()) { DBG_PRINTLN(Web, "extract_string: unterminated"); return false; }
    out = val;
    DBG_PRINTF(Web, "extract_string: parsed '%s'\n", out.c_str());
    return true;
}

// Extract bool: {"power":true}
bool extract_bool(const std::string& body, const std::string& key, bool& out) {
    DBG_PRINTF(Web, "extract_bool(key='%s') body_len=%u\n", key.c_str(), (unsigned)body.size());
    const std::string pat = "\"" + key + "\"";
    size_t k = body.find(pat);
    if (k == std::string::npos) { DBG_PRINTLN(Web, "extract_bool: key not found"); return false; }
    size_t colon = body.find(':', k + pat.size());
    if (colon == std::string::npos) { DBG_PRINTLN(Web, "extract_bool: colon not found"); return false; }
    size_t i = colon + 1;
    while (i < body.size() && std::isspace((unsigned char)body[i])) ++i;
    if (body.compare(i, 4, "true") == 0)  { out = true;  DBG_PRINTLN(Web, "extract_bool: true");  return true; }
    if (body.compare(i, 5, "false") == 0) { out = false; DBG_PRINTLN(Web, "extract_bool: false"); return true; }
    DBG_PRINTLN(Web, "extract_bool: invalid literal");
    return false;
}

// Extract RGB triplet: {"rgb":[r,g,b]}
bool extract_rgb_array(const std::string& body, const std::string& key, std::array<uint8_t,3>& out) {
    DBG_PRINTF(Web, "extract_rgb_array(key='%s') body_len=%u\n", key.c_str(), (unsigned)body.size());
    const std::string pat = "\"" + key + "\"";
    size_t k = body.find(pat);
    if (k == std::string::npos) { DBG_PRINTLN(Web, "extract_rgb_array: key not found"); return false; }
    size_t colon = body.find(':', k + pat.size());
    if (colon == std::string::npos) { DBG_PRINTLN(Web, "extract_rgb_array: colon not found"); return false; }
    size_t i = colon + 1;
    while (i < body.size() && std::isspace((unsigned char)body[i])) ++i;
    if (i >= body.size() || body[i] != '[') { DBG_PRINTLN(Web, "extract_rgb_array: '[' not found"); return false; }
    ++i;

    int vals[3] = {0,0,0};
    int n = 0;

    while (i < body.size() && n < 3) {
        while (i < body.size() && std::isspace((unsigned char)body[i])) ++i;
        if (i >= body.size()) break;

        bool any=false; long v=0; bool neg=false;
        if (body[i]=='-'||body[i]=='+'){neg=(body[i]=='-'); ++i;}
        while (i < body.size() && std::isdigit((unsigned char)body[i])) { any=true; v = v*10 + (body[i]-'0'); ++i; }
        if (!any) { DBG_PRINTLN(Web, "extract_rgb_array: missing number"); return false; }
        vals[n++] = neg ? -int(v) : int(v);

        while (i < body.size() && std::isspace((unsigned char)body[i])) ++i;
        if (i < body.size() && body[i] == ',') { ++i; continue; }
        if (i < body.size() && body[i] == ']') { ++i; break; }
    }

    if (n != 3) { DBG_PRINTF(Web, "extract_rgb_array: expected 3 numbers, got %d\n", n); return false; }

    out = { clamp_u8(vals[0]), clamp_u8(vals[1]), clamp_u8(vals[2]) };
    DBG_PRINTF(Web, "extract_rgb_array: parsed rgb={%u,%u,%u}\n", out[0], out[1], out[2]);
    return true;
}

// -------------- JSON helpers for pushing patches/state --------------
std::string mode_to_string(uint8_t id) {
    switch (id) {
        case 0: default:
            DBG_PRINTF(Web, "mode_to_string(%u) -> 'solid'\n", id);
            return "solid";
    }
}
std::string json_quote(const std::string& s) {
    return std::string("\"") + s + "\"";
}
void push_patch(const std::string& json) {
    DBG_PRINTF(Web, "push_patch(len=%u): %s\n", (unsigned)json.size(), json.c_str());
    events.send(json.c_str(), "patch");
}
std::string make_color_patch_json(const std::array<uint8_t,3>& rgb) {
    // Conversions happen on client; only send canonical rgb
    std::string js = std::string("{\"rgb\":[")
         + std::to_string(rgb[0]) + "," + std::to_string(rgb[1]) + "," + std::to_string(rgb[2]) + "]}";
    DBG_PRINTF(Web, "make_color_patch_json -> %s\n", js.c_str());
    return js;
}
std::string make_brightness_patch_json(uint8_t b255) {
    // Conversions happen on client; send canonical 0..255
    std::string js = std::string("{\"brightness\":") + std::to_string(b255) + "}";
    DBG_PRINTF(Web, "make_brightness_patch_json -> %s\n", js.c_str());
    return js;
}
std::string make_power_patch_json(bool pwr) {
    std::string js = std::string("{\"power\":") + (pwr ? "true" : "false") + "}";
    DBG_PRINTF(Web, "make_power_patch_json -> %s\n", js.c_str());
    return js;
}
std::string make_mode_patch_json(uint8_t id) {
    std::string js = std::string("{\"mode\":") + json_quote(mode_to_string(id)) + "}";
    DBG_PRINTF(Web, "make_mode_patch_json -> %s\n", js.c_str());
    return js;
}
std::string make_full_state_json(const Cache& c) {
    // Canonical payload; client will derive hue/percent as needed
    std::string js = std::string("{\"rgb\":[")
        + std::to_string(c.rgb[0]) + "," + std::to_string(c.rgb[1]) + "," + std::to_string(c.rgb[2]) + "],"
        + "\"brightness\":" + std::to_string(c.brightness_255) + ","
        + "\"power\":" + (c.power ? "true" : "false") + ","
        + "\"mode\":" + json_quote(mode_to_string(c.mode_id))
        + "}";
    DBG_PRINTF(Web, "make_full_state_json -> %s\n", js.c_str());
    return js;
}

// ------------------------- UI templates -------------------------
// DROP-IN REPLACEMENT: INDEX_HTML (keep route /static/styles.css)
// REPLACE YOUR INDEX_HTML WITH THIS (note: cache-bust ?v=3 on CSS link)
const char* INDEX_HTML = R"HTML(<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>XeWe LED</title>
  <link rel="stylesheet" href="/static/styles.css?v=3"><!-- cache-bust -->
</head>
<body>

  <header class="page-header">
    <div class="wrap">
      <h1 class="page-title">XeWe LED</h1>
    </div>
  </header>

  <main class="wrap">

    <section class="block" aria-label="{{ name }} controls">
      <h2 class="block-title">{{ name }}</h2>

      <!-- 1) Color slider -->
      <div class="row">
        <input id="color" type="range" min="0" max="360" value="{{ state.hue }}" aria-label="Color">
      </div>

      <!-- 2) Brightness slider -->
      <div class="row">
        <input id="brightness" type="range" min="0" max="100" value="{{ state.brightness }}" aria-label="Brightness">
      </div>

      <!-- 3) Power radio-style switch -->
      <div class="row">
        <div class="seg" role="radiogroup" aria-label="Power">
          <button id="on"  type="button" role="radio" aria-checked="false">On</button>
          <button id="off" type="button" role="radio" aria-checked="true">Off</button>
        </div>
      </div>

      <!-- 4) Mode dropdown -->
      <div class="row">
        <select id="mode" aria-label="Mode">
          <option value="solid">Solid</option>
          <option value="perlin-noise">Perlin Noise</option>
          <option value="rainbow">Rainbow</option>
        </select>
      </div>

      <!-- 5) Advanced button row -->
      <div class="row row--full">
        <a class="btn" href="/advanced" role="button" aria-label="Open advanced controls">Advanced</a>
      </div>
    </section>

  </main>

  <script>
    const color = document.getElementById('color');
    const bri   = document.getElementById('brightness');
    const btnOn = document.getElementById('on');
    const btnOff= document.getElementById('off');
    const mode  = document.getElementById('mode');
    mode.value  = "{{ state.mode }}";

    function post(patch){
      fetch('/api/state', {
        method:'POST',
        headers:{'Content-Type':'application/json'},
        body: JSON.stringify(patch)
      }).catch(()=>{});
    }

    // Client-side conversions
    function hsvToRgbDeg(h, s, v){
      h = ((h % 360) + 360) % 360;
      const hf = h / 60, i = Math.floor(hf), f = hf - i;
      const sf = s / 255, vf = v / 255;
      const p = vf * (1 - sf), q = vf * (1 - sf * f), t = vf * (1 - sf * (1 - f));
      let r=0,g=0,b=0;
      switch(i){case 0:r=vf;g=t;b=p;break;case 1:r=q;g=vf;b=p;break;case 2:r=p;g=vf;b=t;break;
                 case 3:r=p;g=q;b=vf;break;case 4:r=t;g=p;b=vf;break;default:r=vf;g=p;b=q;}
      return [Math.round(r*255),Math.round(g*255),Math.round(b*255)];
    }
    function rgbToHueDeg(r,g,b){
      const rf=r/255,gf=g/255,bf=b/255, max=Math.max(rf,gf,bf), min=Math.min(rf,gf,bf), d=max-min;
      if(!d) return 0;
      let h = max===rf ? ((gf-bf)/d)%6 : max===gf ? (bf-rf)/d+2 : (rf-gf)/d+4;
      h*=60; if(h<0) h+=360; return Math.round(h);
    }
    const bri255ToPercent = b => Math.round(b*100/255);
    const percentToBri255 = p => Math.round(p*255/100);

    // UI helpers
    function briGradient(h){
      return `linear-gradient(90deg,#000 0%,hsl(${h} 100% 10%) 12%,hsl(${h} 100% 50%) 100%)`;
    }
    function updateColorUI(){
      const h = +color.value;
      bri.style.setProperty('--h', h);
      bri.style.background = briGradient(h);
    }
    function setPowerUI(isOn){
      btnOn.setAttribute('aria-checked', isOn ? 'true' : 'false');
      btnOff.setAttribute('aria-checked', !isOn ? 'true' : 'false');
      btnOn.disabled  = isOn;
      btnOff.disabled = !isOn;
    }

    // Send user changes
    color.addEventListener('input', updateColorUI);
    color.addEventListener('change', ()=> post({rgb: hsvToRgbDeg(+color.value, 255, 255)}));
    bri  .addEventListener('change', ()=> post({brightness: percentToBri255(+bri.value)}));
    btnOn.addEventListener('click', ()=>{ if(btnOn.disabled) return; setPowerUI(true);  post({power:true});  });
    btnOff.addEventListener('click', ()=>{ if(btnOff.disabled) return; setPowerUI(false); post({power:false}); });
    mode.addEventListener('change', ()=> post({mode: mode.value}));

    // Realtime patches (SSE)
    function applyPatch(obj){
      if (obj.rgb){ const [r,g,b]=obj.rgb; color.value = rgbToHueDeg(r,g,b); updateColorUI(); }
      if (obj.brightness!==undefined){ bri.value = bri255ToPercent(+obj.brightness); }
      if (obj.power!==undefined){ setPowerUI(!!obj.power); }
      if (obj.mode!==undefined){ mode.value = obj.mode; }
    }
    try{
      const es = new EventSource('/events');
      es.addEventListener('state', ev=>{ try{ applyPatch(JSON.parse(ev.data)); }catch(e){} });
      es.addEventListener('patch', ev=>{ try{ applyPatch(JSON.parse(ev.data)); }catch(e){} });
    }catch(e){}

    // Init
    updateColorUI();
    setPowerUI({{ "true" if state.power else "false" }});
  </script>
</body>
</html>)HTML";



// Compact CSS — make all controls fill the available width (match Advanced button)
// DROP-IN REPLACEMENT: STYLES_CSS
// REPLACE YOUR STYLES_CSS WITH THIS
const char* STYLES_CSS = R"CSS(:root{
  /* Theme */
  --bg:#0f1216; --text:#e8eaed; --muted:#c2c7cf; --panel:#171b21; --control:#1e252e;
  --button-idle:#252e39; --button-active:#10161c; --divider:#2a3138; --focus:#39424a;
  color-scheme:dark;

  /* Power control */
  --indicator-green: rgb(43,200,64);

  /* Typography */
  --font-main:16px; --font-subprime:14px; --line:1.45;

  /* Layout heights (compacted) */
  --row-h:44px; --ctl-h:36px;

  /* Slider sizing */
  --track-h:6px; --thumb-d:var(--ctl-h);
}
*{box-sizing:border-box}
html,body{
  margin:0; padding:0;
  font-family:system-ui,-apple-system,Segoe UI,Roboto,Helvetica,Arial,sans-serif;
  font-size:var(--font-subprime); line-height:var(--line);
  background:var(--bg); color:var(--text);
  -webkit-text-size-adjust:100%; text-size-adjust:100%;
}

/* Page header */
.page-header{border-bottom:1px solid var(--divider); background:var(--bg)}
.page-title{margin:6px 0; font-weight:800; font-size:22px; color:var(--text); text-align:center}

/* Full-width content */
.wrap{width:100%; max-width:none; margin:0; padding:12px 24px}

/* Control block card */
.block{background:var(--panel); border:1px solid var(--divider); border-radius:16px; padding:0 10px}
.block+.block{margin-top:8px}

/* Title */
.block-title{margin:0; padding:8px 0; font-weight:800; font-size:18px; color:var(--text); text-align:center; border-bottom:1px solid var(--divider)}

/* Rows are single-column and full-width controls */
.row{
  display:grid; grid-template-columns:1fr; gap:12px; align-items:center;
  min-height:var(--row-h); padding:8px 10px; border-bottom:1px solid var(--divider)
}
.row:last-child{border-bottom:0}
.row>*{width:100%} /* ensure immediate children span full width */

/* Shared control sizing */
input[type=range], .seg, .seg button, #mode { block-size: var(--ctl-h); }
input[type=range]{ display:block; width:100%; } /* stretch on Safari/iOS */

/* Segmented radio-style switch */
.seg{display:grid; grid-template-columns:1fr 1fr; width:100%; border-radius:12px; background:var(--control); overflow:hidden}
.seg button{
  position:relative; appearance:none; display:inline-flex; align-items:center; justify-content:flex-start;
  padding:0 12px 0 36px; font:inherit; font-size:var(--font-subprime);
  background:var(--button-idle); color:var(--muted); border:none; cursor:pointer
}
.seg button+button{border-left:1px solid var(--divider)}
.seg button::before{
  content:""; position:absolute; left:12px; top:50%; transform:translateY(-50%);
  width:14px; height:14px; border-radius:50%; box-shadow:inset 0 0 0 2px #6a6a6a; background:transparent
}
.seg button[aria-checked=true]{background:var(--button-active); color:#fff; cursor:default}
.seg button[aria-checked=true]::before{background:var(--indicator-green); box-shadow:inset 0 0 0 2px var(--indicator-green)}
.seg button[aria-checked=false]{background:var(--button-idle); color:var(--muted)}
.seg button[disabled]{pointer-events:none}

/* Color slider */
#color{
  appearance:none; height:var(--ctl-h); border-radius:calc(var(--ctl-h)/2); outline:none;
  background:linear-gradient(90deg,
    hsl(0 100% 50%),
    hsl(60 100% 50%),
    hsl(120 100% 40%),
    hsl(180 100% 40%),
    hsl(240 100% 60%),
    hsl(300 100% 50%),
    hsl(360 100% 50%))
}
#color::-webkit-slider-runnable-track{height:var(--track-h); border-radius:999px; background:inherit}
#color::-moz-range-track{height:var(--track-h); border-radius:999px; background:inherit}
#color::-webkit-slider-thumb{
  appearance:none; width:var(--thumb-d); height:var(--thumb-d);
  border-radius:50%; background:#fff; border:2px solid #000;
  margin-top:calc((var(--track-h) - var(--thumb-d))/2)
}
#color::-moz-range-thumb{width:var(--thumb-d); height:var(--thumb-d); border-radius:50%; background:#fff; border:2px solid #000}

/* Brightness */
#brightness{
  --h:200; appearance:none; height:var(--ctl-h); border-radius:calc(var(--ctl-h)/2); outline:none;
  background:linear-gradient(90deg,#000 0%, hsl(var(--h) 100% 10%) 12%, hsl(var(--h) 100% 50%) 100%)
}
#brightness::-webkit-slider-runnable-track{height:var(--track-h); border-radius:999px; background:inherit}
#brightness::-moz-range-track{height:var(--track-h); border-radius:999px; background:inherit}
#brightness::-webkit-slider-thumb{
  appearance:none; width:var(--thumb-d); height:var(--thumb-d);
  border-radius:50%; background:#fff; border:2px solid #000;
  margin-top:calc((var(--track-h) - var(--thumb-d))/2)
}
#brightness::-moz-range-thumb{width:var(--thumb-d); height:var(--thumb-d); border-radius:50%; background:#fff; border:2px solid #000}

/* Dropdown (Mode) */
#mode{
  appearance:none; -webkit-appearance:none; -moz-appearance:none;
  width:100%; height:var(--ctl-h); padding:0 36px 0 12px;
  border:1px solid var(--divider); border-radius:12px; background-color:var(--control);
  color:var(--muted); line-height:var(--ctl-h); outline:none; font-size:var(--font-subprime); font-family:inherit;
  background-image:url("data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='14' height='14' viewBox='0 0 24 24' fill='none' stroke='%23c2c7cf' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><polyline points='6 9 12 15 18 9'/></svg>");
  background-repeat:no-repeat; background-position:right 12px center; background-size:14px 14px
}
#mode:focus{border-color:var(--focus); color:var(--text)}
#mode option{background:var(--control); color:var(--text); font-size:var(--font-subprime); font-family:inherit}
#mode::-ms-value{color:var(--muted); background:transparent}

/* Full-width single-control row */
.row--full{grid-template-columns:1fr}

/* Button */
.btn{
  display:inline-flex; justify-content:center; align-items:center;
  height:var(--ctl-h); width:100%; padding:0 16px; border-radius:12px;
  border:1px solid var(--divider); background:var(--control); color:var(--muted);
  text-decoration:none; font:inherit; font-size:var(--font-subprime); cursor:pointer
}
.btn:hover{color:var(--text)}
.btn:focus{outline:none; border-color:var(--focus); color:var(--text)}
.btn:active{background:var(--button-active); color:#fff}
.btn::after{content:"›"; margin-left:8px; opacity:.7}
)CSS";


// Render index with current values (no server-side conversions)
std::string render_index() {
    DBG_PRINTLN(Web, "-> render_index()");
    lock();
    auto   rgb     = cache.rgb;
    uint8_t bri255 = cache.brightness_255;
    bool   power   = cache.power;
    uint8_t mode_id= cache.mode_id;
    unlock();

    DBG_PRINTF(Web, "render_index (canonical): rgb={%u,%u,%u} bri255=%u power=%s mode_id=%u\n",
               rgb[0], rgb[1], rgb[2], bri255, power ? "true" : "false", mode_id);

    std::string html = INDEX_HTML;
    replace_all(html, "{{ name }}", "LED Strip");
    // Placeholders; client SSE will derive/view-state
    replace_all(html, "{{ state.hue }}", "0");
    replace_all(html, "{{ state.brightness }}", "0");

    const char* mode_str = (mode_id == 0) ? "solid" : "solid";
    replace_all(html, "{{ state.mode }}", mode_str);

    replace_all(html, "{{ \"true\" if state.power else \"false\" }}", power ? "true" : "false");
    DBG_PRINTF(Web, "<- render_index() size=%u\n", (unsigned)html.size());
    return html;
}

// Send JSON {"ok":true}
void send_ok(AsyncWebServerRequest* req) {
    DBG_PRINTF(Web, "send_ok() url=%s\n", req->url().c_str());
    AsyncWebServerResponse* res = req->beginResponse(200, "application/json", "{\"ok\":true}");
    res->addHeader("Access-Control-Allow-Origin", "*");
    res->addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    res->addHeader("Access-Control-Allow-Headers", "Content-Type");
    req->send(res);
}

// Minimal 204 for OPTIONS
void send_options(AsyncWebServerRequest* req) {
    DBG_PRINTF(Web, "send_options() url=%s\n", req->url().c_str());
    AsyncWebServerResponse* res = req->beginResponse(204);
    res->addHeader("Access-Control-Allow-Origin", "*");
    res->addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    res->addHeader("Access-Control-Allow-Headers", "Content-Type");
    req->send(res);
}

} // namespace

// --------------------------------------------------------------------
// Web class
// --------------------------------------------------------------------
Web::Web(SystemController& controller_ref)
: Interface(controller_ref, "web", "web", false, false)
{
    DBG_PRINTLN(Web, "Web::Web()");
}

void Web::begin(const ModuleConfig& cfg) {
    (void)cfg;
    DBG_PRINTLN(Web, "Web::begin()");
    g_web = this;

    if (!cache_mutex) {
        cache_mutex = xSemaphoreCreateMutex();
        if (!cache_mutex) {
            DBG_PRINTLN(Web, "FATAL: cache_mutex create failed");
        } else {
            DBG_PRINTLN(Web, "cache_mutex created");
        }
    }

    DBG_PRINTF(Web, "WiFi status=%d, IP=%s\n", WiFi.status(), WiFi.localIP().toString().c_str());

    // ------------------------- Routes -------------------------
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req){
        DBG_PRINTF(Web, "HTTP GET %s from client=%p\n", req->url().c_str(), (void*)req->client());
        std::string html = render_index();
        AsyncWebServerResponse* res = req->beginResponse(200, "text/html; charset=utf-8", html.c_str());
        res->addHeader("Cache-Control", "no-store");
        req->send(res);
    });

    server.on("/advanced", HTTP_GET, [](AsyncWebServerRequest* req){
        DBG_PRINTF(Web, "HTTP GET %s\n", req->url().c_str());
        req->send(200, "text/plain; charset=utf-8", "Advanced UI placeholder");
    });

    server.on("/static/styles.css", HTTP_GET, [](AsyncWebServerRequest* req){
        DBG_PRINTF(Web, "HTTP GET %s\n", req->url().c_str());
        AsyncWebServerResponse* res = req->beginResponse(200, "text/css; charset=utf-8", STYLES_CSS);
        res->addHeader("Cache-Control", "max-age=31536000, immutable");
        req->send(res);
    });

    // SSE handler: send full canonical state to new client
    events.onConnect([](AsyncEventSourceClient* client){
        DBG_PRINTF(Web, "SSE onConnect client=%p lastId=%u\n", (void*)client, client ? client->lastId() : 0);
        lock(); Cache c = cache; unlock();
        std::string js = make_full_state_json(c);
        client->send(js.c_str(), "state");
        DBG_PRINTLN(Web, "SSE initial state sent");
    });
    server.addHandler(&events);
    DBG_PRINTLN(Web, "SSE /events handler added");

    // API: JSON PATCH to update any of {rgb, brightness(0..255), power, mode}
    server.on(
        "/api/state",
        HTTP_POST,
        [](AsyncWebServerRequest* req){
            DBG_PRINTF(Web, "HTTP POST %s contentLength=%u (awaiting body)\n",
                       req->url().c_str(), (unsigned)req->contentLength());
            if (req->contentLength() == 0) {
                DBG_PRINTLN(Web, "POST /api/state: empty body -> 400");
                req->send(400, "application/json", "{\"ok\":false,\"err\":\"empty body\"}");
            }
        },
        nullptr,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total){
            if (req->url() != "/api/state") return;

            DBG_PRINTF(Web, "onBody: chunk len=%u index=%u total=%u\n",
                       (unsigned)len, (unsigned)index, (unsigned)total);

            // Accumulate chunks
            std::string* buf = static_cast<std::string*>(req->_tempObject);
            if (index == 0) {
                DBG_PRINTLN(Web, "onBody: allocate buffer");
                buf = new std::string();
                buf->reserve(total);
                req->_tempObject = buf;
            }
            buf->append(reinterpret_cast<char*>(data), len);

            if (index + len < total) {
                DBG_PRINTLN(Web, "onBody: awaiting more chunks");
                return;
            }

            // Completed body
            std::string body = std::move(*buf);
            delete buf;
            req->_tempObject = nullptr;

            DBG_PRINTF(Web, "onBody: complete body len=%u: %s\n", (unsigned)body.size(), body.c_str());

            bool changed = false;

            // Prefer canonical fields (no server-side conversions)
            std::array<uint8_t,3> rgb;
            if (extract_rgb_array(body, "rgb", rgb)) {
                { lock(); cache.rgb = rgb; unlock(); }
                DBG_PRINTF(Web, "PATCH rgb={%u,%u,%u}\n", rgb[0], rgb[1], rgb[2]);
                push_patch(make_color_patch_json(rgb));
                DBG_PRINTLN(Web, "controller.sync_color(..., {true,true,false,true,true})");
                g_web->controller.sync_color(rgb, {true,true,false,true,true});
                changed = true;
            }

            int bri_255_int;
            if (extract_number(body, "brightness", bri_255_int)) {
                uint8_t b = clamp_u8(bri_255_int);
                { lock(); cache.brightness_255 = b; unlock(); }
                DBG_PRINTF(Web, "PATCH brightness=%u (0..255)\n", b);
                push_patch(make_brightness_patch_json(b));
                DBG_PRINTLN(Web, "controller.sync_brightness(..., {true,true,false,true,true})");
                g_web->controller.sync_brightness(b, {true,true,false,true,true});
                changed = true;
            }

            bool pwr;
            if (extract_bool(body, "power", pwr)) {
                { lock(); cache.power = pwr; unlock(); }
                DBG_PRINTF(Web, "PATCH power=%s\n", pwr ? "true" : "false");
                push_patch(make_power_patch_json(pwr));
                DBG_PRINTLN(Web, "controller.sync_state(..., {true,true,false,true,true})");
                g_web->controller.sync_state(pwr ? 1 : 0, {true,true,false,true,true});
                changed = true;
            }

            std::string mode_str;
            if (extract_string(body, "mode", mode_str)) {
                uint8_t id = 0; // extend mapping as modes are added
                { lock(); cache.mode_id = id; unlock(); }
                DBG_PRINTF(Web, "PATCH mode='%s' -> id=%u\n", mode_str.c_str(), id);
                push_patch(make_mode_patch_json(id));
                DBG_PRINTLN(Web, "controller.sync_mode(..., {true,true,false,true,true})");
                g_web->controller.sync_mode(id, {true,true,false,true,true});
                changed = true;
            }

            if (!changed) {
                DBG_PRINTLN(Web, "POST /api/state: no recognized fields -> 400");
                req->send(400, "application/json", "{\"ok\":false,\"err\":\"no recognized fields\"}");
                return;
            }
            DBG_PRINTLN(Web, "POST /api/state: ok");
            send_ok(req);
        }
    );

    // CORS preflight
    server.on("/api/state", HTTP_OPTIONS, [](AsyncWebServerRequest* req){
        DBG_PRINTF(Web, "HTTP OPTIONS %s\n", req->url().c_str());
        send_options(req);
    });

    server.onNotFound([](AsyncWebServerRequest* req){
        DBG_PRINTF(Web, "HTTP 404 %s\n", req->url().c_str());
        req->send(404, "application/json", "{\"ok\":false,\"err\":\"not found\"}");
    });

    server.begin();
    DBG_PRINTLN(Web, "Async Web server started on port 80 (ESP32).");
}

void Web::loop() {
    // Async server does not require polling.
}

void Web::reset(bool verbose) {
    (void)verbose;
    DBG_PRINTLN(Web, "Web::reset() -> clearing cache and broadcasting full state");
    lock();
    cache = Cache{};
    unlock();
    push_patch(make_full_state_json(cache));
}

// --------------- Interface sync feeds our UI cache (and PUSH) ---------------
void Web::sync_color(std::array<uint8_t,3> color) {
    DBG_PRINTF(Web, "sync_color(rgb={%u,%u,%u}) -> cache+push\n", color[0], color[1], color[2]);
    lock(); cache.rgb = color; unlock();
    push_patch(make_color_patch_json(color));
}
void Web::sync_brightness(uint8_t brightness) {
    DBG_PRINTF(Web, "sync_brightness(%u) -> cache+push\n", brightness);
    lock(); cache.brightness_255 = brightness; unlock();
    push_patch(make_brightness_patch_json(brightness));
}
void Web::sync_state(uint8_t state) {
    bool p = (state != 0);
    DBG_PRINTF(Web, "sync_state(%u -> power=%s) -> cache+push\n", state, p ? "true" : "false");
    lock(); cache.power = p; unlock();
    push_patch(make_power_patch_json(p));
}
void Web::sync_mode(uint8_t mode) {
    DBG_PRINTF(Web, "sync_mode(%u) -> cache+push\n", mode);
    lock(); cache.mode_id = mode; unlock();
    push_patch(make_mode_patch_json(mode));
}
void Web::sync_length(uint16_t /*length*/) {
    DBG_PRINTLN(Web, "sync_length() called -> no-op (not exposed in UI)");
}
void Web::sync_all(std::array<uint8_t,3> color,
                   uint8_t brightness,
                   uint8_t state,
                   uint8_t mode,
                   uint16_t /*length*/) {
    DBG_PRINTF(Web, "sync_all(rgb={%u,%u,%u}, bri=%u, state=%u, mode=%u)\n",
               color[0], color[1], color[2], brightness, state, mode);
    lock();
    cache.rgb = color;
    cache.brightness_255 = brightness;
    cache.power = (state != 0);
    cache.mode_id = mode;
    Cache snapshot = cache;
    unlock();

    std::string js = make_full_state_json(snapshot);
    DBG_PRINTF(Web, "sync_all: broadcasting state len=%u\n", (unsigned)js.size());
    events.send(js.c_str(), "state");
}

