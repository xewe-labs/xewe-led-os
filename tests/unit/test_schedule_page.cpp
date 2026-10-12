// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/tests/unit/test_schedule_page.cpp
//
// Host test of src/SchedulePage: the route table, the CORS headers and the list / set / delete
// handlers of SchedulePageApi.h against a scheduler with the Scheduler module's id and enable
// rules, then SchedulePage.cpp itself against a recording fake web server (route registration,
// the headers, the send overload, code, type and bytes of every answer). Built by
// tests/unit/test_schedule_page.py with g++ -Wall -Wextra -Werror, ArduinoJson and the empty
// firmware header files on the include path.

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "SchedulePageApi.h"

namespace api = schedule_page_api;

static int checks = 0;
static int failures = 0;

#define CHECK(cond) \
    do { \
        ++checks; \
        if (!(cond)) { \
            ++failures; \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        } \
    } while (0)

// The Scheduler module's add/remove rules: add is refused while disabled, a new id is the
// largest id + 1 (0 when empty); remove works while disabled and answers whether a block went.
struct FakeScheduler {
    struct Block {
        uint8_t id;
        uint16_t start;
        uint16_t end;
        uint8_t day;
        std::string color;
        std::vector<std::string> commands;
    };
    bool enabled = true;
    std::vector<Block> blocks;
    std::vector<std::string> calls;
    std::string json = "[]";

    bool add(uint16_t start, uint16_t end, uint8_t day, std::string color,
        std::vector<std::string> commands) {
        calls.push_back("add");
        if (!enabled)
            return false;
        uint8_t id = 0;
        for (const Block& b : blocks)
            if (b.id >= id)
                id = static_cast<uint8_t>(b.id + 1);
        blocks.push_back({id, start, end, day, std::move(color), std::move(commands)});
        return true;
    }
    bool remove(uint8_t id) {
        calls.push_back("remove " + std::to_string(id));
        for (auto it = blocks.begin(); it != blocks.end(); ++it) {
            if (it->id == id) {
                blocks.erase(it);
                return true;
            }
        }
        return false;
    }
    std::string get_all_json() const {
        return json;
    }
};

static const char* SUCCESS = "{\"status\": \"success\"}";
static const char* NO_PAYLOAD = "{\"status\": \"error\", \"message\": \"No payload\"}";
static const char* INVALID_JSON = "{\"status\": \"error\", \"message\": \"Invalid JSON\"}";
static const char* REJECTED = "{\"status\": \"error\", \"message\": \"Rejected\"}";
static const char* INVALID_ID = "{\"status\": \"error\", \"message\": \"Invalid ID\"}";

static bool is(const api::Response& r, int code, const char* body) {
    return r.code == code && std::strcmp(r.type, "application/json") == 0 && r.body == body;
}

// (path, method, content type) in registration order
static const struct {
    const char* path;
    const char* method;
    const char* type;
} ROUTE_TABLE[] = {
    {"/schedule", "GET", "text/html"},
    {"/templates/schedule.html", "GET", "text/html"},
    {"/schedule/json", "GET", "application/json"},
    {"/schedule/set", "POST", "application/json"},
    {"/schedule/delete", "POST", "application/json"},
    {"/%7B%7B%20url_for('static',%20filename='style.css')%20%7D%7D", "GET", "text/css"},
    {"/static/schedule-style.css", "GET", "text/css"},
    {"/static/schedule-core.js", "GET", "application/javascript"},
    {"/static/schedule-actions.js", "GET", "application/javascript"},
    {"/static/schedule-interactions.js", "GET", "application/javascript"},
    {"/static/schedule-ui.js", "GET", "application/javascript"},
    {"/static/schedule-utils.js", "GET", "application/javascript"},
};

static const char* const CORS[3][2] = {
    {"Access-Control-Allow-Origin", "*"},
    {"Access-Control-Allow-Methods", "POST, GET, OPTIONS, DELETE, PUT"},
    {"Access-Control-Allow-Headers", "Content-Type"},
};

