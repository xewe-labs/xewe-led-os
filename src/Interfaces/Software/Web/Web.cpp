#include "Web.h"
#include "../../../SystemController/SystemController.h"

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <pgmspace.h>

// Access LED strip API
#include "../../Hardware/LedStrip/LedStrip.h"

// ----------------------------------------------------
// Embedded UI (PROGMEM) – JS now does ALL HSV; backend is RGB-only.
// ----------------------------------------------------

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
<li>Hue track renders red→spectrum→red.</li>
<li>UI commits on touch/drag end; press <b>Sync</b> or call <code>sync_()</code> to pull.</li>
</ul></details></section></main>
<div id="toast" class="toast" role="status" aria-live="polite"></div>
<script src="/script.js" defer></script></body></html>)html";

// styles.css (minified)
static const char STYLES_CSS[] PROGMEM = R"css(:root{--bg:#0e0f12;--surface:#171922;--surface-2:#1f2230;--text:#e6e8ef;--muted:#a6adbb;--accent:#4da3ff;--outline:#2b2f3d;--radius:14px;--shadow:0 6px 26px rgba(0,0,0,.35);--thumb-size:28px;--track-height:14px}*{box-sizing:border-box}html,body{height:100%}body{margin:0;font:16px/1.4 system-ui,-apple-system,Segoe UI,Roboto,Helvetica Neue,Arial,sans-serif;color:var(--text);background:radial-gradient(1200px 800px at 100% -20%,#131625 0%,var(--bg) 55%);-webkit-font-smoothing:antialiased}.appbar{position:sticky;top:0;display:flex;align-items:center;justify-content:space-between;padding:16px clamp(16px,5vw,28px);background:linear-gradient(180deg,rgba(12,13,18,.75) 0%,rgba(12,13,18,.4) 100%);backdrop-filter:blur(10px);border-bottom:1px solid var(--outline);z-index:10}.appbar h1{margin:0;font-size:18px;letter-spacing:.4px}.actions{display:flex;gap:10px}.container{padding:18px clamp(16px,5vw,28px) 40px;max-width:720px;margin:0 auto;display:grid;gap:16px}.card{background:var(--surface);border:1px solid var(--outline);border-radius:var(--radius);box-shadow:var(--shadow);padding:14px}.preview-card{display:grid;grid-template-columns:96px 1fr;gap:14px;align-items:center}.preview-swatch{width:96px;height:96px;border-radius:16px;border:1px solid var(--outline);background:#000;box-shadow:inset 0 0 0 1px rgba(255,255,255,.05),0 10px 24px rgba(0,0,0,.6)}.preview-meta{color:var(--muted);display:grid;gap:6px;font-size:14px}.preview-meta strong{color:var(--text);font-weight:600;margin-right:6px}.controls .row{display:grid;grid-template-columns:120px 1fr;align-items:center;gap:12px;padding:10px 8px;border-radius:10px}.controls .row+.row{border-top:1px dashed var(--outline)}.controls label{color:var(--muted);font-size:14px}.btn{padding:10px 14px;font-weight:600;border-radius:999px;border:1px solid var(--outline);background:var(--surface-2);color:var(--text)}.btn.secondary{background:transparent}.btn:active{transform:translateY(1px)}.select{width:100%;padding:12px 14px;border-radius:12px;border:1px solid var(--outline);background:var(--surface-2);color:var(--text);appearance:none}.switch{position:relative;display:inline-block;width:60px;height:34px}.switch input{display:none}.switch .slider{position:absolute;cursor:pointer;inset:0;background:#2a2f3b;border-radius:999px;border:1px solid var(--outline);transition:background .2s ease,box-shadow .2s ease}.switch .slider:before{content:"";position:absolute;height:26px;width:26px;left:4px;top:3px;background:linear-gradient(180deg,#fff,#cfd3da);border-radius:50%;box-shadow:0 2px 8px rgba(0,0,0,.4);transition:transform .22s cubic-bezier(.2,.7,.2,1)}.switch input:checked+.slider{background:linear-gradient(90deg,#1f6fff,#6cc8ff)}.switch input:checked+.slider:before{transform:translateX(26px)}.range-wrap{position:relative;display:grid;align-items:center}.bubble{position:absolute;right:0;top:-28px;font-size:12px;color:var(--muted);background:transparent;padding:0 4px}input[type=range].range{-webkit-appearance:none;appearance:none;width:100%;height:var(--thumb-size);background:transparent;margin:8px 0;touch-action:none}input[type=range].range::-webkit-slider-runnable-track{height:var(--track-height);background:var(--track-bg,linear-gradient(90deg,#3b3f52,#3b3f52));border-radius:999px;border:1px solid var(--outline)}input[type=range].range::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:var(--thumb-size);height:var(--thumb-size);border-radius:50%;border:2px solid rgba(0,0,0,.25);background:var(--thumb-bg,#fff);box-shadow:0 4px 10px rgba(0,0,0,.45);margin-top:calc((var(--track-height) - var(--thumb-size))/2)}input[type=range].range::-moz-range-track{height:var(--track-height);background:var(--track-bg,linear-gradient(90deg,#3b3f52,#3b3f52));border-radius:999px;border:1px solid var(--outline)}input[type=range].range::-moz-range-thumb{width:var(--thumb-size);height:var(--thumb-size);border-radius:50%;border:2px solid rgba(0,0,0,.25);background:var(--thumb-bg,#fff);box-shadow:0 4px 10px rgba(0,0,0,.45)}input[type=range].hue{--track-bg:linear-gradient(to right,hsl(0,100%,50%) 0%,hsl(60,100%,50%) 16.6%,hsl(120,100%,45%) 33.3%,hsl(180,100%,45%) 50%,hsl(240,100%,50%) 66.6%,hsl(300,100%,50%) 83.3%,hsl(360,100%,50%) 100%)}.toast{position:fixed;z-index:999;left:50%;bottom:18px;transform:translateX(-50%) translateY(20px);padding:10px 14px;background:rgba(22,25,34,.88);border:1px solid var(--outline);color:var(--text);border-radius:12px;opacity:0;transition:opacity .2s ease,transform .2s ease;pointer-events:none;font-size:14px}.toast.show{opacity:1;transform:translateX(-50%) translateY(0)}@media (min-width:780px){.preview-card{grid-template-columns:120px 1fr}})css";

