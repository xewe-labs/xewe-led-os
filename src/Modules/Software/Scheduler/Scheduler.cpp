// src/Modules/Software/Scheduler/Scheduler.cpp
#include "Scheduler.h"
#include "../../../SystemController/SystemController.h"

#include <WiFi.h>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

#include "esp_netif_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// -----------------------------------------------------------------------------
// Anonymous Namespace: Encapsulated Helpers & Types
// -----------------------------------------------------------------------------
namespace {
    struct CurrentTimeInfo {
        uint8_t day;
        uint16_t minute_of_day;
        int32_t daystamp;
        int year, month, day_of_month, hour, minute, second;
    };

    bool is_system_time_valid() {
        const time_t now = time(nullptr);
        tm tm_now{};
        return localtime_r(&now, &tm_now) && (tm_now.tm_year + 1900) >= 2024;
    }

    bool get_current_time(CurrentTimeInfo& out) {
        if (!is_system_time_valid()) return false;
        const time_t now = time(nullptr);
        tm tm_now{};
        localtime_r(&now, &tm_now);

        out.day = (tm_now.tm_wday + 6) % 7; // 0=Monday
        out.minute_of_day = (tm_now.tm_hour * 60) + tm_now.tm_min;
        out.daystamp = ((tm_now.tm_year + 1900) * 1000) + tm_now.tm_yday;
        out.year = tm_now.tm_year + 1900;
        out.month = tm_now.tm_mon + 1;
        out.day_of_month = tm_now.tm_mday;
        out.hour = tm_now.tm_hour;
        out.minute = tm_now.tm_min;
        out.second = tm_now.tm_sec;
        return true;
    }

