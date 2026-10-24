// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/SchedulePage/SchedulePageApi.h
// Standard library and ArduinoJson only, so the host tests run the code the firmware runs.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <ArduinoJson.h>

namespace schedule_page_api {

// ---------------------------------------------------------------------------------------------
// Routes
// ---------------------------------------------------------------------------------------------

enum class Content : uint8_t {
    Page, // SCHEDULE_HTML, text/html
    Style, // SCHEDULE_STYLE_CSS, text/css
    CoreJs, // SCHEDULE_CORE_JS, application/javascript
    ActionsJs, // SCHEDULE_ACTIONS_JS, application/javascript
    InteractionsJs, // SCHEDULE_INTERACTIONS_JS, application/javascript
    UiJs, // SCHEDULE_UI_JS, application/javascript
    UtilsJs, // an empty body, application/javascript
    List, // list_schedules, application/json
    Set, // set_schedule, application/json
    Delete, // delete_schedule, application/json
};

struct Route {
    const char* path;
    const char* method; // "GET" or "POST"
    Content content;
};

// In registration order. Paths are matched byte for byte (the server does not URL-decode them):
// the %7B%7B ... %7D%7D path is the URL-encoded {{ url_for('static', filename='style.css') }}
// template link and serves the stylesheet. The page loads its scripts with relative links, so
// through /templates/schedule.html they resolve under /templates/static/, which is not routed.
inline constexpr Route ROUTES[] = {
    {"/schedule", "GET", Content::Page},
    {"/templates/schedule.html", "GET", Content::Page},
    {"/schedule/json", "GET", Content::List},
    {"/schedule/set", "POST", Content::Set},
    {"/schedule/delete", "POST", Content::Delete},
    {"/%7B%7B%20url_for('static',%20filename='style.css')%20%7D%7D", "GET", Content::Style},
    {"/static/schedule-style.css", "GET", Content::Style},
    {"/static/schedule-core.js", "GET", Content::CoreJs},
    {"/static/schedule-actions.js", "GET", Content::ActionsJs},
    {"/static/schedule-interactions.js", "GET", Content::InteractionsJs},
    {"/static/schedule-ui.js", "GET", Content::UiJs},
    {"/static/schedule-utils.js", "GET", Content::UtilsJs},
};
inline constexpr std::size_t ROUTE_COUNT = sizeof(ROUTES) / sizeof(ROUTES[0]);

// Sent before every answer of these routes. No caching headers are sent.
inline constexpr const char* CORS_HEADERS[3][2] = {
    {"Access-Control-Allow-Origin", "*"},
    {"Access-Control-Allow-Methods", "POST, GET, OPTIONS, DELETE, PUT"},
    {"Access-Control-Allow-Headers", "Content-Type"},
};

inline constexpr const char* TYPE_HTML = "text/html";
inline constexpr const char* TYPE_CSS = "text/css";
inline constexpr const char* TYPE_JS = "application/javascript";
inline constexpr const char* TYPE_JSON = "application/json";

inline const char* content_type(Content content) {
    switch (content) {
    case Content::Page:
        return TYPE_HTML;
    case Content::Style:
        return TYPE_CSS;
    case Content::CoreJs:
    case Content::ActionsJs:
    case Content::InteractionsJs:
    case Content::UiJs:
    case Content::UtilsJs:
        return TYPE_JS;
    case Content::List:
    case Content::Set:
    case Content::Delete:
        return TYPE_JSON;
    }
    return TYPE_HTML;
}

// ---------------------------------------------------------------------------------------------
// Handlers
// ---------------------------------------------------------------------------------------------

struct Response {
    int code;
    const char* type;
    std::string body;
};

// The answer bodies, byte for byte (a space after every colon).
inline constexpr const char* BODY_SUCCESS = "{\"status\": \"success\"}";
inline constexpr const char* BODY_NO_PAYLOAD =
    "{\"status\": \"error\", \"message\": \"No payload\"}";
inline constexpr const char* BODY_INVALID_JSON =
    "{\"status\": \"error\", \"message\": \"Invalid JSON\"}";
inline constexpr const char* BODY_REJECTED = "{\"status\": \"error\", \"message\": \"Rejected\"}";
inline constexpr const char* BODY_INVALID_ID =
    "{\"status\": \"error\", \"message\": \"Invalid ID\"}";

// GET /schedule/json: the scheduler's blocks as a JSON array, as it stores them.
template <class SchedulerT> Response list_schedules(const SchedulerT& scheduler) {
    return {200, TYPE_JSON, scheduler.get_all_json()};
}

// POST /schedule/set, body {id?, commands, displayed_color, day, start_time, end_time}.
// Values are taken as they come (no range checks; day, start and end are narrowed to the
// scheduler's integer types). Empty commands are dropped. An id greater than 0 removes that block
// first, so an edit replaces it; id 0 removes nothing, so editing block 0 adds a second block.
// The scheduler refuses the add only while it is disabled. It is disabled only from boot on, and
// then it has loaded no blocks, so the remove finds nothing and the stored blocks stay as they are.
template <class SchedulerT>
Response set_schedule(bool has_payload, const std::string& payload, SchedulerT& scheduler) {
    if (!has_payload)
        return {400, TYPE_JSON, BODY_NO_PAYLOAD};

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err)
        return {400, TYPE_JSON, BODY_INVALID_JSON};

    std::string color = doc["displayed_color"].as<std::string>();
    uint8_t day = static_cast<uint8_t>(doc["day"].as<int>());
    uint16_t start = static_cast<uint16_t>(doc["start_time"].as<int>());
    uint16_t end = static_cast<uint16_t>(doc["end_time"].as<int>());

    std::vector<std::string> commands;
    for (JsonVariant c : doc["commands"].as<JsonArray>()) {
        std::string cmd = c.as<std::string>();
        if (!cmd.empty())
            commands.push_back(std::move(cmd));
    }

    if (!doc["id"].isNull()) {
        int old_id = doc["id"].as<int>();
        if (old_id > 0)
            scheduler.remove(static_cast<uint8_t>(old_id));
    }

    const bool added = scheduler.add(start, end, day, std::move(color), std::move(commands));
    if (!added)
        return {400, TYPE_JSON, BODY_REJECTED};
    return {200, TYPE_JSON, BODY_SUCCESS};
}

// POST /schedule/delete, body {id}. The id is narrowed to 8 bits; the scheduler's answer is not
// checked, so an unknown id is a success too.
template <class SchedulerT>
Response delete_schedule(bool has_payload, const std::string& payload, SchedulerT& scheduler) {
    if (!has_payload)
        return {400, TYPE_JSON, BODY_NO_PAYLOAD};

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err || doc["id"].isNull())
        return {400, TYPE_JSON, BODY_INVALID_ID};

    scheduler.remove(static_cast<uint8_t>(doc["id"].as<int>()));
    return {200, TYPE_JSON, BODY_SUCCESS};
}

} // namespace schedule_page_api