// script.js – ALL HSV lives here; backend is RGB-only.
static const char SCRIPT_JS[] PROGMEM = R"js("use strict";
const $ = (sel) => document.querySelector(sel);

/* ===== DOM refs ===== */
const els = {
  swatch: $("#previewSwatch"),
  rgbText: $("#rgbText"),
  hueText: $("#hueText"),
  brightnessText: $("#brightnessText"),
  power: $("#powerSwitch"),
  mode: $("#modeSelect"),
  hue: $("#hueRange"),
  hueVal: $("#hueValue"),
  brightness: $("#brightnessRange"),
  brightnessVal: $("#brightnessValue"),
  length: $("#lengthRange"),
  lengthVal: $("#lengthValue"),
  syncBtn: $("#syncBtn"),
  toast: $("#toast"),
};

/* ===== Client state (mirrors server + derived hue) ===== */
const STATE = { hue: 0, brightness: 128, state: 255, mode: 0, length: 128, color: [255,0,0] };

/* Prevent UI jumps while the user is interacting (iOS fix) */
const LOCK = { hue:false, brightness:false, length:false, mode:false, power:false };

let es;
let esDelay = 1000;

/* ===== Helpers ===== */
const clamp255 = (x) => Math.max(0, Math.min(255, x|0));

function hsvToRgb255(h255, s255, v255) {
  const h = ((h255 % 256) / 255) * 360, s = clamp255(s255)/255, v = clamp255(v255)/255;
  if (s <= 0) { const c = (v * 255) | 0; return [c,c,c]; }
  const i = Math.floor(h/60)%6, f = h/60 - Math.floor(h/60);
  const p = v*(1-s), q = v*(1-f*s), t = v*(1-(1-f)*s);
  let r,g,b;
  switch(i){case 0:r=v;g=t;b=p;break;case 1:r=q;g=v;b=p;break;case 2:r=p;g=v;b=t;break;case 3:r=p;g=q;b=v;break;case 4:r=t;g=p;b=v;break;default:r=v;g=p;b=q;}
  return [clamp255(Math.round(r*255)), clamp255(Math.round(g*255)), clamp255(Math.round(b*255))];
}

