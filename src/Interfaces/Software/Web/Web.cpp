#include "Web.h"
#include "../../../SystemController/SystemController.h"

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <pgmspace.h>
#include <math.h>

// -----------------------------
// Embedded UI (PROGMEM)
// -----------------------------

// index.html
static const char INDEX_HTML[] PROGMEM = R"html(<!doctype html><html lang="en"><head>
<meta charset="utf-8"/><meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover"/>
<title>ESP32 LED Controller</title>
<link rel="stylesheet" href="/styles.css"/>
</head><body>
<header class="appbar"><h1>LED Controller</h1><div class="actions"><button id="syncBtn" class="btn secondary" title="Sync from device">Sync</button></div></header>
<main class="container">
<section class="card preview-card" aria-label="Current color preview">
  <div class="preview-swatch" id="previewSwatch" role="img" aria-label="Color preview"></div>
  <div class="preview-meta">
    <div><strong>RGB</strong> <span id="rgbText">—</span></div>
    <div><strong>Hue</strong> <span id="hueText">—</span></div>
    <div><strong>Brightness</strong> <span id="brightnessText">—</span></div>
  </div>
</section>
<section class="card controls">
  <div class="row"><label for="powerSwitch">Power</label>
    <label class="switch"><input type="checkbox" id="powerSwitch"/><span class="slider"></span></label>
  </div>
  <div class="row"><label for="modeSelect">Mode</label><select id="modeSelect" class="select"></select></div>
  <div class="row"><label for="hueRange">Hue</label><div class="range-wrap">
    <input id="hueRange" class="range hue" type="range" min="0" max="255" step="1" aria-label="Hue (0–255)"/>
    <output id="hueValue" class="bubble">0</output></div></div>
  <div class="row"><label for="brightnessRange">Brightness</label><div class="range-wrap">
    <input id="brightnessRange" class="range brightness" type="range" min="0" max="255" step="1" aria-label="Brightness (0–255)"/>
    <output id="brightnessValue" class="bubble">0</output></div></div>
  <div class="row"><label for="lengthRange">Length</label><div class="range-wrap">
    <input id="lengthRange" class="range" type="range" min="0" max="255" step="1" aria-label="Length (0–255)"/>
    <output id="lengthValue" class="bubble">0</output></div></div>
</section>
<section class="card help"><details><summary>Notes (firmware)</summary>
<ul>
<li>All values are 0–255 on the wire. Power is 0/255.</li>
<li>Hue track renders red→spectrum→red; hue 0 and 255 are both red.</li>
<li>UI commits on touch/drag end; press <b>Sync</b> or call <code>sync_()</code> to pull.</li>
</ul></details></section></main>
<div id="toast" class="toast" role="status" aria-live="polite"></div>
<script src="/script.js" defer></script></body></html>)html";