static void test_routes() {
    CHECK(api::ROUTE_COUNT == 12);
    for (std::size_t i = 0; i < api::ROUTE_COUNT && i < 12; ++i) {
        CHECK(std::strcmp(api::ROUTES[i].path, ROUTE_TABLE[i].path) == 0);
        CHECK(std::strcmp(api::ROUTES[i].method, ROUTE_TABLE[i].method) == 0);
        CHECK(std::strcmp(api::content_type(api::ROUTES[i].content), ROUTE_TABLE[i].type) == 0);
    }
    CHECK(std::strcmp(api::CORS_HEADERS[0][0], "Access-Control-Allow-Origin") == 0);
    CHECK(std::strcmp(api::CORS_HEADERS[0][1], "*") == 0);
    CHECK(std::strcmp(api::CORS_HEADERS[1][0], "Access-Control-Allow-Methods") == 0);
    CHECK(std::strcmp(api::CORS_HEADERS[1][1], "POST, GET, OPTIONS, DELETE, PUT") == 0);
    CHECK(std::strcmp(api::CORS_HEADERS[2][0], "Access-Control-Allow-Headers") == 0);
    CHECK(std::strcmp(api::CORS_HEADERS[2][1], "Content-Type") == 0);
    CHECK(std::strcmp(api::BODY_SUCCESS, SUCCESS) == 0);
    CHECK(std::strcmp(api::BODY_NO_PAYLOAD, NO_PAYLOAD) == 0);
    CHECK(std::strcmp(api::BODY_INVALID_JSON, INVALID_JSON) == 0);
    CHECK(std::strcmp(api::BODY_REJECTED, REJECTED) == 0);
    CHECK(std::strcmp(api::BODY_INVALID_ID, INVALID_ID) == 0);
}

static void test_list() {
    FakeScheduler s;
    s.json =
        "[{\"id\":0,\"start_time\":60,\"end_time\":75,\"day\":0,\"displayed_color\":\"33ff33\","
        "\"commands\":[\"$led set_hsv 85 255 255\"]}]";
    const api::Response r = api::list_schedules(s);
    CHECK(r.code == 200 && std::strcmp(r.type, "application/json") == 0 && r.body == s.json);
    s.json = "[]"; // a disabled scheduler loads nothing
    CHECK(api::list_schedules(s).body == "[]");
}

static void test_set_payload_errors() {
    FakeScheduler s;
    CHECK(is(api::set_schedule(false, "", s), 400, NO_PAYLOAD));
    CHECK(is(api::set_schedule(false, "{\"day\":1}", s), 400, NO_PAYLOAD));
    CHECK(is(api::set_schedule(true, "", s), 400, INVALID_JSON));
    CHECK(is(api::set_schedule(true, "{", s), 400, INVALID_JSON));
    CHECK(is(api::set_schedule(true, "not json", s), 400, INVALID_JSON));
    CHECK(s.calls.empty());
}