// Minimal rgb->hsv (0..255 space) to derive hue from RGB coming from server
function rgbToHsv255(r, g, b) {
  const rf=r/255, gf=g/255, bf=b/255;
  const max=Math.max(rf,gf,bf), min=Math.min(rf,gf,bf), d=max-min;
  let h=0, s=max===0?0:d/max, v=max;
  if (d!==0){
    switch(max){
      case rf: h=((gf-bf)/d + (gf<bf?6:0)); break;
      case gf: h=((bf-rf)/d + 2); break;
      default: h=((rf-gf)/d + 4); break;
    }
    h *= 60;
  }
  return [clamp255(Math.round(h/360*255)), clamp255(Math.round(s*255)), clamp255(Math.round(v*255))];
}

const rgbToCss = ([r,g,b]) => `rgb(${r}, ${g}, ${b})`;
const showToast = (msg) => { if (!els.toast) return; els.toast.textContent = msg; els.toast.classList.add("show"); setTimeout(()=>els.toast.classList.remove("show"), 1200); };

function setBrightnessTrack(h255) {
  const [r,g,b] = hsvToRgb255(h255, 255, 255);
  els.brightness.style.setProperty("--track-bg", `linear-gradient(to right, rgb(0,0,0), rgb(${r}, ${g}, ${b}))`);
}
function setHueThumb(h255, v255) {
  const [r,g,b] = hsvToRgb255(h255, 255, v255);
  els.hue.style.setProperty("--thumb-bg", `radial-gradient(circle at 35% 35%, rgba(255,255,255,.9), rgba(255,255,255,.1)), ${rgbToCss([r,g,b])}`);
}

/* Merge server payload (no hue provided) and derive hue from color if needed */
function mergeStateFromServer(s) {
  if (!s || typeof s !== "object") return;
  if (!LOCK.power && typeof s.state === "number") STATE.state = s.state ? 255 : 0;
  if (!LOCK.mode  && typeof s.mode  === "number") STATE.mode  = clamp255(s.mode);
  if (!LOCK.length&& typeof s.length=== "number") STATE.length= clamp255(s.length);
  if (!LOCK.brightness && typeof s.brightness === "number") STATE.brightness = clamp255(s.brightness);
  if (Array.isArray(s.color) && s.color.length === 3) {
    const rgb = s.color.map(clamp255);
    STATE.color = rgb;
    if (!LOCK.hue) {
      const [h] = rgbToHsv255(rgb[0], rgb[1], rgb[2]);
      STATE.hue = h;
    }
  }
}

