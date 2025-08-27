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
Web* g_web = nullptr;

// Thread-safe cache of current state exposed to UI
struct Cache {
    std::array<uint8_t,3> rgb {0,0,0};  // current color
    uint8_t brightness_255 = 0;         // 0..255
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

// Map hue [0..360] and S,V [0..255] to RGB [0..255]
std::array<uint8_t,3> hsv_to_rgb_deg(uint16_t hue_deg, uint8_t s, uint8_t v) {
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
    return { uint8_t(rf*255 + 0.5f), uint8_t(gf*255 + 0.5f), uint8_t(bf*255 + 0.5f) };
}

// Convert RGB [0..255] to hue degrees [0..360)
uint16_t rgb_to_hue_deg(std::array<uint8_t,3> rgb) {
    float r = rgb[0]/255.0f, g = rgb[1]/255.0f, b = rgb[2]/255.0f;
    float maxc = std::max({r,g,b});
    float minc = std::min({r,g,b});
    float d = maxc - minc;
    if (d == 0.0f) return 0;
    float h;
    if (maxc == r)      h = fmodf((g - b) / d, 6.0f);
    else if (maxc == g) h = ((b - r) / d) + 2.0f;
    else                h = ((r - g) / d) + 4.0f;
    h *= 60.0f;
    if (h < 0) h += 360.0f;
    return uint16_t(h + 0.5f);
}

// Replace all occurrences of `from` with `to` in s
void replace_all(std::string& s, const std::string& from, const std::string& to) {
    if (from.empty()) return;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
}

// Trim and lowercase helper
std::string lc(std::string s) {
    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c){return std::isspace(c);}), s.end());
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return char(std::tolower(c)); });
    return s;
}

// Extract number field from a tiny JSON body like {"hue":123,...}
bool extract_number(const std::string& body, const std::string& key, int& out) {
    const std::string pat = "\"" + key + "\"";
    size_t k = body.find(pat);
    if (k == std::string::npos) return false;
    size_t colon = body.find(':', k + pat.size());
    if (colon == std::string::npos) return false;
    size_t i = colon + 1;
    while (i < body.size() && std::isspace((unsigned char)body[i])) ++i;
    bool neg = false;
    if (i < body.size() && (body[i] == '-' || body[i] == '+')) { neg = (body[i]=='-'); ++i; }
    long val = 0;
    bool any = false;
    while (i < body.size() && std::isdigit((unsigned char)body[i])) { any = true; val = val*10 + (body[i]-'0'); ++i; }
    if (!any) return false;
    out = neg ? -int(val) : int(val);
    return true;
}
bool extract_bool(const std::string& body, const std::string& key, bool& out) {
    const std::string pat = "\"" + key + "\"";
    size_t k = body.find(pat);
    if (k == std::string::npos) return false;
    size_t colon = body.find(':', k + pat.size());
    if (colon == std::string::npos) return false;
    size_t i = colon + 1;
    while (i < body.size() && std::isspace((unsigned char)body[i])) ++i;
    if (body.compare(i, 4, "true") == 0)  { out = true;  return true; }
    if (body.compare(i, 5, "false") == 0) { out = false; return true; }
    return false;
}
bool extract_string(const std::string& body, const std::string& key, std::string& out) {
    const std::string pat = "\"" + key + "\"";
    size_t k = body.find(pat);
    if (k == std::string::npos) return false;
    size_t colon = body.find(':', k + pat.size());
    if (colon == std::string::npos) return false;
    size_t i = colon + 1;
    while (i < body.size() && std::isspace((unsigned char)body[i])) ++i;
    if (i >= body.size() || body[i] != '\"') return false;
    ++i;
    std::string val;
    while (i < body.size() && body[i] != '\"') { val.push_back(body[i++]); }
    if (i >= body.size()) return false;
    out = val;
    return true;
}

