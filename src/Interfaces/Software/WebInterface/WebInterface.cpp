// src/Interfaces/Software/WebInterface/WebInterface.cpp

#include "WebInterface.h"
#include "../../../SystemController/SystemController.h"
#include <ArduinoJson.h>

// required
WebInterface::WebInterface(SystemController& controller)
      : Interface(controller,
               /* module_name         */ "Web_Interface",
               /* module_description  */ "Allows to control LED in web browser from\nany local device",
               /* nvs_key             */ "web",
               /* requires_init_setup */ true,
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
    (void)length;
    // received new value, propagate it in the module
}

void WebInterface::sync_param(std::string_view key, uint16_t value) {
    if (is_disabled()) return;
    char payload[64];
    size_t len = snprintf(payload, sizeof(payload), "P%.*s:%u", (int)key.length(), key.data(), value);
    broadcast(payload, len);
}

// optional
void WebInterface::sync_all(std::array<uint8_t,3> color,
                   uint8_t brightness,
                   uint8_t state,
                   uint8_t mode,
                   uint16_t length) {

    if (is_disabled()) return;
    (void)length;

    char payload[64];
    size_t len = snprintf(payload, sizeof(payload), "F%02X%02X%02X,%u,%u,%u",
        color[0], color[1], color[2],
        (unsigned)brightness,
        (unsigned)(state ? 1 : 0),
        (unsigned)mode
    );
    broadcast(payload, len);
}

void WebInterface::begin_routines_required(const ModuleConfig& cfg) {
    (void)cfg;

    // --- Main UI ---
    httpServer.on("/",      HTTP_GET, std::bind(&WebInterface::serveMainPage,         this));
    httpServer.on("/set",   HTTP_GET, std::bind(&WebInterface::handleSetRequest,      this));
    httpServer.on("/state", HTTP_GET, std::bind(&WebInterface::handleGetStateRequest, this));
    httpServer.on("/modes", HTTP_GET, std::bind(&WebInterface::handleGetModesRequest, this));
    httpServer.on("/name",  HTTP_GET, std::bind(&WebInterface::handleGetNameRequest,  this));

    // Standard Paths
    httpServer.on("/index.css", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS(); httpServer.send_P(200, "text/css", INDEX_CSS);
    });
    httpServer.on("/index.js", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS(); httpServer.send_P(200, "application/javascript", INDEX_JS);
    });

    // Static Paths (Often requested by HTML templates)
    httpServer.on("/static/index.css", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS(); httpServer.send_P(200, "text/css", INDEX_CSS);
    });
    httpServer.on("/static/index.js", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS(); httpServer.send_P(200, "application/javascript", INDEX_JS);
    });

    // Jinja Fallback Catches for Main Page
    httpServer.on("/%7B%7B%20url_for('static',%20filename='index.css')%20%7D%7D", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS(); httpServer.send_P(200, "text/css", INDEX_CSS);
    });
    httpServer.on("/%7B%7B%20url_for('static',%20filename='index.js')%20%7D%7D", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS(); httpServer.send_P(200, "application/javascript", INDEX_JS);
    });

    // --- Scheduler Application ---
    httpServer.on("/schedule", HTTP_GET, std::bind(&WebInterface::serveSchedulePage, this));
    httpServer.on("/templates/schedule.html", HTTP_GET, std::bind(&WebInterface::serveSchedulePage, this));

    httpServer.on("/schedule/json", HTTP_GET, std::bind(&WebInterface::handleScheduleJson, this));
    httpServer.on("/schedule/set", HTTP_POST, std::bind(&WebInterface::handleScheduleSet, this));
    httpServer.on("/schedule/delete", HTTP_POST, std::bind(&WebInterface::handleScheduleDelete, this));

    // Jinja Fallback Catch for Scheduler
    httpServer.on("/%7B%7B%20url_for('static',%20filename='style.css')%20%7D%7D", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS(); httpServer.send_P(200, "text/css", SCHEDULE_STYLE_CSS);
    });

    // Scheduler Static Files
    httpServer.on("/static/schedule-style.css", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS(); httpServer.send_P(200, "text/css", SCHEDULE_STYLE_CSS);
    });
    httpServer.on("/static/schedule-core.js", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS(); httpServer.send_P(200, "application/javascript", SCHEDULE_CORE_JS);
    });
    httpServer.on("/static/schedule-actions.js", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS(); httpServer.send_P(200, "application/javascript", SCHEDULE_ACTIONS_JS);
    });
    httpServer.on("/static/schedule-interactions.js", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS(); httpServer.send_P(200, "application/javascript", SCHEDULE_INTERACTIONS_JS);
    });
    httpServer.on("/static/schedule-ui.js", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS(); httpServer.send_P(200, "application/javascript", SCHEDULE_UI_JS);
    });
    // Fallback for missing utils JS
    httpServer.on("/static/schedule-utils.js", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS(); httpServer.send(200, "application/javascript", "");
    });

    // Preflight CORS and Global 404
    httpServer.onNotFound([this]() {
        applyCORS();
        if (httpServer.method() == HTTP_OPTIONS) {
            httpServer.send(204);
        } else {
            httpServer.send(404, "text/plain", "404: Not Found");
        }
    });
}