/* Render (respecting locks) */
function renderAll() {
  if (!LOCK.power)       els.power.checked = !!STATE.state;
  if (!LOCK.mode)        els.mode.value = String(STATE.mode);

  if (!LOCK.hue) {
    els.hue.value = String(STATE.hue);
    els.hueVal.value = STATE.hue;
    setBrightnessTrack(STATE.hue);
    setHueThumb(STATE.hue, STATE.brightness);
  }
  if (!LOCK.brightness) {
    els.brightness.value = String(STATE.brightness);
    els.brightnessVal.value = STATE.brightness;
    setHueThumb(STATE.hue, STATE.brightness);
  }
  if (!LOCK.length) {
    els.length.value = String(STATE.length);
    els.lengthVal.value = STATE.length;
  }

  const rgb = (Array.isArray(STATE.color) && STATE.color.length===3)
    ? STATE.color
    : hsvToRgb255(STATE.hue, 255, STATE.brightness);
  els.swatch.style.background = rgbToCss(rgb);
  els.rgbText.textContent = `${rgb[0]}, ${rgb[1]}, ${rgb[2]}`;
  els.hueText.textContent = STATE.hue;
  els.brightnessText.textContent = STATE.brightness;
}

/* ===== Networking ===== */
async function apiGet(path){ const r=await fetch(path,{cache:"no-store"}); if(!r.ok) throw new Error(`${path} -> ${r.status}`); return r.json(); }
async function apiPost(path,payload){ const r=await fetch(path,{method:"POST",headers:{"Content-Type":"application/json"},cache:"no-store",body:JSON.stringify(payload)}); if(!r.ok) throw new Error(`${path} -> ${r.status}`); return r.json(); }

async function initData(){
  const modes = (await apiGet("/api/modes"))?.modes || [];
  els.mode.innerHTML = modes.map(m=>`<option value="${m.id}">${m.name}</option>`).join("");
  const s = await apiGet("/api/state");
  mergeStateFromServer(s);
  renderAll();
  connectSSE();
}

async function sync_(){
  try{ mergeStateFromServer(await apiGet("/api/state")); renderAll(); showToast("Synced"); }
  catch(e){ console.error(e); showToast("Sync failed"); }
}
window.sync_ = sync_;

/* Commit helpers: backend expects RGB + brightness, no hue */
async function commitUpdate(partial){
  try{ mergeStateFromServer(await apiPost("/api/update", partial)); renderAll(); showToast("Updated"); }
  catch(e){ console.error(e); showToast("Update failed"); }
}

/* ===== SSE ===== */
function connectSSE(){
  try{ if (es) es.close(); }catch(e){}
  es = new EventSource("/events");
  es.addEventListener("open", ()=>{ esDelay = 1000; });
  es.addEventListener("error", ()=>{ try{es.close();}catch(e){}; setTimeout(connectSSE, esDelay); esDelay = Math.min(10000, esDelay*2); });
  es.addEventListener("state", (ev)=>{
    try{ const s = JSON.parse(ev.data); mergeStateFromServer(s); renderAll(); }
    catch(e){ console.error("SSE parse error", e); }
  });
}

/* ===== Gesture handling with per-control locks ===== */
function attachRange(inputEl, bubbleEl, key, onLive, toPayload){
  const lock = ()=>{ LOCK[key] = true; };
  const unlockSoon = ()=> setTimeout(()=>{ LOCK[key] = false; }, 50);

  inputEl.addEventListener("pointerdown", lock, {passive:true});
  inputEl.addEventListener("touchstart",  lock, {passive:true});
  inputEl.addEventListener("mousedown",   lock, {passive:true});

  inputEl.addEventListener("input", ()=>{
    const v = clamp255(Number(inputEl.value));
    bubbleEl.value = v;
    onLive?.(v);
  }, {passive:true});

  const commit = ()=>{ const v = clamp255(Number(inputEl.value)); commitUpdate(toPayload(v)).finally(unlockSoon); };
  inputEl.addEventListener("change",    commit, {passive:true});
  inputEl.addEventListener("pointerup", commit, {passive:true});
  inputEl.addEventListener("touchend",  commit, {passive:true});
  inputEl.addEventListener("mouseup",   commit, {passive:true});
}

/* ===== Wire up ===== */
document.addEventListener("DOMContentLoaded", initData);
document.addEventListener("visibilitychange", ()=>{ if (document.visibilityState === "visible") sync_(); });
window.addEventListener("focus", ()=> sync_());
window.addEventListener("pageshow", ()=> sync_());