// ------------------------- UI templates -------------------------
const char* INDEX_HTML = R"HTML(<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>XeWe LED</title>
  <link rel="stylesheet" href="/static/styles.css">
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
        <label for="color">Color</label>
        <input id="color" type="range" min="0" max="360" value="{{ state.hue }}">
      </div>

      <!-- 2) Brightness slider -->
      <div class="row">
        <label for="brightness">Brightness</label>
        <input id="brightness" type="range" min="0" max="100" value="{{ state.brightness }}">
      </div>

      <!-- 3) Power radio-style switch -->
      <div class="row">
        <label>Power</label>
        <div class="seg" role="radiogroup" aria-label="Power">
          <button id="on"  type="button" role="radio" aria-checked="false">On</button>
          <button id="off" type="button" role="radio" aria-checked="true">Off</button>
        </div>
      </div>

      <!-- 4) Mode dropdown -->
      <div class="row">
        <label for="mode">Mode</label>
        <select id="mode">
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

    // Brightness: black -> dim knee -> full color
    function briGradient(h){
      return `linear-gradient(90deg,
        #000 0%,
        hsl(${h} 100% 10%) 12%,
        hsl(${h} 100% 50%) 100%)`;
    }
    function updateColorUI(){
      const h = +color.value;
      bri.style.setProperty('--h', h);
      bri.style.background = briGradient(h);
    }

    // Power UI lock
    function setPowerUI(isOn){
      btnOn.setAttribute('aria-checked', isOn ? 'true' : 'false');
      btnOff.setAttribute('aria-checked', !isOn ? 'true' : 'false');
      btnOn.disabled  = isOn;
      btnOff.disabled = !isOn;
    }

    color.addEventListener('input', updateColorUI);
    color.addEventListener('change', ()=> post({hue:+color.value}));

    bri.addEventListener('change', ()=> post({brightness:+bri.value}));

    btnOn.addEventListener('click', ()=>{
      if (btnOn.disabled) return;
      setPowerUI(true);
      post({power:true});
    });
    btnOff.addEventListener('click', ()=>{
      if (btnOff.disabled) return;
      setPowerUI(false);
      post({power:false});
    });

    mode.addEventListener('change', ()=> post({mode: mode.value}));

    // Init
    updateColorUI();
    setPowerUI({{ "true" if state.power else "false" }});
  </script>
</body>
</html>)HTML";