static void test_set_new_block() {
    FakeScheduler s;
    const api::Response r = api::set_schedule(true,
        "{\"commands\":[\"$led set_hsv 85 255 255\",\"\",\"$led set_brightness 10\"],"
        "\"displayed_color\":\"33ff33\",\"day\":2,\"start_time\":480,\"end_time\":495}",
        s);
    CHECK(is(r, 200, SUCCESS));
    CHECK(s.calls.size() == 1 && s.calls[0] == "add");
    CHECK(s.blocks.size() == 1);
    if (s.blocks.size() == 1) {
        const FakeScheduler::Block& b = s.blocks[0];
        CHECK(b.id == 0 && b.start == 480 && b.end == 495 && b.day == 2);
        CHECK(b.color == "33ff33");
        // empty commands are dropped, the rest keep their order
        CHECK(b.commands.size() == 2);
        CHECK(b.commands.size() == 2 && b.commands[0] == "$led set_hsv 85 255 255");
        CHECK(b.commands.size() == 2 && b.commands[1] == "$led set_brightness 10");
    }
    // no range checks; values are narrowed to the scheduler's types
    FakeScheduler t;
    CHECK(is(api::set_schedule(true,
                 "{\"commands\":[],\"displayed_color\":\"ff0000\",\"day\":300,"
                 "\"start_time\":70000,\"end_time\":2000}",
                 t),
        200, SUCCESS));
    CHECK(t.blocks.size() == 1 && t.blocks[0].day == 44 && t.blocks[0].start == 4464 &&
        t.blocks[0].end == 2000 && t.blocks[0].commands.empty());
    // missing fields: 0 for the numbers, no commands
    FakeScheduler u;
    CHECK(is(api::set_schedule(true, "{}", u), 200, SUCCESS));
    CHECK(u.blocks.size() == 1 && u.blocks[0].day == 0 && u.blocks[0].start == 0 &&
        u.blocks[0].end == 0 && u.blocks[0].commands.empty());
    // "id": null is the same as no id
    FakeScheduler v;
    CHECK(is(api::set_schedule(true, "{\"id\":null,\"day\":1}", v), 200, SUCCESS));
    CHECK(v.calls.size() == 1 && v.calls[0] == "add");
}

static void test_set_edit() {
    // an id > 0 removes that block first, then adds the edited one with a new id
    FakeScheduler s;
    s.add(0, 15, 0, "aaaaaa", {"$led set_state 1"});
    s.add(15, 30, 0, "bbbbbb", {"$led set_state 0"});
    s.calls.clear();
    CHECK(is(api::set_schedule(true,
                 "{\"id\":1,\"commands\":[\"x\"],\"displayed_color\":\"cccccc\",\"day\":0,"
                 "\"start_time\":15,\"end_time\":45}",
                 s),
        200, SUCCESS));
    CHECK(s.calls.size() == 2 && s.calls[0] == "remove 1" && s.calls[1] == "add");
    CHECK(s.blocks.size() == 2 && s.blocks[1].id == 1 && s.blocks[1].color == "cccccc");

    // id 0 removes nothing: editing block 0 adds a second block next to it
    FakeScheduler z;
    z.add(60, 75, 3, "33ff33", {"$led set_hsv 85 255 255"});
    z.calls.clear();
    CHECK(is(api::set_schedule(true,
                 "{\"id\":0,\"commands\":[\"$led set_hsv 0 255 255\"],"
                 "\"displayed_color\":\"ff0000\",\"day\":3,\"start_time\":60,\"end_time\":90}",
                 z),
        200, SUCCESS));
    CHECK(z.calls.size() == 1 && z.calls[0] == "add");
    CHECK(z.blocks.size() == 2);
    CHECK(z.blocks.size() == 2 && z.blocks[0].id == 0 && z.blocks[0].color == "33ff33");
    CHECK(z.blocks.size() == 2 && z.blocks[1].id == 1 && z.blocks[1].color == "ff0000");

    // negative ids remove nothing; ids above 255 are narrowed to 8 bits
    FakeScheduler n;
    CHECK(is(api::set_schedule(true, "{\"id\":-1}", n), 200, SUCCESS));
    CHECK(n.calls.size() == 1 && n.calls[0] == "add");
    FakeScheduler w;
    CHECK(is(api::set_schedule(true, "{\"id\":300}", w), 200, SUCCESS));
    CHECK(w.calls.size() == 2 && w.calls[0] == "remove 44");
    // an unknown id still adds
    FakeScheduler k;
    CHECK(is(api::set_schedule(true, "{\"id\":9,\"day\":1}", k), 200, SUCCESS));
    CHECK(k.calls.size() == 2 && k.calls[0] == "remove 9" && k.blocks.size() == 1);
}