if (els.syncBtn) els.syncBtn.addEventListener("click", ()=> sync_(), {passive:true});

if (els.power) els.power.addEventListener("change", ()=>{
  LOCK.power = true;
  commitUpdate({state: els.power.checked ? 255 : 0}).finally(()=>{ setTimeout(()=>{ LOCK.power=false; },50); });
}, {passive:true});

if (els.mode) els.mode.addEventListener("change", ()=>{
  LOCK.mode = true;
  commitUpdate({mode: clamp255(Number(els.mode.value))}).finally(()=>{ setTimeout(()=>{ LOCK.mode=false; },50); });
}, {passive:true});

/* Hue slider:
 * - Live: update local preview only.
 * - Commit: compute RGB from (hue, STATE.brightness) and send {color:[r,g,b]}.
 */
attachRange(
  els.hue, els.hueVal, "hue",
  (v)=>{ STATE.hue = v;
         const rgb = hsvToRgb255(v,255,STATE.brightness);
         STATE.color = rgb;
         els.swatch.style.background = rgbToCss(rgb);
         els.rgbText.textContent = `${rgb[0]}, ${rgb[1]}, ${rgb[2]}`;
         els.hueText.textContent = v;
         setBrightnessTrack(v);
         setHueThumb(v, STATE.brightness); },
  (v)=>{ const rgb = hsvToRgb255(v,255,STATE.brightness); return { color: rgb }; }
);

/* Brightness slider:
 * - Live: preview.
 * - Commit: send BOTH brightness and color to preserve previous semantics.
 */
attachRange(
  els.brightness, els.brightnessVal, "brightness",
  (v)=>{ STATE.brightness = v;
         const rgb = hsvToRgb255(STATE.hue,255,v);
         STATE.color = rgb;
         els.swatch.style.background = rgbToCss(rgb);
         els.rgbText.textContent = `${rgb[0]}, ${rgb[1]}, ${rgb[2]}`;
         els.brightnessText.textContent = v;
         setHueThumb(STATE.hue, v); },
  (v)=>{ const rgb = hsvToRgb255(STATE.hue,255,v); return { brightness: v, color: rgb }; }
);

// Length slider
attachRange(
  els.length, els.lengthVal, "length",
  (v)=>{ STATE.length = v; },
  (v)=>({length:v})
);
)js";

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

// ----------------------------------------------------
// Utility: add no-cache headers
// ----------------------------------------------------
static inline void add_no_cache(AsyncWebServerRequest* req, AsyncWebServerResponse* res) {
    res->addHeader("Cache-Control", "no-store, max-age=0");
    res->addHeader("Pragma", "no-cache");
    res->addHeader("Expires", "0");
}

// ----------------------------------------------------
// Web lifecycle
// ----------------------------------------------------
Web::Web(SystemController& controller_ref)
: Interface(controller_ref, "web", "web", false, false, true) {
    DBG_PRINTLN(Web, "Constructor called.");
}

void Web::begin(const ModuleConfig& cfg) {
    (void)cfg; // -fno-rtti
    DBG_PRINTLN(Web, "begin() called.");
    Module::begin(cfg);

    port_  = 80;

    server_ = new AsyncWebServer(port_);
    events_ = new AsyncEventSource("/events");
    server_->addHandler(events_);

    DefaultHeaders::Instance().addHeader("Cache-Control", "no-store, max-age=0");

    setup_routes_();
    server_->begin();
    DBG_PRINTF(Web, "Web server started on port %u\n", port_);
}

void Web::loop() { /* async */ }

void Web::reset(bool verbose) {
    (void)verbose;
    DBG_PRINTLN(Web, "reset(): broadcasting current state.");
    broadcast_state_sse_();
}