// styles.css (minified)
static const char STYLES_CSS[] PROGMEM = R"css(:root{--bg:#0e0f12;--surface:#171922;--surface-2:#1f2230;--text:#e6e8ef;--muted:#a6adbb;--accent:#4da3ff;--outline:#2b2f3d;--radius:14px;--shadow:0 6px 26px rgba(0,0,0,.35);--thumb-size:28px;--track-height:14px}*{box-sizing:border-box}html,body{height:100%}body{margin:0;font:16px/1.4 system-ui,-apple-system,Segoe UI,Roboto,Helvetica Neue,Arial,sans-serif;color:var(--text);background:radial-gradient(1200px 800px at 100% -20%,#131625 0%,var(--bg) 55%);-webkit-font-smoothing:antialiased}.appbar{position:sticky;top:0;display:flex;align-items:center;justify-content:space-between;padding:16px clamp(16px,5vw,28px);background:linear-gradient(180deg,rgba(12,13,18,.75) 0%,rgba(12,13,18,.4) 100%);backdrop-filter:blur(10px);border-bottom:1px solid var(--outline);z-index:10}.appbar h1{margin:0;font-size:18px;letter-spacing:.4px}.actions{display:flex;gap:10px}.container{padding:18px clamp(16px,5vw,28px) 40px;max-width:720px;margin:0 auto;display:grid;gap:16px}.card{background:var(--surface);border:1px solid var(--outline);border-radius:var(--radius);box-shadow:var(--shadow);padding:14px}.preview-card{display:grid;grid-template-columns:96px 1fr;gap:14px;align-items:center}.preview-swatch{width:96px;height:96px;border-radius:16px;border:1px solid var(--outline);background:#000;box-shadow:inset 0 0 0 1px rgba(255,255,255,.05),0 10px 24px rgba(0,0,0,.6)}.preview-meta{color:var(--muted);display:grid;gap:6px;font-size:14px}.preview-meta strong{color:var(--text);font-weight:600;margin-right:6px}.controls .row{display:grid;grid-template-columns:120px 1fr;align-items:center;gap:12px;padding:10px 8px;border-radius:10px}.controls .row+.row{border-top:1px dashed var(--outline)}.controls label{color:var(--muted);font-size:14px}.btn{padding:10px 14px;font-weight:600;border-radius:999px;border:1px solid var(--outline);background:var(--surface-2);color:var(--text)}.btn.secondary{background:transparent}.btn:active{transform:translateY(1px)}.select{width:100%;padding:12px 14px;border-radius:12px;border:1px solid var(--outline);background:var(--surface-2);color:var(--text);appearance:none}.switch{position:relative;display:inline-block;width:60px;height:34px}.switch input{display:none}.switch .slider{position:absolute;cursor:pointer;inset:0;background:#2a2f3b;border-radius:999px;border:1px solid var(--outline);transition:background .2s ease,box-shadow .2s ease}.switch .slider:before{content:"";position:absolute;height:26px;width:26px;left:4px;top:3px;background:linear-gradient(180deg,#fff,#cfd3da);border-radius:50%;box-shadow:0 2px 8px rgba(0,0,0,.4);transition:transform .22s cubic-bezier(.2,.7,.2,1)}.switch input:checked+.slider{background:linear-gradient(90deg,#1f6fff,#6cc8ff)}.switch input:checked+.slider:before{transform:translateX(26px)}.range-wrap{position:relative;display:grid;align-items:center}.bubble{position:absolute;right:0;top:-28px;font-size:12px;color:var(--muted);background:transparent;padding:0 4px}input[type=range].range{-webkit-appearance:none;appearance:none;width:100%;height:var(--thumb-size);background:transparent;margin:8px 0;touch-action:none}input[type=range].range::-webkit-slider-runnable-track{height:var(--track-height);background:var(--track-bg,linear-gradient(90deg,#3b3f52,#3b3f52));border-radius:999px;border:1px solid var(--outline)}input[type=range].range::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:var(--thumb-size);height:var(--thumb-size);border-radius:50%;border:2px solid rgba(0,0,0,.25);background:var(--thumb-bg,#fff);box-shadow:0 4px 10px rgba(0,0,0,.45);margin-top:calc((var(--track-height) - var(--thumb-size))/2)}input[type=range].range::-moz-range-track{height:var(--track-height);background:var(--track-bg,linear-gradient(90deg,#3b3f52,#3b3f52));border-radius:999px;border:1px solid var(--outline)}input[type=range].range::-moz-range-thumb{width:var(--thumb-size);height:var(--thumb-size);border-radius:50%;border:2px solid rgba(0,0,0,.25);background:var(--thumb-bg,#fff);box-shadow:0 4px 10px rgba(0,0,0,.45)}input[type=range].hue{--track-bg:linear-gradient(to right,hsl(0,100%,50%) 0%,hsl(60,100%,50%) 16.6%,hsl(120,100%,45%) 33.3%,hsl(180,100%,45%) 50%,hsl(240,100%,50%) 66.6%,hsl(300,100%,50%) 83.3%,hsl(360,100%,50%) 100%)}.toast{position:fixed;z-index:999;left:50%;bottom:18px;transform:translateX(-50%) translateY(20px);padding:10px 14px;background:rgba(22,25,34,.88);border:1px solid var(--outline);color:var(--text);border-radius:12px;opacity:0;transition:opacity .2s ease,transform .2s ease;pointer-events:none;font-size:14px}.toast.show{opacity:1;transform:translateX(-50%) translateY(0)}@media (min-width:780px){.preview-card{grid-template-columns:120px 1fr}})css";

