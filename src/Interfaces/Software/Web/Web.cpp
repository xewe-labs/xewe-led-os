// Software/Web/Web.cpp
#include "Web.h"
#include "../../../SystemController/SystemController.h"

#if !defined(ESP32)
  #error "This Web interface builds only for ESP32."
#endif

#include <Arduino.h>
#include "JsonUtils.h"
#include "MainHtml.h"
#include "MainCss.h"

using xewe::str::replace_all;
using xewe::json::make_color_patch_json;
using xewe::json::make_brightness_patch_json;
using xewe::json::make_power_patch_json;
using xewe::json::make_mode_patch_json;
using xewe::json::make_full_state_json;
using xewe::json::extract_number;
using xewe::json::extract_bool;
using xewe::json::extract_string;
using xewe::json::extract_rgb_array;
using xewe::json::clamp_u8;

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

    if (!cache_mutex_) {
        cache_mutex_ = xSemaphoreCreateMutex();
        if (!cache_mutex_) {
            DBG_PRINTLN(Web, "FATAL: cache_mutex create failed");
        } else {
            DBG_PRINTLN(Web, "cache_mutex created");
        }
    }

    DBG_PRINTF(Web, "WiFi status=%d, IP=%s\n", WiFi.status(), WiFi.localIP().toString().c_str());

    // ------------------------- Routes -------------------------

    // GET /
    server_.on("/", HTTP_GET, [this](AsyncWebServerRequest* req){
        DBG_PRINTF(Web, "HTTP GET %s\n", req->url().c_str());
        std::string html = render_index();
        AsyncWebServerResponse* res = req->beginResponse(200, "text/html; charset=utf-8", html.c_str());
        res->addHeader("Cache-Control", "no-store");
        req->send(res);
    });

    // Placeholder advanced
    server_.on("/advanced", HTTP_GET, [](AsyncWebServerRequest* req){
        req->send(200, "text/plain; charset=utf-8", "Advanced UI placeholder");
    });

    // Static CSS
    server_.on("/static/styles.css", HTTP_GET, [](AsyncWebServerRequest* req){
        AsyncWebServerResponse* res = req->beginResponse(200, "text/css; charset=utf-8", STYLES_CSS);
        res->addHeader("Cache-Control", "max-age=31536000, immutable");
        req->send(res);
    });

    // GET /api/state (canonical + meta)
    server_.on("/api/state", HTTP_GET, [this](AsyncWebServerRequest* req){
        DBG_PRINTLN(Web, "HTTP GET /api/state");
        Cache snap; { xSemaphoreTake(cache_mutex_, portMAX_DELAY); snap = cache_; xSemaphoreGive(cache_mutex_); }
        const std::string name = controller.get_name();
        const bool online = (millis() - last_heartbeat_ms_) < 5000;
        std::string js = make_full_state_json(snap.rgb, snap.brightness_255, snap.power, snap.mode_id, &name, &online);
        AsyncWebServerResponse* res = req->beginResponse(200, "application/json; charset=utf-8", js.c_str());
        res->addHeader("Cache-Control", "no-store");
        req->send(res);
    });

    // SSE: initial state + immediate heartbeat for quick online recovery
    events_.onConnect([this](AsyncEventSourceClient* client){
        (void)client;
        DBG_PRINTF(Web, "SSE onConnect client=%p lastId=%u\n", (void*)client, client ? client->lastId() : 0);
        last_heartbeat_ms_ = millis();              // new session starts online
        broadcast_state_with_meta();                // includes "online": true
        events_.send("{\"online\":true}", "hb");    // kick UI heartbeat right away
    });
    server_.addHandler(&events_);
    DBG_PRINTLN(Web, "SSE /events handler added");

    // POST /api/state (patch)
    server_.on(
        "/api/state",
        HTTP_POST,
        [this](AsyncWebServerRequest* req){
            DBG_PRINTF(Web, "HTTP POST %s contentLength=%u (awaiting body)\n",
                       req->url().c_str(), (unsigned)req->contentLength());
            if (req->contentLength() == 0) {
                DBG_PRINTLN(Web, "POST /api/state: empty body -> 400");
                req->send(400, "application/json", "{\"ok\":false,\"err\":\"empty body\"}");
            }
        },
        nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total){
            if (req->url() != "/api/state") return;

            // Accumulate body
            std::string* buf = static_cast<std::string*>(req->_tempObject);
            if (index == 0) {
                buf = new std::string();
                buf->reserve(total);
                req->_tempObject = buf;
            }
            buf->append(reinterpret_cast<char*>(data), len);
            if (index + len < total) return;

            // Complete body
            std::string body = std::move(*buf);
            delete buf;
            req->_tempObject = nullptr;

            DBG_PRINTF(Web, "POST /api/state body len=%u: %s\n", (unsigned)body.size(), body.c_str());

            bool changed = false;

            // rgb
            std::array<uint8_t,3> rgb;
            if (extract_rgb_array(body, "rgb", rgb)) {
                { xSemaphoreTake(cache_mutex_, portMAX_DELAY); cache_.rgb = rgb; xSemaphoreGive(cache_mutex_); }
                DBG_PRINTF(Web, "PATCH rgb={%u,%u,%u}\n", rgb[0], rgb[1], rgb[2]);
                push_patch(make_color_patch_json(rgb));
                controller.sync_color(rgb, {true,true,false,true,true});
                changed = true;
            }

            // brightness (0..255)
            int bval;
            if (extract_number(body, "brightness", bval)) {
                uint8_t b = clamp_u8(bval);
                { xSemaphoreTake(cache_mutex_, portMAX_DELAY); cache_.brightness_255 = b; xSemaphoreGive(cache_mutex_); }
                DBG_PRINTF(Web, "PATCH brightness=%u (0..255)\n", b);
                push_patch(make_brightness_patch_json(b));
                controller.sync_brightness(b, {true,true,false,true,true});
                changed = true;
            }

            // power
            bool pwr;
            if (extract_bool(body, "power", pwr)) {
                { xSemaphoreTake(cache_mutex_, portMAX_DELAY); cache_.power = pwr; xSemaphoreGive(cache_mutex_); }
                DBG_PRINTF(Web, "PATCH power=%s\n", pwr ? "true" : "false");
                push_patch(make_power_patch_json(pwr));
                controller.sync_state(pwr ? 1 : 0, {true,true,false,true,true});
                changed = true;
            }

            // mode (string -> id (only "solid" for now))
            std::string mode_str;
            if (extract_string(body, "mode", mode_str)) {
                uint8_t id = 0;
                { xSemaphoreTake(cache_mutex_, portMAX_DELAY); cache_.mode_id = id; xSemaphoreGive(cache_mutex_); }
                DBG_PRINTF(Web, "PATCH mode='%s' -> id=%u\n", mode_str.c_str(), id);
                push_patch(make_mode_patch_json(id));
                controller.sync_mode(id, {true,true,false,true,true});
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
    server_.on("/api/state", HTTP_OPTIONS, [this](AsyncWebServerRequest* req){
        send_options(req);
    });

    server_.onNotFound([](AsyncWebServerRequest* req){
        req->send(404, "application/json", "{\"ok\":false,\"err\":\"not found\"}");
    });

    server_.begin();
    DBG_PRINTLN(Web, "Async Web server started on port 80 (ESP32).");

    // Heartbeat ticker
    arm_heartbeat();
    last_heartbeat_ms_ = millis();
}

void Web::loop() {
    // Send heartbeat event outside ISR when due
    if (heartbeat_due_) {
        heartbeat_due_ = false;
        events_.send("{\"online\":true}", "hb");
        last_heartbeat_ms_ = millis();
    }
}

void Web::reset(bool verbose) {
    (void)verbose;
    DBG_PRINTLN(Web, "Web::reset() -> clearing cache and broadcasting full state");
    xSemaphoreTake(cache_mutex_, portMAX_DELAY);
    cache_ = Cache{};
    xSemaphoreGive(cache_mutex_);
    broadcast_state_with_meta();
}

// --------------- Interface sync feeds our UI cache (and PUSH) ---------------
void Web::sync_color(std::array<uint8_t,3> color) {
    DBG_PRINTF(Web, "sync_color(rgb={%u,%u,%u}) -> cache+push\n", color[0], color[1], color[2]);
    xSemaphoreTake(cache_mutex_, portMAX_DELAY); cache_.rgb = color; xSemaphoreGive(cache_mutex_);
    push_patch(make_color_patch_json(color));
}
void Web::sync_brightness(uint8_t brightness) {
    DBG_PRINTF(Web, "sync_brightness(%u) -> cache+push\n", brightness);
    xSemaphoreTake(cache_mutex_, portMAX_DELAY); cache_.brightness_255 = brightness; xSemaphoreGive(cache_mutex_);
    push_patch(make_brightness_patch_json(brightness));
}
void Web::sync_state(uint8_t state) {
    bool p = (state != 0);
    DBG_PRINTF(Web, "sync_state(%u -> power=%s) -> cache+push\n", state, p ? "true" : "false");
    xSemaphoreTake(cache_mutex_, portMAX_DELAY); cache_.power = p; xSemaphoreGive(cache_mutex_);
    push_patch(make_power_patch_json(p));
}
void Web::sync_mode(uint8_t mode) {
    DBG_PRINTF(Web, "sync_mode(%u) -> cache+push\n", mode);
    xSemaphoreTake(cache_mutex_, portMAX_DELAY); cache_.mode_id = mode; xSemaphoreGive(cache_mutex_);
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
    xSemaphoreTake(cache_mutex_, portMAX_DELAY);
    cache_.rgb = color;
    cache_.brightness_255 = brightness;
    cache_.power = (state != 0);
    cache_.mode_id = mode;
    Cache snap = cache_;
    xSemaphoreGive(cache_mutex_);

    const std::string name = controller.get_name();
    const bool online = (millis() - last_heartbeat_ms_) < 5000;
    std::string js = make_full_state_json(snap.rgb, snap.brightness_255, snap.power, snap.mode_id, &name, &online);
    events_.send(js.c_str(), "state");
}

// ------------------------- private helpers -------------------------
std::string Web::render_index() const {
    Cache snap; { xSemaphoreTake(cache_mutex_, portMAX_DELAY); snap = cache_; xSemaphoreGive(cache_mutex_); }

    std::string html = INDEX_HTML;
    // Device name from controller
    std::string device_name = controller.get_name();
    if (device_name.empty()) device_name = "LED Strip";
    replace_all(html, "{{ name }}", device_name);
    // Initial slider placeholders (client will immediately resync via SSE)
    replace_all(html, "{{ state.hue }}", "0");
    replace_all(html, "{{ state.brightness }}", "0");
    replace_all(html, "{{ state.mode }}", "solid");
    return html;
}

void Web::send_ok(AsyncWebServerRequest* req) {
    AsyncWebServerResponse* res = req->beginResponse(200, "application/json", "{\"ok\":true}");
    res->addHeader("Access-Control-Allow-Origin", "*");
    res->addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    res->addHeader("Access-Control-Allow-Headers", "Content-Type");
    req->send(res);
}

void Web::send_options(AsyncWebServerRequest* req) {
    AsyncWebServerResponse* res = req->beginResponse(204);
    res->addHeader("Access-Control-Allow-Origin", "*");
    res->addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    res->addHeader("Access-Control-Allow-Headers", "Content-Type");
    req->send(res);
}

void Web::push_patch(const std::string& json) {
    DBG_PRINTF(Web, "push_patch(len=%u): %s\n", (unsigned)json.size(), json.c_str());
    events_.send(json.c_str(), "patch");
}

void Web::broadcast_state_with_meta() {
    Cache snap; { xSemaphoreTake(cache_mutex_, portMAX_DELAY); snap = cache_; xSemaphoreGive(cache_mutex_); }
    const std::string name = controller.get_name();
    const bool online = (millis() - last_heartbeat_ms_) < 5000;
    std::string js = make_full_state_json(snap.rgb, snap.brightness_255, snap.power, snap.mode_id, &name, &online);
    events_.send(js.c_str(), "state");
}

void Web::arm_heartbeat() {
    // Ticker ISR must be lightweight; set a flag and let loop() send SSE.
    hb_ticker_.attach_ms(2000, [this](){
        this->heartbeat_due_ = true;
    });
}