void WebInterface::begin_routines_regular(const ModuleConfig& cfg) {
    (void)cfg;

    controller.serial_port.print(
        "Web Interface now available for the devices\non the " +
        controller.wifi.get_ssid() +
        " WiFi network\nhttp://" +
        controller.wifi.get_local_ip()
    );
}

void WebInterface::begin_routines_common(const ModuleConfig& cfg) {
    (void)cfg;

    httpServer.begin();
    webSocket.begin();
    webSocket.onEvent(std::bind(&WebInterface::webSocketEvent, this,
                                std::placeholders::_1, std::placeholders::_2,
                                std::placeholders::_3, std::placeholders::_4));
}

void WebInterface::loop() {
    if (is_disabled()) return;

    httpServer.handleClient();
    webSocket.loop();

    if (connected_clients && (millis() - last_heartbeat_ms >= HEARTBEAT_INTERVAL_MS)) {
        broadcast("H", 1);
        last_heartbeat_ms = millis();
    }
}

void WebInterface::reset(const bool verbose, const bool do_restart, const bool keep_enabled) {
    webSocket.disconnect();
    Module::reset(verbose, do_restart, keep_enabled);
}

std::string WebInterface::status(const bool verbose) const {
    if (is_disabled()) return std::string("WebInterface module disabled");

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

// ---------------------------------------------------------
// Default Web Handlers
// ---------------------------------------------------------
void WebInterface::serveMainPage() {
    if (is_disabled()) return;
    applyCORS();
    httpServer.send_P(200, "text/html", INDEX_HTML);
}

void WebInterface::handleSetRequest() {
    if (is_disabled()) return;
    applyCORS();

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
    } else if (httpServer.hasArg("reset_params")) {
        controller.led_strip.reset_current_mode();
    } else if (httpServer.hasArg("param") && httpServer.hasArg("val")) {
        controller.led_strip.set_mode_param(httpServer.arg("param").c_str(), httpServer.arg("val").toInt());
    }

    httpServer.send(200, "text/plain", "OK");
}

void WebInterface::handleGetStateRequest() {
    if (is_disabled()) return;
    applyCORS();

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
    applyCORS();

    std::string json_payload = controller.led_strip.get_all_modes_json();
    httpServer.send(200, "application/json", json_payload.c_str());
}

void WebInterface::handleGetNameRequest() {
    if (is_disabled()) return;
    applyCORS();
    httpServer.send(200, "text/plain", controller.system.get_device_name().c_str());
}

// ---------------------------------------------------------
// Scheduler Web Handlers
// ---------------------------------------------------------