// script.js (minified)
static const char SCRIPT_JS[] PROGMEM = R"js("use strict";const $=e=>document.querySelector(e),els={swatch:$("#previewSwatch"),rgbText:$("#rgbText"),hueText:$("#hueText"),brightnessText:$("#brightnessText"),power:$("#powerSwitch"),mode:$("#modeSelect"),hue:$("#hueRange"),hueVal:$("#hueValue"),brightness:$("#brightnessRange"),brightnessVal:$("#brightnessValue"),length:$("#lengthRange"),lengthVal:$("#lengthValue"),syncBtn:$("#syncBtn"),toast:$("#toast")},STATE={hue:0,brightness:128,state:255,mode:0,length:128,color:[255,0,0]};const clamp255=e=>Math.max(0,Math.min(255,0|e));function hsvToRgb255(e,t,a){const n=e%256/255*360,s=clamp255(t)/255,r=clamp255(a)/255;if(!(s>0)){const e=r*255|0;return[e,e,e]}const o=Math.floor(n/60)%6,i=n/60-Math.floor(n/60),l=r*(1-s),c=r*(1-i*s),d=r*(1-(1-i)*s);let u,h,m;switch(o){case 0:u=r,h=d,m=l;break;case 1:u=c,h=r,m=l;break;case 2:u=l,h=r,m=d;break;case 3:u=l,h=c,m=r;break;case 4:u=d,h=l,m=r;break;default:u=r,h=l,m=c}return[clamp255(Math.round(255*u)),clamp255(Math.round(255*h)),clamp255(Math.round(255*m))]}function rgbTupleToCss(e){return`rgb(${e[0]}, ${e[1]}, ${e[2]})`}function powerChecked(){return!!STATE.state}function showToast(e){els.toast&&(els.toast.textContent=e,els.toast.classList.add("show"),setTimeout(()=>els.toast.classList.remove("show"),1400))}function setBrightnessTrack(e){const[t,a,n]=hsvToRgb255(e,255,255);els.brightness.style.setProperty("--track-bg",`linear-gradient(to right, rgb(0,0,0), rgb(${t}, ${a}, ${n}))`)}function setHueThumb(e,t){const[a,n,s]=hsvToRgb255(e,255,t);els.hue.style.setProperty("--thumb-bg",`radial-gradient(circle at 35% 35%, rgba(255,255,255,.9), rgba(255,255,255,.1)), ${rgbTupleToCss([a,n,s])}`)}function renderFromState(){els.power.checked=powerChecked(),els.mode.value=String(STATE.mode),els.hue.value=String(STATE.hue),els.hueVal.value=STATE.hue,els.brightness.value=String(STATE.brightness),els.brightnessVal.value=STATE.brightness,els.length.value=String(STATE.length),els.lengthVal.value=STATE.length;const e=hsvToRgb255(STATE.hue,255,STATE.brightness);STATE.color=e;const t=rgbTupleToCss(e);els.swatch.style.background=t,els.rgbText.textContent=`${e[0]}, ${e[1]}, ${e[2]}`,els.hueText.textContent=STATE.hue,els.brightnessText.textContent=STATE.brightness,setBrightnessTrack(STATE.hue),setHueThumb(STATE.hue,STATE.brightness)}async function apiGet(e){const t=await fetch(e,{cache:"no-store"});if(!t.ok)throw new Error(`${e} -> ${t.status}`);return t.json()}async function apiPost(e,t){const a=await fetch(e,{method:"POST",headers:{"Content-Type":"application/json"},cache:"no-store",body:JSON.stringify(t)});if(!a.ok)throw new Error(`${e} -> ${a.status}`);return a.json()}async function initData(){const e=(await apiGet("/api/modes"))?.modes||[];els.mode.innerHTML=e.map(e=>`<option value="${e.id}">${e.name}</option>`).join("");const t=await apiGet("/api/state");Object.assign(STATE,t),renderFromState()}async function sync_(){try{const e=await apiGet("/api/state");Object.assign(STATE,e),renderFromState(),showToast("Synced from device")}catch(e){console.error(e),showToast("Sync failed")}}window.sync_=sync_;async function commitUpdate(e){try{const t=await apiPost("/api/update",e);Object.assign(STATE,t),renderFromState(),showToast("Updated")}catch(e){console.error(e),showToast("Update failed")}}function attachRangeHandlers(e,t,a,n){["change","mouseup","touchend","pointerup","keyup"].forEach(s=>{e.addEventListener(s,o=>{if("keyup"===o.type&&!["Enter"," ","Spacebar","ArrowLeft","ArrowRight","ArrowUp","ArrowDown"].includes(o.key))return;const s=clamp255(Number(e.value));commitUpdate(n(s))},{passive:!0})}),e.addEventListener("input",()=>{const s=clamp255(Number(e.value));t.value=s,a?.(s)})}document.addEventListener("DOMContentLoaded",async()=>{await initData(),els.power.addEventListener("change",()=>{commitUpdate({state:els.power.checked?255:0})},{passive:!0}),els.mode.addEventListener("change",()=>{commitUpdate({mode:clamp255(Number(els.mode.value))})},{passive:!0}),attachRangeHandlers(els.hue,els.hueVal,e=>{STATE.hue=e,setBrightnessTrack(e),setHueThumb(e,STATE.brightness),renderFromState()},e=>({hue:e})),attachRangeHandlers(els.brightness,els.brightnessVal,e=>{STATE.brightness=e,setHueThumb(STATE.hue,e),renderFromState()},e=>({brightness:e})),attachRangeHandlers(els.length,els.lengthVal,e=>{STATE.length=e},e=>({length:e})),els.syncBtn.addEventListener("click",()=>sync_(),{passive:!0})});)js";