static void test_set_rejected() {
    // The scheduler is disabled only at boot (its requirements are not met) and then loads no
    // blocks, so the edit's remove finds nothing and the add is refused: Rejected, and the block
    // stored in NVS is untouched.
    FakeScheduler s;
    s.enabled = false;
    CHECK(is(api::set_schedule(true, "{\"id\":1,\"day\":0}", s), 400, REJECTED));
    CHECK(s.calls.size() == 2 && s.calls[0] == "remove 1" && s.calls[1] == "add");
    CHECK(s.blocks.empty());
    s.calls.clear();
    CHECK(is(api::set_schedule(true, "{}", s), 400, REJECTED));
    CHECK(s.calls.size() == 1 && s.calls[0] == "add");
    // Model only (the scheduler cannot be disabled while it holds blocks): the remove comes
    // before the add, whatever the add answers.
    FakeScheduler m;
    m.add(0, 15, 0, "aaaaaa", {});
    m.add(15, 30, 0, "bbbbbb", {});
    m.enabled = false;
    m.calls.clear();
    CHECK(is(api::set_schedule(true, "{\"id\":1,\"day\":0}", m), 400, REJECTED));
    CHECK(m.calls.size() == 2 && m.calls[0] == "remove 1" && m.calls[1] == "add");
}

static void test_delete() {
    FakeScheduler s;
    s.add(0, 15, 0, "aaaaaa", {});
    s.add(15, 30, 0, "bbbbbb", {});
    s.calls.clear();
    CHECK(is(api::delete_schedule(false, "", s), 400, NO_PAYLOAD));
    CHECK(is(api::delete_schedule(true, "", s), 400, INVALID_ID));
    CHECK(is(api::delete_schedule(true, "{", s), 400, INVALID_ID));
    CHECK(is(api::delete_schedule(true, "{}", s), 400, INVALID_ID));
    CHECK(is(api::delete_schedule(true, "{\"id\":null}", s), 400, INVALID_ID));
    CHECK(s.calls.empty());
    CHECK(is(api::delete_schedule(true, "{\"id\":1}", s), 200, SUCCESS));
    CHECK(s.calls.size() == 1 && s.calls[0] == "remove 1" && s.blocks.size() == 1);
    // the scheduler's answer is not checked: an unknown id is a success
    CHECK(is(api::delete_schedule(true, "{\"id\":42}", s), 200, SUCCESS));
    CHECK(s.calls.size() == 2 && s.calls[1] == "remove 42" && s.blocks.size() == 1);
    // id 0 is deleted normally (only the edit skips it); 256 narrows to 0
    CHECK(is(api::delete_schedule(true, "{\"id\":256}", s), 200, SUCCESS));
    CHECK(s.calls.size() == 3 && s.calls[2] == "remove 0" && s.blocks.empty());
    // works while the scheduler is disabled
    s.enabled = false;
    CHECK(is(api::delete_schedule(true, "{\"id\":0}", s), 200, SUCCESS));
}

// ---------------------------------------------------------------------------------------------
// The glue: src/SchedulePage/SchedulePage.cpp itself, compiled on the host. The declarations
// below stand in for the firmware headers SchedulePage.h and the pages include (XeWeCore.h,
// WebServer.h, pgmspace.h, Scheduler/Scheduler.h, WebInterface/WebInterface.h); the test puts
// empty files of those names on the include path. Signatures follow XeWeCore 3.0.0, the esp32
// core 3.3.12 WebServer (the same send() overload set, so the glue picks the same overloads) and
// the Scheduler and Web Interface modules. The fake server records every call the glue makes.
// ---------------------------------------------------------------------------------------------

#define PROGMEM
typedef const char* PGM_P;

class String {
public:
    String(const char* s = "") : text(s == nullptr ? "" : s) {}
    explicit String(std::string s) : text(std::move(s)) {}
    const char* c_str() const {
        return text.c_str();
    }
    unsigned int length() const {
        return static_cast<unsigned int>(text.size());
    }
    const std::string& str() const {
        return text;
    }

private:
    std::string text;
};