    std::string format_datetime(const CurrentTimeInfo& t) {
        const char* days[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
        char buf[64];
        snprintf(buf, sizeof(buf), "%s %04d-%02d-%02d %02d:%02d:%02d",
                 days[t.day % 7], t.year, t.month, t.day_of_month, t.hour, t.minute, t.second);
        return std::string(buf);
    }

    std::string format_gmt_bias(int32_t bias_minutes) {
        char buf[16];
        snprintf(buf, sizeof(buf), "GMT%c%02d:%02d",
                 bias_minutes >= 0 ? '+' : '-', abs(bias_minutes) / 60, abs(bias_minutes) % 60);
        return std::string(buf);
    }

    std::vector<std::string> extract_commands(const std::string& blob) {
        std::vector<std::string> cmds;
        size_t start = 0;
        while ((start = blob.find('"', start)) != std::string::npos) {
            size_t end = blob.find('"', start + 1);
            while (end != std::string::npos && blob[end - 1] == '\\') {
                end = blob.find('"', end + 1);
            }
            if (end == std::string::npos) break;

            std::string cmd = blob.substr(start + 1, end - start - 1);
            size_t pos = 0;
            while ((pos = cmd.find("\\\"", pos)) != std::string::npos) {
                cmd.replace(pos, 2, "\"");
                pos++;
            }
            cmds.push_back(cmd);
            start = end + 1;
        }
        return cmds;
    }

    bool parse_tz_offset(const std::string& s, int32_t& bias_minutes) {
        std::string tz = s;
        for (char& c : tz) c = toupper(c);
        if (tz == "GMT" || tz == "GMT0") { bias_minutes = 0; return true; }

        if (tz.find("GMT") != 0 || tz.length() < 5) return false;
        char sign = tz[3];
        int h = 0, m = 0;

        if (sscanf(tz.c_str() + 4, "%d:%d", &h, &m) < 1) return false;
        bias_minutes = (h * 60 + m) * (sign == '-' ? -1 : 1);
        return bias_minutes >= -840 && bias_minutes <= 840;
    }

    std::string escape_json(const std::string& s) {
        std::string res;
        for (char c : s) {
            if (c == '"') res += "\\\"";
            else if (c == '\\') res += "\\\\";
            else res += c;
        }
        return res;
    }
}

// -----------------------------------------------------------------------------
// Scheduler Class Implementation
// -----------------------------------------------------------------------------
Scheduler::Scheduler(SystemController& controller)
    : Module(controller, "Scheduler", "Bind CLI cmds to scheduled weekday/time slots", "sched", true, true, true)
{
    commands_storage.push_back({"add", "Add schedule: id color day start_min end_min cmds", "$scheduler add evt-123 #33FF33 0 540 600 \"\\\"$led set_rgb 0 255 120\\\"\"", 6, [this](string args){ cli_add(args); }});
    commands_storage.push_back({"remove", "Remove a schedule block by ID", "$scheduler remove evt-123", 1, [this](string args){ cli_remove(args); }});
    commands_storage.push_back({"timezone", "Set timezone offset (e.g. GMT-08:00)", "$scheduler timezone GMT-08:00", 1, [this](string args){ cli_timezone(args); }});
}

void Scheduler::begin_routines_init(const ModuleConfig& cfg) {
    if (!is_enabled()) return;

    // 1. Benchmarking NTP Servers
    controller.serial_port.print("\n=== SCHEDULER INIT: FINDING FASTEST NTP SERVER ===");
    std::vector<std::string> ntp_servers = {"pool.ntp.org", "time.google.com", "time.cloudflare.com", "time.windows.com"};
    std::string best_server = "pool.ntp.org";
    uint32_t best_time = 0xFFFFFFFF;

    for (const auto& srv : ntp_servers) {
        esp_sntp_config_t sntp_cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG(srv.c_str());
        sntp_cfg.start = false;
        sntp_cfg.wait_for_sync = true;

        esp_netif_sntp_init(&sntp_cfg);
        esp_netif_sntp_start();

        uint32_t start_tick = xTaskGetTickCount();
        if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(4000)) == ESP_OK) {
            uint32_t elapsed = (xTaskGetTickCount() - start_tick) * portTICK_PERIOD_MS;
            controller.serial_port.print("  " + srv + " responded in " + std::to_string(elapsed) + "ms");

            if (elapsed < best_time) {
                best_time = elapsed;
                best_server = srv;
            }
        } else {
            controller.serial_port.print("  " + srv + " timed out.");
        }
        esp_netif_sntp_deinit();
    }

    controller.serial_port.print("-> Selected " + best_server + " as primary time server.");
    controller.nvs.write_str(nvs_key, "ntp_server", best_server);

    // 2. Interactive Timezone Prompt
    controller.serial_port.print("\n=== SCHEDULER INIT: SET TIMEZONE ===");
    bool tz_valid = false;
    int32_t parsed_bias = 0;
    std::string tz_input;

    while (!tz_valid) {
        tz_input = controller.serial_port.get_string("Enter your timezone offset (e.g. GMT-08:00): ", 4, 15, 0, 0, "GMT+00:00");
        if (parse_tz_offset(tz_input, parsed_bias)) {
            tz_valid = true;
            controller.nvs.write_str(nvs_key, "sched_tz", tz_input);
            controller.nvs.write_str(nvs_key, "sched_tz_min", std::to_string(parsed_bias));
            controller.serial_port.print("-> Timezone saved as " + tz_input + ".");
        } else {
            controller.serial_port.print("-> Error: Invalid format. Please use the exact format like GMT-05:00 or GMT+02:00.");
        }
    }
}

void Scheduler::begin_routines_regular(const ModuleConfig& cfg) {
    if (!is_enabled()) return;

    // 1. Read and apply the saved timezone
    std::string tz_str = controller.nvs.read_str(nvs_key, "sched_tz_min");
    if (!tz_str.empty()) {
        active_timezone_bias_min = std::stoi(tz_str);
        apply_timezone(active_timezone_bias_min);
        timezone_ready = true;
    }

    // 2. Load the best NTP server and aggressively sync time until success
    std::string ntp_server = controller.nvs.read_str(nvs_key, "ntp_server", "pool.ntp.org");
    controller.serial_port.print("Scheduler: Syncing time via " + ntp_server + "...");

    esp_sntp_config_t sntp_cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG(ntp_server.c_str());
    sntp_cfg.start = false;
    sntp_cfg.wait_for_sync = true;

    esp_netif_sntp_init(&sntp_cfg);
    esp_netif_sntp_start();

    // Blocking loop until we successfully get the time
    while (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(5000)) != ESP_OK) {
        controller.serial_port.print("Scheduler: Waiting for SNTP sync to complete...");
    }

    time_ready = true;
    CurrentTimeInfo now;
    if (get_current_time(now)) {
        controller.serial_port.print("Scheduler: Time synced successfully -> " + format_datetime(now));
    }

    // 3. Load user schedules
    if (!loaded_from_nvs) load_from_nvs();
}