// Modes JSON
static const char MODES_JSON[] PROGMEM = R"json({"modes":[
{"id":0,"name":"Solid"},
{"id":1,"name":"Breathe"},
{"id":2,"name":"Rainbow"},
{"id":3,"name":"Chase"},
{"id":4,"name":"Twinkle"},
{"id":5,"name":"Theater"},
{"id":6,"name":"Meteor"},
{"id":7,"name":"Fire"}
]})json";

// -----------------------------
// Utility: add no-cache headers
// -----------------------------
static inline void add_no_cache(AsyncWebServerRequest* req, AsyncWebServerResponse* res) {
    res->addHeader("Cache-Control", "no-store, max-age=0");
    res->addHeader("Pragma", "no-cache");
    res->addHeader("Expires", "0");
}

// -----------------------------
// Web: Constructor
// -----------------------------
Web::Web(SystemController& controller_ref)
: Interface(controller_ref, "web", "web", false, false, true)
{
    DBG_PRINTLN(Web, "Constructor called.");
}

// -----------------------------
// Web: begin / loop / reset
// -----------------------------
void Web::begin(const ModuleConfig& cfg) {
    (void)cfg; // With -fno-rtti we don't try to downcast; use default port (80).
    DBG_PRINTLN(Web, "begin() called.");
    Module::begin(cfg);

    port_  = 80;

    server_ = new AsyncWebServer(port_);
    events_ = new AsyncEventSource("/events");
    server_->addHandler(events_);

    DefaultHeaders::Instance().addHeader("Cache-Control", "no-store, max-age=0");

    setup_routes_();
    server_->begin();

    recompute_hv_from_color_();

    DBG_PRINTF(Web, "Web server started on port %u\n", port_);
}

void Web::loop() {
    // Async; nothing to poll
}

void Web::reset(bool verbose) {
    (void)verbose;
    DBG_PRINTLN(Web, "reset(): Resetting internal state to defaults.");
    color_      = {255, 0, 0};
    brightness_ = 128;
    state_      = 255;
    mode_       = 0;
    length_     = 128;
    hue_        = 0;
    broadcast_state_sse_();
}

// -----------------------------
// Web: sync_*
// -----------------------------
void Web::sync_color(std::array<uint8_t,3> color) {
    color_ = color;
    recompute_hv_from_color_();
    DBG_PRINTF(Web, "sync_color(): R=%u G=%u B=%u\n", color_[0], color_[1], color_[2]);
    broadcast_state_sse_();
}