void WebInterface::applyCORS() {
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
    httpServer.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS, DELETE, PUT");
    httpServer.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void WebInterface::serveSchedulePage() {
    if (is_disabled()) return;
    applyCORS();
    httpServer.send_P(200, "text/html", SCHEDULE_HTML);
}

void WebInterface::handleScheduleJson() {
    if (is_disabled()) return;
    applyCORS();
    // Directly inject the scheduler's robust JSON generator into the HTTP response.
    httpServer.send(200, "application/json", controller.scheduler.get_all_json().c_str());
}

void WebInterface::handleScheduleSet() {
    if (is_disabled()) return;
    applyCORS();

    if (!httpServer.hasArg("plain")) {
        httpServer.send(400, "application/json", "{\"status\": \"error\", \"message\": \"No payload\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, httpServer.arg("plain"));
    if (err) {
        httpServer.send(400, "application/json", "{\"status\": \"error\", \"message\": \"Invalid JSON\"}");
        return;
    }

    String event_id  = doc["id"].as<String>();

    // UPDATED: The frontend now sends 'displayed_color' instead of 'color'
    String color     = doc["displayed_color"].as<String>();

    int day_int      = doc["day"].as<int>();
    int start_min    = doc["start_time"].as<int>();
    int end_min      = doc["end_time"].as<int>();

    // Translation: Day Integer -> CLI Day String
    const char* day_strs[] = {"MO", "TU", "WE", "TH", "FR", "SA", "SU"};
    String day_str = (day_int >= 0 && day_int <= 6) ? day_strs[day_int] : "MO";

    // Translation: Minutes Integer -> CLI Time String
    char start_hhmm[10], end_hhmm[10];
    snprintf(start_hhmm, sizeof(start_hhmm), "%02d:%02d", start_min / 60, start_min % 60);
    snprintf(end_hhmm, sizeof(end_hhmm), "%02d:%02d", end_min / 60, end_min % 60);

    // Translation: Extract and format CLI commands array
    String cmds_str = "\"";
    JsonArray cmds = doc["commands"].as<JsonArray>();
    for (size_t i = 0; i < cmds.size(); i++) {
        String c = cmds[i].as<String>();
        c.replace("\"", "\\\""); // Escape inner quotes
        cmds_str += c;
        if (i < cmds.size() - 1) cmds_str += " ";
    }
    cmds_str += "\"";

    // If updating, delete the old schedule first (the CLI handles this cleanly natively)
    if (!doc["id"].isNull() && event_id != "null" && event_id != "0" && event_id != "") {
        controller.command_parser.parse("$scheduler remove " + std::string(event_id.c_str()));
    }

    // UPDATED: Iterate over a JsonArray instead of a JsonObject to find max_id
    int max_id = 0;
    JsonDocument all_doc;
    deserializeJson(all_doc, controller.scheduler.get_all_json());

    JsonArray root = all_doc.as<JsonArray>();
    for (JsonObject item : root) {
        int current_id = item["id"].as<int>();
        if (current_id > max_id) max_id = current_id;
    }
    int new_id = max_id + 1;

    // Build the strict formatting the command parser requires and push it to the scheduler
    std::string add_cmd = "$scheduler add " + std::string(color.c_str()) + " " +
                          std::string(day_str.c_str()) + " " + std::string(start_hhmm) + " " +
                          std::string(end_hhmm) + " " + std::string(cmds_str.c_str());

    controller.command_parser.parse(add_cmd);

    // Return Success
    JsonDocument res;
    res["status"] = "success";
    res["id"] = String(new_id);
    String response;
    serializeJson(res, response);
    httpServer.send(200, "application/json", response);
}

void WebInterface::handleScheduleDelete() {
    if (is_disabled()) return;
    applyCORS();

    if (!httpServer.hasArg("plain")) {
        httpServer.send(400, "application/json", "{\"status\": \"error\", \"message\": \"No payload\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, httpServer.arg("plain"));
    if (err || doc["id"].isNull()) {
        httpServer.send(400, "application/json", "{\"status\": \"error\", \"message\": \"Invalid ID\"}");
        return;
    }

    String event_id = doc["id"].as<String>();

    // Leverage the command parser to execute the removal
    controller.command_parser.parse("$scheduler remove " + std::string(event_id.c_str()));

    httpServer.send(200, "application/json", "{\"status\": \"success\"}");
}

// ---------------------------------------------------------
// WebSocket Logic
// ---------------------------------------------------------
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

        default:
            break;
    }
}

void WebInterface::broadcast(const char* payload, size_t length) {
    if (is_disabled()) return;
    if (length > 0) webSocket.broadcastTXT(payload, length);
}