enum http_method {
    HTTP_DELETE = 0,
    HTTP_GET = 1,
    HTTP_HEAD = 2,
    HTTP_POST = 3,
    HTTP_PUT = 4,
    HTTP_CONNECT = 5,
    HTTP_OPTIONS = 6,
};
typedef enum http_method HTTPMethod;

class Uri {
public:
    Uri(const char* uri) : path(uri) {}
    std::string path;
};

class RequestHandler {};

class WebServer {
public:
    typedef std::function<void(void)> THandlerFunction;

    struct Route {
        std::string path;
        HTTPMethod method;
        THandlerFunction fn;
    };
    // one sendHeader or send call: kind is "header" or the send overload that was called
    struct Call {
        std::string kind;
        int code;
        std::string a; // header name / content type ("<null>" for a null type)
        std::string b; // header value / body
    };

    std::vector<Route> routes;
    std::vector<Call> calls;
    std::map<std::string, std::string> args; // the request's arguments ("plain": the body)
    int not_found_handlers = 0;
    RequestHandler handler;

    RequestHandler& on(const Uri& uri, HTTPMethod method, THandlerFunction fn) {
        routes.push_back({uri.path, method, std::move(fn)});
        return handler;
    }
    void onNotFound(THandlerFunction) {
        ++not_found_handlers;
    }
    String arg(const String& name) const {
        auto it = args.find(name.str());
        return it == args.end() ? String("") : String(it->second);
    }
    bool hasArg(const String& name) const {
        return args.count(name.str()) != 0;
    }
    void sendHeader(const String& name, const String& value, bool first = false) {
        calls.push_back({first ? "header first" : "header", 0, name.str(), value.str()});
    }
    void send(int code, const char* content_type = nullptr, const String& content = String("")) {
        record("send(const char*, const String&)", code, content_type, content.str());
    }
    void send(int code, char* content_type, const String& content) {
        record("send(char*, const String&)", code, content_type, content.str());
    }
    void send(int code, const String& content_type, const String& content) {
        record("send(const String&, const String&)", code, content_type.c_str(), content.str());
    }
    void send(int code, const char* content_type, const char* content) {
        record("send(const char*, const char*)", code, content_type, content);
    }
    void send_P(int code, PGM_P content_type, PGM_P content) {
        record("send_P", code, content_type, content);
    }

private:
    void record(const char* kind, int code, const char* type, const std::string& body) {
        calls.push_back({kind, code, type == nullptr ? "<null>" : type, body});
    }
};

namespace xewe {

class Module;

class Os {
public:
    Module* get_module(std::string_view id) const;
    void report_error(const char* fmt, ...) {
        errors.emplace_back(fmt);
    }
    std::vector<Module*> modules;
    std::vector<std::string> errors;
};

class Module {
public:
    Module(Os& os_ref, std::string id_, std::string name_, std::string description_,
        bool requires_init_setup_, bool can_be_disabled_, bool has_cli_commands_)
        : os(os_ref), id(std::move(id_)), name(std::move(name_)),
          description(std::move(description_)), requires_init_setup(requires_init_setup_),
          can_be_disabled(can_be_disabled_), has_cli_commands(has_cli_commands_) {
        os.modules.push_back(this);
    }
    virtual ~Module() noexcept = default;
    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;

    virtual void begin_routines_required() {}
    virtual void begin_routines_init() {}
    virtual void begin_routines_regular() {}
    virtual void begin_routines_common() {}

    void add_requirement(Module& other) {
        requirements.push_back(&other);
    }
    bool is_disabled(const bool verbose = false) const {
        (void)verbose;
        return disabled;
    }

    Os& os;
    std::string id;
    std::string name;
    std::string description;
    bool requires_init_setup;
    bool can_be_disabled;
    bool has_cli_commands;
    bool disabled = false; // set by the test
    std::vector<Module*> requirements;
};

inline Module* Os::get_module(std::string_view wanted) const {
    for (Module* m : modules)
        if (m->id == wanted)
            return m;
    return nullptr;
}

} // namespace xewe