void Web::sync_brightness(uint8_t brightness) {
    brightness_ = clamp8_(brightness);
    DBG_PRINTF(Web, "sync_brightness(): %u\n", brightness_);
    broadcast_state_sse_();
}

void Web::sync_state(uint8_t state) {
    state_ = clamp8_(state) >= 128 ? 255 : 0;
    DBG_PRINTF(Web, "sync_state(): %s\n", state_ ? "ON" : "OFF");
    broadcast_state_sse_();
}

void Web::sync_mode(uint8_t mode) {
    mode_ = clamp8_(mode);
    DBG_PRINTF(Web, "sync_mode(): %u\n", mode_);
    broadcast_state_sse_();
}

void Web::sync_length(uint16_t length) {
    length_ = clamp16_(length);
    DBG_PRINTF(Web, "sync_length(): %u\n", static_cast<unsigned>(length_));
    broadcast_state_sse_();
}

void Web::sync_all(std::array<uint8_t,3> color,
                   uint8_t brightness,
                   uint8_t state,
                   uint8_t mode,
                   uint16_t length)
{
    DBG_PRINTLN(Web, "sync_all(): Applying full system state.");
    color_      = { clamp8_(color[0]), clamp8_(color[1]), clamp8_(color[2]) };
    brightness_ = clamp8_(brightness);
    state_      = clamp8_(state) >= 128 ? 255 : 0;
    mode_       = clamp8_(mode);
    length_     = clamp16_(length);
    recompute_hv_from_color_();
    broadcast_state_sse_();
    DBG_PRINTLN(Web, "sync_all(): Done.");
}

// -----------------------------
// Private: routes
// -----------------------------
void Web::setup_routes_() {
    // UI
    server_->on("/", HTTP_GET, [this](AsyncWebServerRequest* req) { send_index_(req); });
    server_->on("/styles.css", HTTP_GET, [this](AsyncWebServerRequest* req) { send_css_(req); });
    server_->on("/script.js", HTTP_GET, [this](AsyncWebServerRequest* req) { send_js_(req); });

    // API
    server_->on("/api/state", HTTP_GET, [this](AsyncWebServerRequest* req) { send_state_json_(req); });
    server_->on("/api/modes", HTTP_GET, [this](AsyncWebServerRequest* req) { send_modes_json_(req); });

    // POST /api/update (JSON)
    server_->on("/api/update", HTTP_POST,
        [](AsyncWebServerRequest* req) {
            // response is sent from the upload/body handler
        },
        nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            handle_update_body_(req, data, len, index, total);
        }
    );
}

// -----------------------------
// Private: asset senders (PROGMEM)
// -----------------------------
void Web::send_index_(AsyncWebServerRequest* req) {
    AsyncWebServerResponse* res = req->beginResponse_P(
        200,
        F("text/html; charset=utf-8"),
        reinterpret_cast<const uint8_t*>(INDEX_HTML),
        strlen_P(INDEX_HTML)
    );
    add_no_cache(req, res);
    req->send(res);
}
void Web::send_css_(AsyncWebServerRequest* req) {
    AsyncWebServerResponse* res = req->beginResponse_P(
        200,
        F("text/css; charset=utf-8"),
        reinterpret_cast<const uint8_t*>(STYLES_CSS),
        strlen_P(STYLES_CSS)
    );
    add_no_cache(req, res);
    req->send(res);
}
void Web::send_js_(AsyncWebServerRequest* req) {
    AsyncWebServerResponse* res = req->beginResponse_P(
        200,
        F("application/javascript; charset=utf-8"),
        reinterpret_cast<const uint8_t*>(SCRIPT_JS),
        strlen_P(SCRIPT_JS)
    );
    add_no_cache(req, res);
    req->send(res);
}

// -----------------------------
// Private: JSON endpoints
// -----------------------------
void Web::send_state_json_(AsyncWebServerRequest* req) {
    StaticJsonDocument<256> doc;
    doc["hue"]        = hue_;
    doc["brightness"] = brightness_;
    doc["state"]      = state_ ? 255 : 0;
    doc["mode"]       = mode_;
    doc["length"]     = (length_ > 255) ? 255 : static_cast<uint8_t>(length_);
    JsonArray col     = doc.createNestedArray("color");
    col.add(color_[0]); col.add(color_[1]); col.add(color_[2]);

    String out;
    serializeJson(doc, out);
    AsyncWebServerResponse* res = req->beginResponse(200, F("application/json"), out);
    add_no_cache(req, res);
    req->send(res);
}