// ----------------------------------------------------
// sync_*() — broadcast only (so all UIs update)
// ----------------------------------------------------
void Web::sync_color(std::array<uint8_t,3> /*color*/)  { broadcast_state_sse_(); }
void Web::sync_brightness(uint8_t /*brightness*/)      { broadcast_state_sse_(); }
void Web::sync_state(uint8_t /*state*/)                { broadcast_state_sse_(); }
void Web::sync_mode(uint8_t /*mode*/)                  { broadcast_state_sse_(); }
void Web::sync_length(uint16_t /*length*/)             { broadcast_state_sse_(); }
void Web::sync_all(std::array<uint8_t,3> /*color*/, uint8_t /*brightness*/, uint8_t /*state*/, uint8_t /*mode*/, uint16_t /*length*/) {
    broadcast_state_sse_();
}

// ----------------------------------------------------
// Routes
// ----------------------------------------------------
void Web::setup_routes_() {
    // UI
    server_->on("/",           HTTP_GET, [this](AsyncWebServerRequest* req){ send_index_(req); });
    server_->on("/styles.css", HTTP_GET, [this](AsyncWebServerRequest* req){ send_css_(req); });
    server_->on("/script.js",  HTTP_GET, [this](AsyncWebServerRequest* req){ send_js_(req); });

    // API
    server_->on("/api/state",  HTTP_GET, [this](AsyncWebServerRequest* req){ send_state_json_(req); });
    server_->on("/api/modes",  HTTP_GET, [this](AsyncWebServerRequest* req){ send_modes_json_(req); });

    // POST /api/update (JSON body)
    server_->on("/api/update", HTTP_POST,
        [](AsyncWebServerRequest* req){ /* response sent in body handler */ },
        nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total){
            handle_update_body_(req, data, len, index, total);
        }
    );
}

// ----------------------------------------------------
// Asset senders (PROGMEM)
// ----------------------------------------------------
void Web::send_index_(AsyncWebServerRequest* req) {
    AsyncWebServerResponse* res = req->beginResponse_P(
        200, F("text/html; charset=utf-8"),
        reinterpret_cast<const uint8_t*>(INDEX_HTML),
        strlen_P(INDEX_HTML)
    );
    add_no_cache(req, res);
    req->send(res);
}
void Web::send_css_(AsyncWebServerRequest* req) {
    AsyncWebServerResponse* res = req->beginResponse_P(
        200, F("text/css; charset=utf-8"),
        reinterpret_cast<const uint8_t*>(STYLES_CSS),
        strlen_P(STYLES_CSS)
    );
    add_no_cache(req, res);
    req->send(res);
}
void Web::send_js_(AsyncWebServerRequest* req) {
    AsyncWebServerResponse* res = req->beginResponse_P(
        200, F("application/javascript; charset=utf-8"),
        reinterpret_cast<const uint8_t*>(SCRIPT_JS),
        strlen_P(SCRIPT_JS)
    );
    add_no_cache(req, res);
    req->send(res);
}

// ----------------------------------------------------
// /api/state – live values from controller.led_strip (RGB only + brightness)
// ----------------------------------------------------
void Web::send_state_json_(AsyncWebServerRequest* req) {
    const auto rgb = controller.led_strip.get_rgb();

    StaticJsonDocument<256> doc;
    doc["brightness"] = controller.led_strip.get_brightness();          // 0..255
    doc["state"]      = controller.led_strip.get_state() ? 255 : 0;     // 0/255
    doc["mode"]       = controller.led_strip.get_mode_id();             // 0..255
    {
        uint16_t L = controller.led_strip.get_length();
        doc["length"] = (L > 255) ? 255 : static_cast<uint8_t>(L);      // wire limited to 0..255
    }
    JsonArray col = doc.createNestedArray("color");
    col.add(rgb[0]); col.add(rgb[1]); col.add(rgb[2]);

    String out; serializeJson(doc, out);
    AsyncWebServerResponse* res = req->beginResponse(200, F("application/json"), out);
    add_no_cache(req, res);
    req->send(res);
}

