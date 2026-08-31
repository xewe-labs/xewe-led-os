// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Software/SmartHome/WebInterface/WebInterface.cpp

#include "WebInterface.h"
#include "../../../Module/ModuleController.h"


// required
WebInterface::WebInterface(ModuleController& controller)
    : SyncModule(controller,
          /* id                  */ "web_interface",
          /* name                */ "Web_Interface",
          /* description         */ "Allows to control LED in web browser from\nany local device",
          /* requires_init_setup */ true,
          /* can_be_disabled     */ true,
          /* has_cli_cmds        */ true
    )
{}

void WebInterface::sync_color(std::array<uint8_t, 3> color) {
    if (is_disabled()) return;
    char   payload[8];
    size_t len = snprintf(payload, sizeof(payload), "C%02X%02X%02X", color[0], color[1], color[2]);
    broadcast(payload, len);
}

void WebInterface::sync_brightness(uint8_t brightness) {
    if (is_disabled()) return;
    char   payload[6];
    size_t len = snprintf(payload, sizeof(payload), "B%u", (unsigned)brightness);
    broadcast(payload, len);
}

void WebInterface::sync_state(bool state) {
    if (is_disabled()) return;
    char   payload[4];
    size_t len = snprintf(payload, sizeof(payload), "S%u", (unsigned)(state ? 1 : 0));
    broadcast(payload, len);
}

void WebInterface::sync_mode(uint8_t mode) {
    if (is_disabled()) return;
    char   payload[6];
    size_t len = snprintf(payload, sizeof(payload), "M%u", (unsigned)mode);
    broadcast(payload, len);
}

void WebInterface::sync_length(uint16_t length) {
    if (is_disabled()) return;
    (void)length;
    // received new value, propagate it in the module
}

// optional
void WebInterface::sync_all(std::array<uint8_t, 3> color,
                            uint8_t brightness,
                            bool state,
                            uint8_t mode,
                            uint16_t length) {
    if (is_disabled()) return;
    (void)length;

    char   payload[64];
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
    httpServer.on("/", HTTP_GET, std::bind(&WebInterface::serveMainPage, this));
    httpServer.on("/set", HTTP_GET, std::bind(&WebInterface::handleSetRequest, this));
    httpServer.on("/state", HTTP_GET, std::bind(&WebInterface::handleGetStateRequest, this));
    httpServer.on("/modes", HTTP_GET, std::bind(&WebInterface::handleGetModesRequest, this));
    httpServer.on("/name", HTTP_GET, std::bind(&WebInterface::handleGetNameRequest, this));

    // Standard Paths
    httpServer.on("/index.css", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS();
        httpServer.send_P(200, "text/css", INDEX_CSS);
    });
    httpServer.on("/index.js", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS();
        httpServer.send_P(200, "application/javascript", INDEX_JS);
    });

    // Static Paths (Often requested by HTML templates)
    httpServer.on("/static/index.css", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS();
        httpServer.send_P(200, "text/css", INDEX_CSS);
    });
    httpServer.on("/static/index.js", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS();
        httpServer.send_P(200, "application/javascript", INDEX_JS);
    });

    // Jinja Fallback Catches for Main Page
    httpServer.on("/%7B%7B%20url_for('static',%20filename='index.css')%20%7D%7D", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS();
        httpServer.send_P(200, "text/css", INDEX_CSS);
    });
    httpServer.on("/%7B%7B%20url_for('static',%20filename='index.js')%20%7D%7D", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS();
        httpServer.send_P(200, "application/javascript", INDEX_JS);
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
        applyCORS();
        httpServer.send_P(200, "text/css", SCHEDULE_STYLE_CSS);
    });

    // Scheduler Static Files
    httpServer.on("/static/schedule-style.css", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS();
        httpServer.send_P(200, "text/css", SCHEDULE_STYLE_CSS);
    });
    httpServer.on("/static/schedule-core.js", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS();
        httpServer.send_P(200, "application/javascript", SCHEDULE_CORE_JS);
    });
    httpServer.on("/static/schedule-actions.js", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS();
        httpServer.send_P(200, "application/javascript", SCHEDULE_ACTIONS_JS);
    });
    httpServer.on("/static/schedule-interactions.js", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS();
        httpServer.send_P(200, "application/javascript", SCHEDULE_INTERACTIONS_JS);
    });
    httpServer.on("/static/schedule-ui.js", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS();
        httpServer.send_P(200, "application/javascript", SCHEDULE_UI_JS);
    });
    // Fallback for missing utils JS
    httpServer.on("/static/schedule-utils.js", HTTP_GET, [this]() {
        if (is_disabled()) return;
        applyCORS();
        httpServer.send(200, "application/javascript", "");
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
        std::placeholders::_3, std::placeholders::_4
    ));
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

void WebInterface::reset(const bool verbose,
                         const bool do_restart,
                         const bool keep_enabled) {
    webSocket.disconnect();
    Module::reset(verbose, do_restart, keep_enabled);
}