void Web::send_modes_json_(AsyncWebServerRequest* req) {
    AsyncWebServerResponse* res = req->beginResponse_P(
        200,
        F("application/json"),
        reinterpret_cast<const uint8_t*>(MODES_JSON),
        strlen_P(MODES_JSON)
    );
    add_no_cache(req, res);
    req->send(res);
}

// -----------------------------
// Private: POST /api/update body handler
// -----------------------------
void Web::handle_update_body_(AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
    static String body;
    if (index == 0) body = "";
    body.reserve(total);
    body.concat(reinterpret_cast<const char*>(data), len);

    if (index + len < total) return; // wait until complete

    StaticJsonDocument<384> doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        DBG_PRINTF(Web, "JSON parse error: %s\n", err.c_str());
        req->send(400, F("application/json"), F("{\"error\":\"bad json\"}"));
        return;
    }

    bool changed = false;

    // 1) STATE (coerce to 0/255)
    if (doc.containsKey("state")) {
        uint8_t s = clamp8_(doc["state"].as<int>());
        state_ = (s >= 128) ? 255 : 0;
        controller.sync_state(state_, {true, true, false, true, true});
        changed = true;
    }

    // 2) MODE
    if (doc.containsKey("mode")) {
        mode_ = clamp8_(doc["mode"].as<int>());
        controller.sync_mode(mode_, {true, true, false, true, true});
        changed = true;
    }

    // 3) LENGTH (wire 0..255 → controller uint16_t)
    if (doc.containsKey("length")) {
        length_ = clamp16_(doc["length"].as<int>());
        if (length_ > 255) length_ = 255; // wire guarantee
        controller.sync_length(length_, {true, true, false, true, true});
        changed = true;
    }

    // 4) COLOR (derive hue/brightness from RGB)
    if (doc.containsKey("color")) {
        JsonArray c = doc["color"].as<JsonArray>();
        if (c && c.size() == 3) {
            color_[0] = clamp8_(c[0].as<int>());
            color_[1] = clamp8_(c[1].as<int>());
            color_[2] = clamp8_(c[2].as<int>());
            uint8_t h, s, v;
            rgb255_to_hsv_(color_[0], color_[1], color_[2], h, s, v);
            hue_ = h;
            brightness_ = v;
            controller.sync_color(color_, {true, true, false, true, true});
            controller.sync_brightness(brightness_, {true, true, false, true, true});
            changed = true;
        }
    }

    // 5) HUE (recompute color from hue + current brightness)
    if (doc.containsKey("hue")) {
        hue_ = clamp8_(doc["hue"].as<int>());
        uint8_t r, g, b;
        hsv_to_rgb255_(hue_, 255, brightness_, r, g, b);
        color_ = { r, g, b };
        controller.sync_color(color_, {true, true, false, true, true});
        changed = true;
    }

    // 6) BRIGHTNESS
    if (doc.containsKey("brightness")) {
        brightness_ = clamp8_(doc["brightness"].as<int>());
        uint8_t r, g, b;
        hsv_to_rgb255_(hue_, 255, brightness_, r, g, b);
        color_ = { r, g, b };
        controller.sync_brightness(brightness_, {true, true, false, true, true});
        controller.sync_color(color_, {true, true, false, true, true});
        changed = true;
    }

    if (changed) {
        broadcast_state_sse_();
    }

    // Respond with canonical full state (0–255 on wire)
    StaticJsonDocument<256> resp;
    resp["hue"]        = hue_;
    resp["brightness"] = brightness_;
    resp["state"]      = state_ ? 255 : 0;
    resp["mode"]       = mode_;
    resp["length"]     = (length_ > 255) ? 255 : static_cast<uint8_t>(length_);
    JsonArray col      = resp.createNestedArray("color");
    col.add(color_[0]); col.add(color_[1]); col.add(color_[2]);

    String out;
    serializeJson(resp, out);
    AsyncWebServerResponse* res = req->beginResponse(200, F("application/json"), out);
    add_no_cache(req, res);
    req->send(res);
}