struct Scheduler : FakeScheduler {};

class WebInterface {
public:
    WebServer& get_server() {
        return server;
    }
    WebServer server;
};

#include "SchedulePage.cpp"

// the served bytes: sizes of the page literals
static const struct {
    const char* path;
    const char* symbol_bytes;
    std::size_t size;
} SERVED[] = {
    {"/schedule", SCHEDULE_HTML, 883},
    {"/templates/schedule.html", SCHEDULE_HTML, 883},
    {"/%7B%7B%20url_for('static',%20filename='style.css')%20%7D%7D", SCHEDULE_STYLE_CSS, 5094},
    {"/static/schedule-style.css", SCHEDULE_STYLE_CSS, 5094},
    {"/static/schedule-core.js", SCHEDULE_CORE_JS, 4463},
    {"/static/schedule-actions.js", SCHEDULE_ACTIONS_JS, 8556},
    {"/static/schedule-interactions.js", SCHEDULE_INTERACTIONS_JS, 4474},
    {"/static/schedule-ui.js", SCHEDULE_UI_JS, 8876},
};

struct Rig {
    xewe::Os os;
    xewe::Module led_web{os, "led_web", "Led Web", "", false, true, true};
    WebInterface web_interface;
    Scheduler scheduler;
    SchedulePage page{os, web_interface, scheduler};
};

// calls the handler registered for `path` with `method`; returns what it sent
static std::vector<WebServer::Call> request(
    Rig& rig, const char* path, HTTPMethod method, const char* body = nullptr) {
    WebServer& server = rig.web_interface.server;
    server.calls.clear();
    server.args.clear();
    if (body != nullptr)
        server.args["plain"] = body;
    for (const WebServer::Route& r : server.routes)
        if (r.path == path && r.method == method) {
            r.fn();
            return server.calls;
        }
    std::printf("FAIL no route %s\n", path);
    ++failures;
    return {};
}

// the three CORS headers in order, then exactly one send of `kind` with code, type and body
static bool answered(const std::vector<WebServer::Call>& calls, const char* kind, int code,
    const char* type, const std::string& body) {
    if (calls.size() != 4)
        return false;
    for (std::size_t i = 0; i < 3; ++i) {
        if (calls[i].kind != "header" || calls[i].a != CORS[i][0] || calls[i].b != CORS[i][1])
            return false;
    }
    const WebServer::Call& c = calls[3];
    return c.kind == kind && c.code == code && c.a == type && c.b == body;
}

static void test_glue_module() {
    Rig rig;
    CHECK(rig.page.id == "schedule_page" && rig.page.name == "Schedule Page");
    CHECK(!rig.page.requires_init_setup && !rig.page.can_be_disabled && !rig.page.has_cli_commands);
    CHECK(rig.page.requirements.size() == 1 && rig.page.requirements[0] == &rig.led_web);
    CHECK(rig.os.errors.empty());
    // nothing is registered before the routine runs
    CHECK(rig.web_interface.server.routes.empty());

    // without led_web declared before it: one error at construction, no requirement
    xewe::Os os;
    WebInterface web_interface;
    Scheduler scheduler;
    SchedulePage page(os, web_interface, scheduler);
    CHECK(page.requirements.empty());
    CHECK(os.errors.size() == 1);
}

static void test_glue_routes() {
    Rig rig;
    const WebServer& server = rig.web_interface.server;
    rig.page.begin_routines_regular();
    CHECK(server.routes.size() == 12);
    for (std::size_t i = 0; i < server.routes.size() && i < 12; ++i) {
        CHECK(server.routes[i].path == ROUTE_TABLE[i].path);
        const HTTPMethod method =
            std::strcmp(ROUTE_TABLE[i].method, "POST") == 0 ? HTTP_POST : HTTP_GET;
        CHECK(server.routes[i].method == method);
    }
    // the catch-all belongs to Web Fallback; no answer is sent while registering
    CHECK(server.not_found_handlers == 0 && server.calls.empty());
    // a second run of the routine registers nothing more
    rig.page.begin_routines_regular();
    CHECK(server.routes.size() == 12);
}