// Your CSS served at /static/styles.css
const char* STYLES_CSS = R"CSS(:root{
  --bg:#0f1216;--text:#e8eaed;--muted:#c2c7cf;--panel:#171b21;--control:#1e252e;
  --button-idle:#252e39;--button-active:#10161c;--divider:#2a3138;--focus:#39424a;color-scheme:dark;
  --indicator-green:rgb(43,200,64);
  --font-main:16px;--font-subprime:14px;--line:1.45;
  --row-h:44px;--ctl-h:36px;--track-h:6px;--thumb-d:var(--ctl-h)
}
*{box-sizing:border-box}html,body{margin:0;padding:0;font-family:system-ui,-apple-system,Segoe UI,Roboto,Helvetica,Arial,sans-serif;font-size:var(--font-subprime);line-height:var(--line);background:var(--bg);color:var(--text);-webkit-text-size-adjust:100%;text-size-adjust:100%}
.page-header{border-bottom:1px solid var(--divider);background:var(--bg)}
.page-title{margin:6px 0;font-weight:800;font-size:22px;color:var(--text);text-align:center}
.wrap{width:80vw;margin:8px auto;padding:12px}
.block{background:var(--panel);border:1px solid var(--divider);border-radius:16px;padding:0 10px}
.block+.block{margin-top:8px}
.block-title{margin:0;padding:8px 0;font-weight:800;font-size:18px;color:var(--text);text-align:center;border-bottom:1px solid var(--divider)}
.row{display:grid;grid-template-columns:120px 1fr;gap:12px;align-items:center;min-height:var(--row-h);padding:8px 10px;border-bottom:1px solid var(--divider)}
.row:last-child{border-bottom:0}
label{color:var(--muted);font-size:var(--font-main);font-weight:700}
input[type=range],.seg,.seg button,#mode{block-size:var(--ctl-h)}
input[type=range]{width:100%}
.seg{display:grid;grid-template-columns:1fr 1fr;border-radius:12px;background:var(--control);overflow:hidden}
.seg button{position:relative;appearance:none;display:inline-flex;align-items:center;padding:0 12px 0 36px;font:inherit;font-size:var(--font-subprime);background:var(--button-idle);color:var(--muted);border:none;cursor:pointer}
.seg button+button{border-left:1px solid var(--divider)}
.seg button::before{content:"";position:absolute;left:12px;top:50%;transform:translateY(-50%);width:14px;height:14px;border-radius:50%;box-shadow:inset 0 0 0 2px #6a6a6a;background:transparent}
.seg button[aria-checked=true]{background:var(--button-active);color:#fff;cursor:default}
.seg button[aria-checked=true]::before{background:var(--indicator-green);box-shadow:inset 0 0 0 2px var(--indicator-green)}
.seg button[aria-checked=false]{background:var(--button-idle);color:var(--muted)}
.seg button[disabled]{pointer-events:none}
#color{appearance:none;height:var(--ctl-h);border-radius:calc(var(--ctl-h)/2);outline:none;background:linear-gradient(90deg,hsl(0 100% 50%),hsl(60 100% 50%),hsl(120 100% 40%),hsl(180 100% 40%),hsl(240 100% 60%),hsl(300 100% 50%),hsl(360 100% 50%))}
#color::-webkit-slider-runnable-track{height:var(--track-h);border-radius:999px;background:inherit}
#color::-moz-range-track{height:var(--track-h);border-radius:999px;background:inherit}
#color::-webkit-slider-thumb{appearance:none;width:var(--thumb-d);height:var(--thumb-d);border-radius:50%;background:#fff;border:2px solid #000;margin-top:calc((var(--track-h)-var(--thumb-d))/2)}
#color::-moz-range-thumb{width:var(--thumb-d);height:var(--thumb-d);border-radius:50%;background:#fff;border:2px solid #000}
#brightness{--h:200;appearance:none;height:var(--ctl-h);border-radius:calc(var(--ctl-h)/2);outline:none;background:linear-gradient(90deg,#000 0%,hsl(var(--h) 100% 10%) 12%,hsl(var(--h) 100% 50%) 100%)}
#brightness::-webkit-slider-runnable-track{height:var(--track-h);border-radius:999px;background:inherit}
#brightness::-moz-range-track{height:var(--track-h);border-radius:999px;background:inherit}
#brightness::-webkit-slider-thumb{appearance:none;width:var(--thumb-d);height:var(--thumb-d);border-radius:50%;background:#fff;border:2px solid #000;margin-top:calc((var(--track-h)-var(--thumb-d))/2)}
#brightness::-moz-range-thumb{width:var(--thumb-d);height:var(--thumb-d);border-radius:50%;background:#fff;border:2px solid #000}
#mode{appearance:none;-webkit-appearance:none;-moz-appearance:none;width:100%;height:var(--ctl-h);padding:0 36px 0 12px;border:1px solid var(--divider);border-radius:12px;background-color:var(--control);color:var(--muted);line-height:var(--ctl-h);outline:none;font-size:var(--font-subprime);font-family:inherit;background-image:url("data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='14' height='14' viewBox='0 0 24 24' fill='none' stroke='%23c2c7cf' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><polyline points='6 9 12 15 18 9'/></svg>");background-repeat:no-repeat;background-position:right 12px center;background-size:14px 14px}
#mode:focus{border-color:var(--focus);color:var(--text)}
#mode option{background:var(--control);color:var(--text);font-size:var(--font-subprime);font-family:inherit}
#mode::-ms-value{color:var(--muted);background:transparent}
.row--full{grid-template-columns:1fr}
.btn{display:inline-flex;justify-content:center;align-items:center;height:var(--ctl-h);width:100%;padding:0 16px;border-radius:12px;border:1px solid var(--divider);background:var(--control);color:var(--muted);text-decoration:none;font:inherit;font-size:var(--font-subprime);cursor:pointer}
.btn:hover{color:var(--text)}.btn:focus{outline:none;border-color:var(--focus);color:var(--text)}.btn:active{background:var(--button-active);color:#fff}
.btn::after{content:"›";margin-left:8px;opacity:.7}
)CSS";

// Render index with current values
std::string render_index() {
    lock();
    auto rgb = cache.rgb;
    uint8_t bri255 = cache.brightness_255;
    bool power = cache.power;
    uint8_t mode_id = cache.mode_id;
    unlock();

    // Derive hue degrees and brightness percent
    uint16_t hue = rgb_to_hue_deg(rgb);
    uint8_t bri_percent = uint8_t( (uint32_t(bri255) * 100 + 127) / 255 );

    std::string html = INDEX_HTML;
    replace_all(html, "{{ name }}", "LED Strip");
    replace_all(html, "{{ state.hue }}", std::to_string(hue));
    replace_all(html, "{{ state.brightness }}", std::to_string(bri_percent));

    const char* mode_str = (mode_id == 0) ? "solid" : "solid";
    replace_all(html, "{{ state.mode }}", mode_str);

    replace_all(html, "{{ \"true\" if state.power else \"false\" }}", power ? "true" : "false");
    return html;
}

// Send JSON {"ok":true}
void send_ok(AsyncWebServerRequest* req) {
    AsyncWebServerResponse* res = req->beginResponse(200, "application/json", "{\"ok\":true}");
    res->addHeader("Access-Control-Allow-Origin", "*");
    res->addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    res->addHeader("Access-Control-Allow-Headers", "Content-Type");
    req->send(res);
}

// Minimal 204 for OPTIONS
void send_options(AsyncWebServerRequest* req) {
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
        }
    }

    // ------------------------- Routes -------------------------
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req){
        std::string html = render_index();
        AsyncWebServerResponse* res = req->beginResponse(200, "text/html; charset=utf-8", html.c_str());
        res->addHeader("Cache-Control", "no-store");
        req->send(res);
    });

    server.on("/advanced", HTTP_GET, [](AsyncWebServerRequest* req){
        req->send(200, "text/plain; charset=utf-8", "Advanced UI placeholder");
    });

    server.on("/static/styles.css", HTTP_GET, [](AsyncWebServerRequest* req){
        AsyncWebServerResponse* res = req->beginResponse(200, "text/css; charset=utf-8", STYLES_CSS);
        res->addHeader("Cache-Control", "max-age=31536000, immutable");
        req->send(res);
    });

    // API: JSON PATCH to update any of {hue, brightness, power, mode}
    server.on("/api/state", HTTP_POST, [](AsyncWebServerRequest* req){
        // Do not respond here; body will be handled in onRequestBody.
        if (req->contentLength() == 0) {
            req->send(400, "application/json", "{\"ok\":false,\"err\":\"empty body\"}");
        }
    });

    // CORS preflight
    server.on("/api/state", HTTP_OPTIONS, [](AsyncWebServerRequest* req){ send_options(req); });

    // Body handler with accumulation
    server.onRequestBody([](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total){
        if (req->url() != "/api/state" || req->method() != HTTP_POST) return;

        // Accumulate chunks
        std::string* buf = static_cast<std::string*>(req->_tempObject);
        if (index == 0) {
            buf = new std::string();
            buf->reserve(total);
            req->_tempObject = buf;
        }
        buf->append(reinterpret_cast<char*>(data), len);

        // Not complete yet
        if (index + len < total) return;

        // Completed body
        std::string body = std::move(*buf);
        delete buf;
        req->_tempObject = nullptr;

        bool changed = false;

        // 1) hue: 0..360 -> set RGB at full V, full S
        int hue_deg;
        if (extract_number(body, "hue", hue_deg)) {
            hue_deg = std::max(0, std::min(360, hue_deg));
            auto rgb = hsv_to_rgb_deg(uint16_t(hue_deg), 255, 255);
            lock(); cache.rgb = rgb; unlock();
            g_web->controller.sync_color(rgb, {true,true,true,true,true});
            changed = true;
        }

        // 2) brightness: 0..100 -> 0..255
        int bri_pct;
        if (extract_number(body, "brightness", bri_pct)) {
            bri_pct = std::max(0, std::min(100, bri_pct));
            uint8_t bri_255 = uint8_t( (bri_pct * 255 + 50) / 100 );
            lock(); cache.brightness_255 = bri_255; unlock();
            g_web->controller.sync_brightness(bri_255, {true,true,true,true,true});
            changed = true;
        }

        // 3) power: true|false
        bool pwr;
        if (extract_bool(body, "power", pwr)) {
            lock(); cache.power = pwr; unlock();
            g_web->controller.sync_state(pwr ? 1 : 0, {true,true,true,true,true});
            changed = true;
        }

        // 4) mode: "solid" | others (mapped to solid for now)
        std::string mode_str;
        if (extract_string(body, "mode", mode_str)) {
            std::string v = lc(mode_str);
            uint8_t id = 0; // MODE_SOLID only for now
            lock(); cache.mode_id = id; unlock();
            g_web->controller.sync_mode(id, {true,true,false,true,true});
            changed = true;
        }

        if (!changed) {
            req->send(400, "application/json", "{\"ok\":false,\"err\":\"no recognized fields\"}");
            return;
        }
        send_ok(req);
    });

    server.onNotFound([](AsyncWebServerRequest* req){
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
    lock();
    cache = Cache{};
    unlock();
}

// --------------- Interface sync feeds our UI cache ---------------
void Web::sync_color(std::array<uint8_t,3> color) {
    lock(); cache.rgb = color; unlock();
}
void Web::sync_brightness(uint8_t brightness) {
    lock(); cache.brightness_255 = brightness; unlock();
}
void Web::sync_state(uint8_t state) {
    lock(); cache.power = (state != 0); unlock();
}
void Web::sync_mode(uint8_t mode) {
    lock(); cache.mode_id = mode; unlock();
}
void Web::sync_length(uint16_t /*length*/) {
    // Length is not shown in the provided HTML. No-op.
}
void Web::sync_all(std::array<uint8_t,3> color,
                   uint8_t brightness,
                   uint8_t state,
                   uint8_t mode,
                   uint16_t /*length*/) {
    lock();
    cache.rgb = color;
    cache.brightness_255 = brightness;
    cache.power = (state != 0);
    cache.mode_id = mode;
    unlock();
}