void Scheduler::loop() {
    if (is_disabled() || !time_ready || !timezone_ready) return;

    CurrentTimeInfo now;
    if (!get_current_time(now)) return;

    for (auto& sched : schedules) {
        if (sched.day != now.day || sched.start_minute != now.minute_of_day || sched.last_executed_daystamp == now.daystamp) {
            continue;
        }
        for (const auto& cmd : sched.commands) {
            if (!cmd.empty()) controller.command_parser.parse(cmd);
        }
        sched.last_executed_daystamp = now.daystamp;
    }
}

void Scheduler::reset(bool verbose, bool do_restart, bool keep_enabled) {
    nvs_clear_all();
    schedules.clear();
    loaded_from_nvs = timezone_ready = time_ready = false;
    active_timezone_bias_min = 0;
    Module::reset(verbose, do_restart, keep_enabled);
}

string Scheduler::status(bool verbose) const {
    if (is_disabled()) return "Scheduler module disabled";

    std::string s = "Timezone: " + (timezone_ready ? format_gmt_bias(active_timezone_bias_min) : "not configured") + "\n";
    s += "NTP Server: " + controller.nvs.read_str(nvs_key, "ntp_server", "pool.ntp.org") + "\n";

    CurrentTimeInfo now;
    s += "Current time: " + (get_current_time(now) ? format_datetime(now) : "unavailable (waiting on sync)") + "\n";

    if (schedules.empty()) {
        s += "No schedules active.";
    } else {
        s += "--- Active Schedules ---\n";
        for (const auto& sched : schedules) {
            s += "  [ID: " + sched.id + "] Day " + std::to_string(sched.day) +
                 " | " + std::to_string(sched.start_minute) + " to " + std::to_string(sched.end_minute) +
                 " | Cmds: " + std::to_string(sched.commands.size()) + "\n";
        }
    }
    if (verbose) controller.serial_port.print(s);
    return s;
}

std::string Scheduler::get_all_json() const {
    std::ostringstream json;
    json << "{";
    for (size_t i = 0; i < schedules.size(); ++i) {
        const auto& sched = schedules[i];
        json << "\"" << sched.id << "\": {";
        json << "\"id\": \"" << sched.id << "\", ";

        json << "\"commands\": [";
        for (size_t c = 0; c < sched.commands.size(); ++c) {
            json << "\"" << escape_json(sched.commands[c]) << "\"";
            if (c < sched.commands.size() - 1) json << ", ";
        }
        json << "], ";

        json << "\"color\": \"" << sched.color << "\", ";

        json << "\"slots\": [";
        for (uint16_t m = sched.start_minute; m < sched.end_minute; m += 15) {
            char time_buf[16];
            snprintf(time_buf, sizeof(time_buf), "%d:%02d", m / 60, m % 60);
            json << "{\"day\": " << (int)sched.day << ", \"time\": \"" << time_buf << "\"}";
            if (m + 15 < sched.end_minute) json << ", ";
        }
        json << "]";

        json << "}";
        if (i < schedules.size() - 1) json << ", ";
    }
    json << "}";
    return json.str();
}

// -----------------------------------------------------------------------------
// Core / Time Sync
// -----------------------------------------------------------------------------
void Scheduler::apply_timezone(int32_t bias_minutes) {
    char tz_env[32];
    snprintf(tz_env, sizeof(tz_env), "GMT%c%d:%02d",
             bias_minutes >= 0 ? '-' : '+', abs(bias_minutes) / 60, abs(bias_minutes) % 60);
    setenv("TZ", tz_env, 1);
    tzset();
    active_timezone_bias_min = bias_minutes;
}