static void test_glue_assets() {
    Rig rig;
    rig.page.begin_routines_regular();
    for (const auto& s : SERVED) {
        CHECK(std::strlen(s.symbol_bytes) == s.size);
        const auto calls = request(rig, s.path, HTTP_GET);
        std::string type;
        for (const auto& r : ROUTE_TABLE)
            if (std::strcmp(r.path, s.path) == 0)
                type = r.type;
        CHECK(answered(calls, "send_P", 200, type.c_str(), s.symbol_bytes));
    }
    CHECK(answered(request(rig, "/static/schedule-utils.js", HTTP_GET),
        "send(const char*, const char*)", 200, "application/javascript", ""));
}

static void test_glue_json() {
    Rig rig;
    rig.page.begin_routines_regular();
    const char* json_kind = "send(const char*, const char*)";
    rig.scheduler.json = "[{\"id\":0}]";
    CHECK(answered(request(rig, "/schedule/json", HTTP_GET), json_kind, 200, "application/json",
        "[{\"id\":0}]"));

    // set: the body arrives as the "plain" argument
    CHECK(answered(
        request(rig, "/schedule/set", HTTP_POST), json_kind, 400, "application/json", NO_PAYLOAD));
    CHECK(answered(request(rig, "/schedule/set", HTTP_POST, "{"), json_kind, 400,
        "application/json", INVALID_JSON));
    CHECK(answered(request(rig, "/schedule/set", HTTP_POST,
                       "{\"commands\":[\"$led set_state 1\"],\"displayed_color\":\"33ff33\","
                       "\"day\":2,\"start_time\":480,\"end_time\":495}"),
        json_kind, 200, "application/json", SUCCESS));
    CHECK(rig.scheduler.blocks.size() == 1 && rig.scheduler.blocks[0].start == 480 &&
        rig.scheduler.blocks[0].commands.size() == 1);
    rig.scheduler.enabled = false;
    CHECK(answered(request(rig, "/schedule/set", HTTP_POST, "{\"day\":1}"), json_kind, 400,
        "application/json", REJECTED));
    rig.scheduler.enabled = true;

    // delete
    CHECK(answered(request(rig, "/schedule/delete", HTTP_POST), json_kind, 400, "application/json",
        NO_PAYLOAD));
    CHECK(answered(request(rig, "/schedule/delete", HTTP_POST, "{}"), json_kind, 400,
        "application/json", INVALID_ID));
    CHECK(answered(request(rig, "/schedule/delete", HTTP_POST, "{\"id\":0}"), json_kind, 200,
        "application/json", SUCCESS));
    CHECK(rig.scheduler.blocks.empty());
}

static void test_glue_disabled() {
    // between a reset of the module and the restart that follows: no answer at all
    Rig rig;
    rig.page.begin_routines_regular();
    rig.page.disabled = true;
    for (const auto& r : ROUTE_TABLE) {
        const HTTPMethod method = std::strcmp(r.method, "POST") == 0 ? HTTP_POST : HTTP_GET;
        CHECK(request(rig, r.path, method, "{\"id\":1}").empty());
    }
}

int main() {
    test_routes();
    test_list();
    test_set_payload_errors();
    test_set_new_block();
    test_set_edit();
    test_set_rejected();
    test_delete();
    test_glue_module();
    test_glue_routes();
    test_glue_assets();
    test_glue_json();
    test_glue_disabled();
    std::printf(
        "%s: %d check(s), %d failure(s)\n", failures ? "FAILED" : "PASSED", checks, failures);
    return failures ? 1 : 0;
}