std::string WebInterface::status(const bool verbose) const {
    if (is_disabled()) return std::string("WebInterface module disabled");

    std::ostringstream out;

    unsigned long      uptime_s  = millis() / 1000UL;
    int                days      = static_cast<int>(uptime_s / 86400UL);
    int                hours     = static_cast<int>((uptime_s % 86400UL) / 3600UL);
    int                mins      = static_cast<int>((uptime_s % 3600UL) / 60UL);
    int                secs      = static_cast<int>(uptime_s % 60UL);

    uint32_t           freeHeap  = ESP.getFreeHeap();
    uint32_t           totalHeap = ESP.getHeapSize();
    uint32_t           usedHeap  = totalHeap - freeHeap;
    float              heapUsage = (totalHeap ? (usedHeap * 100.0f) / totalHeap : 0.0f);

    out << "--- Web Server Status ---\n";
    out << "  - Uptime:            "
        << days << " days, "
        << std::setw(2) << std::setfill('0') << hours << ':'
        << std::setw(2) << std::setfill('0') << mins << ':'
        << std::setw(2) << std::setfill('0') << secs << '\n';

    out << "  - Memory Usage:      "
        << std::fixed << std::setprecision(2) << heapUsage << "% ("
        << usedHeap << " / " << totalHeap << " bytes)\n";

    out << "  - WebSocket Clients: " << connected_clients << '\n';
    out << "-------------------------";

    if (verbose) controller.serial_port.print(out.str());
    return out.str();
}

WebServer& WebInterface::get_server() { return httpServer; }

void WebInterface::sync_param(std::string_view key,
                              uint16_t value) {
    if (is_disabled()) return;
    char   payload[64];
    size_t len = snprintf(payload, sizeof(payload), "P%.*s:%u", (int)key.length(), key.data(), value);
    broadcast(payload, len);
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
        long    colorValue = strtol(httpServer.arg("color").c_str(), nullptr, 16);
        uint8_t r          = (colorValue >> 16) & 0xFF;
        uint8_t g          = (colorValue >> 8) & 0xFF;
        uint8_t b          = colorValue & 0xFF;
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

// ---------------------------------------------------------
// Scheduler Web Handlers
// ---------------------------------------------------------

void WebInterface::handleGetNameRequest() {
    if (is_disabled()) return;
    applyCORS();
    httpServer.send(200, "text/plain", controller.system.get_device_name().c_str());
}

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
    // Root JSON array of schedule blocks, matching the frontend contract.
    httpServer.send(200, "application/json", controller.scheduler.get_all_json().c_str());
}

void WebInterface::handleScheduleSet() {
    if (is_disabled()) return;
    applyCORS();

    if (!httpServer.hasArg("plain")) {
        httpServer.send(400, "application/json", "{\"status\": \"error\", \"message\": \"No payload\"}");
        return;
    }

    JsonDocument         doc;
    DeserializationError err = deserializeJson(doc, httpServer.arg("plain"));
    if (err) {
        httpServer.send(400, "application/json", "{\"status\": \"error\", \"message\": \"Invalid JSON\"}");
        return;
    }

    // Frontend payload maps directly onto Scheduler::ScheduleBlock fields.
    std::string              color = doc["displayed_color"].as<std::string>();
    uint8_t                  day   = static_cast<uint8_t>(doc["day"].as<int>());
    uint16_t                 start = static_cast<uint16_t>(doc["start_time"].as<int>());
    uint16_t                 end   = static_cast<uint16_t>(doc["end_time"].as<int>());

    std::vector<std::string> commands;
    for (JsonVariant c : doc["commands"].as<JsonArray>()) {
        std::string cmd = c.as<std::string>();
        if (!cmd.empty()) commands.push_back(std::move(cmd));
    }

    // On update the frontend resends the block with its existing id: drop the old one first.
    if (!doc["id"].isNull()) {
        int old_id = doc["id"].as<int>();
        if (old_id > 0) controller.scheduler.remove(static_cast<uint8_t>(old_id));
    }

    const bool added = controller.scheduler.add(start, end, day, std::move(color), std::move(commands));

    if (!added) {
        httpServer.send(400, "application/json", "{\"status\": \"error\", \"message\": \"Rejected\"}");
        return;
    }
    httpServer.send(200, "application/json", "{\"status\": \"success\"}");
}

void WebInterface::handleScheduleDelete() {
    if (is_disabled()) return;
    applyCORS();

    if (!httpServer.hasArg("plain")) {
        httpServer.send(400, "application/json", "{\"status\": \"error\", \"message\": \"No payload\"}");
        return;
    }

    JsonDocument         doc;
    DeserializationError err = deserializeJson(doc, httpServer.arg("plain"));
    if (err || doc["id"].isNull()) {
        httpServer.send(400, "application/json", "{\"status\": \"error\", \"message\": \"Invalid ID\"}");
        return;
    }

    controller.scheduler.remove(static_cast<uint8_t>(doc["id"].as<int>()));
    httpServer.send(200, "application/json", "{\"status\": \"success\"}");
}

// ---------------------------------------------------------
// WebSocket Logic
// ---------------------------------------------------------
void WebInterface::webSocketEvent(uint8_t num,
                                  WStype_t type,
                                  uint8_t* payload,
                                  size_t /*length*/) {
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

void WebInterface::broadcast(const char* payload,
                             size_t length) {
    if (is_disabled()) return;
    if (length > 0) webSocket.broadcastTXT(payload, length);
}