// -----------------------------
// Private: math & state helpers
// -----------------------------
uint8_t Web::clamp8_(int v)   { return (v < 0) ? 0 : (v > 255 ? 255 : (uint8_t)v); }
uint16_t Web::clamp16_(int v) { if (v < 0) return 0; if (v > 0xFFFF) return 0xFFFF; return (uint16_t)v; }

void Web::hsv_to_rgb255_(uint8_t h255, uint8_t s255, uint8_t v255, uint8_t& r, uint8_t& g, uint8_t& b) {
    float h = (h255 % 256) / 255.0f * 360.0f;
    float s = clamp8_(s255) / 255.0f;
    float v = clamp8_(v255) / 255.0f;

    if (s <= 0.0f) {
        uint8_t c = (uint8_t)roundf(v * 255.0f);
        r = g = b = c; return;
    }
    h = fmodf(h, 360.0f);
    int   hi = (int)floorf(h / 60.0f) % 6;
    float f  = (h / 60.0f) - floorf(h / 60.0f);
    float p  = v * (1.0f - s);
    float q  = v * (1.0f - f * s);
    float t  = v * (1.0f - (1.0f - f) * s);

    float rf, gf, bf;
    switch (hi) {
        case 0: rf = v; gf = t; bf = p; break;
        case 1: rf = q; gf = v; bf = p; break;
        case 2: rf = p; gf = v; bf = t; break;
        case 3: rf = p; gf = q; bf = v; break;
        case 4: rf = t; gf = p; bf = v; break;
        default: rf = v; gf = p; bf = q; break;
    }
    r = clamp8_((int)roundf(rf * 255.0f));
    g = clamp8_((int)roundf(gf * 255.0f));
    b = clamp8_((int)roundf(bf * 255.0f));
}

void Web::rgb255_to_hsv_(uint8_t r, uint8_t g, uint8_t b, uint8_t& h255, uint8_t& s255, uint8_t& v255) {
    float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f;
    float mx = fmaxf(fmaxf(rf, gf), bf);
    float mn = fminf(fminf(rf, gf), bf);
    float d  = mx - mn;

    float h;
    if (d == 0.0f)      h = 0.0f;
    else if (mx == rf)  h = fmodf((60.0f * ((gf - bf) / d) + 360.0f), 360.0f);
    else if (mx == gf)  h = 60.0f * ((bf - rf) / d) + 120.0f;
    else                h = 60.0f * ((rf - gf) / d) + 240.0f;

    float s = (mx == 0.0f) ? 0.0f : (d / mx);
    float v = mx;

    h255 = clamp8_((int)roundf(h / 360.0f * 255.0f));
    s255 = clamp8_((int)roundf(s * 255.0f));
    v255 = clamp8_((int)roundf(v * 255.0f));
}

void Web::recompute_color_from_hv_() {
    uint8_t r, g, b;
    hsv_to_rgb255_(hue_, 255, brightness_, r, g, b);
    color_ = { r, g, b };
}

void Web::recompute_hv_from_color_() {
    uint8_t h, s, v;
    rgb255_to_hsv_(color_[0], color_[1], color_[2], h, s, v);
    hue_        = h;
    brightness_ = v;
}

void Web::build_state_json_string_(String& out) const {
    StaticJsonDocument<256> doc;
    doc["hue"]        = hue_;
    doc["brightness"] = brightness_;
    doc["state"]      = state_ ? 255 : 0;
    doc["mode"]       = mode_;
    doc["length"]     = (length_ > 255) ? 255 : static_cast<uint8_t>(length_);
    JsonArray col     = doc.createNestedArray("color");
    col.add(color_[0]); col.add(color_[1]); col.add(color_[2]);
    serializeJson(doc, out);
}

void Web::broadcast_state_sse_() {
    if (!events_) return;
    String payload; payload.reserve(128);
    build_state_json_string_(payload);
    events_->send(payload.c_str(), "state", millis());
}