void Web::send_modes_json_(AsyncWebServerRequest* req) {
    AsyncWebServerResponse* res = req->beginResponse_P(
        200, F("application/json"),
        reinterpret_cast<const uint8_t*>(MODES_JSON),
        strlen_P(MODES_JSON)
    );
    add_no_cache(req, res);
    req->send(res);
}

// ----------------------------------------------------
// POST /api/update – accepts only RGB, brightness, state, mode, length
// ----------------------------------------------------
void Web::handle_update_body_(AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
    static String body;
    if (index == 0) body = "";
    body.reserve(total);
    body.concat(reinterpret_cast<const char*>(data), len);
    if (index + len < total) return;

    StaticJsonDocument<384> doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        DBG_PRINTF(Web, "JSON parse error: %s\n", err.c_str());
        req->send(400, F("application/json"), F("{\"error\":\"bad json\"}"));
        return;
    }

    bool changed = false;

    // STATE (0/255)
    if (doc.containsKey("state")) {
        uint8_t s = clamp8_(doc["state"].as<int>());
        controller.led_strip.set_state(s >= 128 ? 255 : 0);
        changed = true;
    }

    // MODE
    if (doc.containsKey("mode")) {
        uint8_t m = clamp8_(doc["mode"].as<int>());
        controller.led_strip.set_mode(m);
        changed = true;
    }

    // LENGTH (limit to 0..255 on wire)
    if (doc.containsKey("length")) {
        uint16_t L = clamp16_(doc["length"].as<int>());
        if (L > 255) L = 255;
        controller.led_strip.set_length(L);
        changed = true;
    }

    // COLOR (RGB)
    if (doc.containsKey("color")) {
        JsonArray c = doc["color"].as<JsonArray>();
        if (c && c.size() == 3) {
            std::array<uint8_t,3> rgb {
                clamp8_(c[0].as<int>()),
                clamp8_(c[1].as<int>()),
                clamp8_(c[2].as<int>())
            };
            controller.led_strip.set_rgb(rgb);
            changed = true;
        }
    }

    // BRIGHTNESS (0..255)
    if (doc.containsKey("brightness")) {
        uint8_t v = clamp8_(doc["brightness"].as<int>());
        controller.led_strip.set_brightness(v);
        changed = true;
    }

    if (changed) {
        broadcast_state_sse_();
    }

    // Respond with canonical full state (RGB-only + brightness)
    String out; build_state_json_string_(out);
    AsyncWebServerResponse* res = req->beginResponse(200, F("application/json"), out);
    add_no_cache(req, res);
    req->send(res);
}

// ----------------------------------------------------
// Helpers
// ----------------------------------------------------
uint8_t Web::clamp8_(int v)   { return (v < 0) ? 0 : (v > 255 ? 255 : (uint8_t)v); }
uint16_t Web::clamp16_(int v) { if (v < 0) return 0; if (v > 0xFFFF) return 0xFFFF; return (uint16_t)v; }

void Web::build_state_json_string_(String& out) const {
    const auto rgb = controller.led_strip.get_rgb();

    StaticJsonDocument<256> doc;
    doc["brightness"] = controller.led_strip.get_brightness();
    doc["state"]      = controller.led_strip.get_state() ? 255 : 0;
    doc["mode"]       = controller.led_strip.get_mode_id();
    {
        uint16_t L = controller.led_strip.get_length();
        doc["length"] = (L > 255) ? 255 : static_cast<uint8_t>(L);
    }
    JsonArray col = doc.createNestedArray("color");
    col.add(rgb[0]); col.add(rgb[1]); col.add(rgb[2]);

    serializeJson(doc, out);
}

void Web::broadcast_state_sse_() {
    if (!events_) return;
    String payload; payload.reserve(160);
    build_state_json_string_(payload);
    events_->send(payload.c_str(), "state", millis());
}