bool Scheduler::add_schedule(const std::string& config) {
    std::istringstream iss(config);
    std::string id, color;
    uint32_t day, start, end;

    // Parse the new CLI argument layout: id color day start end
    if (!(iss >> id >> color >> day >> start >> end) || day > 6 || start > 1439 || end > 1440 || start >= end) {
        return false;
    }

    std::string blob;
    std::getline(iss, blob);
    auto cmds = extract_commands(blob);
    if (cmds.empty()) return false;

    // Check for duplicate IDs
    for (const auto& s : schedules) {
        if (s.id == id) return false;
    }

    schedules.push_back({id, (uint8_t)day, (uint16_t)start, (uint16_t)end, color, cmds, config, -1});

    // Sort schedules chronologically by start time just to keep things clean
    std::sort(schedules.begin(), schedules.end(), [](const ScheduleBlock& a, const ScheduleBlock& b) {
        return (a.day == b.day) ? (a.start_minute < b.start_minute) : (a.day < b.day);
    });
    return true;
}

void Scheduler::remove_schedule(const std::string& id) {
    schedules.erase(std::remove_if(schedules.begin(), schedules.end(),
                    [&id](const ScheduleBlock& s) { return s.id == id; }), schedules.end());
}

// -----------------------------------------------------------------------------
// NVS Management
// -----------------------------------------------------------------------------
void Scheduler::load_from_nvs() {
    schedules.clear();
    int count = controller.nvs.read_uint8(nvs_key, "sched_count", 0);
    for (int i = 0; i < count; ++i) {
        std::string cfg = controller.nvs.read_str(nvs_key, "sched_cfg_" + std::to_string(i));
        if (!cfg.empty()) add_schedule(cfg);
    }
    loaded_from_nvs = true;
}

void Scheduler::nvs_append_config(const std::string& cfg) {
    int count = controller.nvs.read_uint8(nvs_key, "sched_count", 0);
    controller.nvs.write_str(nvs_key, "sched_cfg_" + std::to_string(count), cfg);
    controller.nvs.write_uint8(nvs_key, "sched_count", count + 1);
}

void Scheduler::nvs_rewrite_all_configs() {
    controller.nvs.write_uint8(nvs_key, "sched_count", 0);
    for (size_t i = 0; i < schedules.size(); ++i) {
        controller.nvs.write_str(nvs_key, "sched_cfg_" + std::to_string(i), schedules[i].config_str);
    }
    controller.nvs.write_uint8(nvs_key, "sched_count", schedules.size());
}

void Scheduler::nvs_clear_all() {
    int count = controller.nvs.read_uint8(nvs_key, "sched_count", 0);
    for (int i = 0; i < count; ++i) {
        controller.nvs.remove(nvs_key, "sched_cfg_" + std::to_string(i));
    }
    controller.nvs.remove(nvs_key, "sched_tz");
    controller.nvs.remove(nvs_key, "sched_tz_min");
    controller.nvs.remove(nvs_key, "ntp_server");
    controller.nvs.write_uint8(nvs_key, "sched_count", 0);
}

// -----------------------------------------------------------------------------
// CLI Handlers
// -----------------------------------------------------------------------------
void Scheduler::cli_add(std::string_view args) {
    if (!is_enabled()) { controller.serial_port.print("Module disabled."); return; }

    std::string config(args);
    if (add_schedule(config)) {
        nvs_append_config(config);
        controller.serial_port.print("Added schedule block.");
    } else {
        controller.serial_port.print("Error: Invalid or duplicate schedule configuration.");
    }
}

void Scheduler::cli_remove(std::string_view args) {
    if (!is_enabled()) { controller.serial_port.print("Module disabled."); return; }

    std::string id(args);
    remove_schedule(id);
    nvs_rewrite_all_configs();
    controller.serial_port.print("Removed schedule ID: " + id);
}

void Scheduler::cli_timezone(std::string_view args) {
    if (!is_enabled()) { controller.serial_port.print("Module disabled."); return; }

    int32_t bias = 0;
    if (parse_tz_offset(std::string(args), bias)) {
        apply_timezone(bias);
        controller.nvs.write_str(nvs_key, "sched_tz", std::string(args));
        controller.nvs.write_str(nvs_key, "sched_tz_min", std::to_string(bias));
        timezone_ready = true;
        controller.serial_port.print("Timezone updated to " + format_gmt_bias(bias));
    } else {
        controller.serial_port.print("Error: Invalid timezone string. Try 'GMT-08:00'.");
    }
}