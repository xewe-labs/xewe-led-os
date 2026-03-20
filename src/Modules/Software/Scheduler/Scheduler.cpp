// src/Modules/Software/Scheduler/Scheduler.cpp
#include "Scheduler.h"
#include "../../../SystemController/SystemController.h"

#include <WiFi.h>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

#include "esp_sntp.h"
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

    bool parse_day(const std::string& day_str, uint8_t& day_num) {
        std::string d = day_str;
        for (char& c : d) c = toupper(c);
        if (d == "MO") { day_num = 0; return true; }
        if (d == "TU") { day_num = 1; return true; }
        if (d == "WE") { day_num = 2; return true; }
        if (d == "TH") { day_num = 3; return true; }
        if (d == "FR") { day_num = 4; return true; }
        if (d == "SA") { day_num = 5; return true; }
        if (d == "SU") { day_num = 6; return true; }
        return false;
    }

    std::string day_to_str(uint8_t day) {
        const char* days[] = {"MO", "TU", "WE", "TH", "FR", "SA", "SU"};
        return day < 7 ? days[day] : "??";
    }

    bool parse_time(const std::string& time_str, uint16_t& minutes) {
        int h = 0, m = 0;
        if (sscanf(time_str.c_str(), "%d:%d", &h, &m) == 2) {
            if (h >= 0 && h <= 23 && m >= 0 && m <= 59) {
                minutes = h * 60 + m;
                return true;
            }
        }
        return false;
    }

    std::vector<std::string> extract_commands(const std::string& blob) {
        std::vector<std::string> cmds;
        bool in_quotes = false;
        bool escaped = false;
        std::string current_cmd;

        // Pass 1: Extract anything enclosed in quotes
        for (size_t i = 0; i < blob.length(); ++i) {
            char c = blob[i];
            if (escaped) {
                current_cmd += c;
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                if (in_quotes) {
                    cmds.push_back(current_cmd);
                    current_cmd.clear();
                    in_quotes = false;
                } else {
                    in_quotes = true;
                }
            } else {
                if (in_quotes) {
                    current_cmd += c;
                }
            }
        }

        // Pass 2: If the user wrapped all commands in a single outer quote (e.g. "\"cmd1\" \"cmd2\""),
        // Pass 1 will extract ONE giant string: `"cmd1" "cmd2"`.
        // We detect this by checking if the extracted string contains multiple unescaped quotes.
        if (cmds.size() == 1) {
            std::string inner = cmds[0];
            int q_count = 0;
            for (size_t i = 0; i < inner.length(); ++i) {
                if (inner[i] == '"' && (i == 0 || inner[i-1] != '\\')) {
                    q_count++;
                }
            }
            if (q_count >= 2) {
                // It's a nested block. Recursively parse it to separate the inner commands!
                return extract_commands(inner);
            }
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

    std::vector<std::string> split_string(const std::string& str, char delim) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(str);
        while (std::getline(tokenStream, token, delim)) {
            if (!token.empty()) tokens.push_back(token);
        }
        return tokens;
    }
}

// -----------------------------------------------------------------------------
// Scheduler Class Implementation
// -----------------------------------------------------------------------------
Scheduler::Scheduler(SystemController& controller)
    : Module(controller, "Scheduler", "Bind CLI cmds to scheduled weekday/time slots", "sched", true, true, true)
{
    commands_storage.push_back({"add", "Add schedule: color day(MO-SU) start(HH:MM) end(HH:MM) cmds", "$scheduler add #33FF33 MO 09:00 10:00 \"\\\"$led turn_on\\\"\"", 5, [this](string args){ cli_add(args); }});
    commands_storage.push_back({"remove", "Remove a schedule block by numeric ID", "$scheduler remove 1", 1, [this](string args){ cli_remove(args); }});
    commands_storage.push_back({"timezone", "Set timezone offset (e.g. GMT-08:00)", "$scheduler timezone GMT-08:00", 1, [this](string args){ cli_timezone(args); }});
    commands_storage.push_back({"print_schedules", "Print all schedules as JSON", "$scheduler print_schedules", 0, [this](string args){ cli_print_schedules(args); }});
}

void Scheduler::begin_routines_init(const ModuleConfig& cfg) {
    if (!is_enabled()) return;

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

    // 2. Configure NTP
    controller.serial_port.print("Scheduler: Syncing time via multiple NTP servers...");

    esp_sntp_config_t sntp_cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    sntp_cfg.start = false;
    sntp_cfg.wait_for_sync = true;
    esp_netif_sntp_init(&sntp_cfg);

    esp_sntp_setservername(1, "time.google.com");
    esp_sntp_setservername(2, "time.cloudflare.com");
    esp_sntp_setservername(3, "time.windows.com");

    esp_netif_sntp_start();

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
            if (!cmd.empty()) {
                std::string exec_cmd = cmd;
                if (exec_cmd.front() != '$') {
                    exec_cmd = "$" + exec_cmd;
                }
                controller.command_parser.parse(exec_cmd);
            }
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
    s += "NTP Servers: pool.ntp.org, time.google.com, time.cloudflare.com, time.windows.com\n";

    CurrentTimeInfo now;
    s += "Current time: " + (get_current_time(now) ? format_datetime(now) : "unavailable (waiting on sync)") + "\n";

    if (schedules.empty()) {
        s += "No schedules active.";
    } else {
        s += "--- Active Schedules ---\n";
        for (const auto& sched : schedules) {
            char time_buf[64];
            snprintf(time_buf, sizeof(time_buf), "%02d:%02d to %02d:%02d",
                     sched.start_minute / 60, sched.start_minute % 60,
                     sched.end_minute / 60, sched.end_minute % 60);

            std::string cmds_str;
            for (size_t i = 0; i < sched.commands.size(); ++i) {
                cmds_str += "\"" + sched.commands[i] + "\"";
                if (i < sched.commands.size() - 1) cmds_str += ", ";
            }

            s += "  [ID: " + sched.id + "] Day " + day_to_str(sched.day) +
                 " | " + time_buf + " | Cmds: [" + cmds_str + "]\n";
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
        json << "\"day\": " << (int)sched.day << ", ";

        json << "\"start_time\": " << sched.start_minute << ", ";
        json << "\"end_time\": " << sched.end_minute;

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
    std::string id_str, color, day_str, start_str, end_str;

    if (!(iss >> id_str >> color >> day_str >> start_str >> end_str)) {
        return false;
    }

    uint8_t day;
    if (!parse_day(day_str, day)) return false;

    uint16_t start, end;
    if (!parse_time(start_str, start) || !parse_time(end_str, end)) return false;

    if (start >= end) return false;

    std::string blob;
    std::getline(iss, blob);
    auto cmds = extract_commands(blob);
    if (cmds.empty()) return false;

    for (const auto& s : schedules) {
        if (s.id == id_str) return false;
    }

    schedules.push_back({id_str, day, start, end, color, cmds, config, -1});

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
// Robust NVS Management
// -----------------------------------------------------------------------------
void Scheduler::load_from_nvs() {
    schedules.clear();

    std::string ids_str = controller.nvs.read_str(nvs_key, "sched_ids");
    auto ids = split_string(ids_str, ',');

    for (const auto& id : ids) {
        std::string cfg = controller.nvs.read_str(nvs_key, "cfg_" + id);
        if (!cfg.empty()) {
            add_schedule(cfg);
        }
    }
    loaded_from_nvs = true;
}

void Scheduler::nvs_save_active_ids() {
    std::string ids_str;
    for (size_t i = 0; i < schedules.size(); ++i) {
        ids_str += schedules[i].id;
        if (i < schedules.size() - 1) ids_str += ",";
    }
    controller.nvs.write_str(nvs_key, "sched_ids", ids_str);
}

void Scheduler::nvs_save_config(const std::string& id, const std::string& cfg) {
    controller.nvs.write_str(nvs_key, "cfg_" + id, cfg);
}

void Scheduler::nvs_delete_config(const std::string& id) {
    controller.nvs.remove(nvs_key, "cfg_" + id);
}

void Scheduler::nvs_clear_all() {
    for (const auto& sched : schedules) {
        controller.nvs.remove(nvs_key, "cfg_" + sched.id);
    }
    controller.nvs.remove(nvs_key, "sched_ids");
    controller.nvs.remove(nvs_key, "sched_tz");
    controller.nvs.remove(nvs_key, "sched_tz_min");
}

// -----------------------------------------------------------------------------
// CLI Handlers
// -----------------------------------------------------------------------------
void Scheduler::cli_add(std::string_view args) {
    if (!is_enabled()) { controller.serial_port.print("Module disabled."); return; }

    uint16_t next_id = 1;
    for (const auto& s : schedules) {
        try {
            uint16_t current_id = std::stoul(s.id);
            if (current_id >= next_id) {
                next_id = current_id + 1;
            }
        } catch (...) {
            // Ignore if any legacy IDs are non-numeric strings
        }
    }

    std::string new_id_str = std::to_string(next_id);
    std::string config = new_id_str + " " + std::string(args);

    if (add_schedule(config)) {
        nvs_save_config(new_id_str, config);
        nvs_save_active_ids();
        controller.serial_port.print("Added schedule block with ID: " + new_id_str);
    } else {
        controller.serial_port.print("Error: Invalid config.\nUsage: $scheduler add #COLOR DAY_STR HH:MM HH:MM \"cmds\"\nExample: $scheduler add #33FF33 MO 09:00 10:00 \"\\\"$led turn_on\\\"\"");
    }
}

void Scheduler::cli_remove(std::string_view args) {
    if (!is_enabled()) { controller.serial_port.print("Module disabled."); return; }

    std::string id(args);
    remove_schedule(id);

    nvs_delete_config(id);
    nvs_save_active_ids();

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

void Scheduler::cli_print_schedules(std::string_view args) {
    if (!is_enabled()) { controller.serial_port.print("Module disabled."); return; }
    controller.serial_port.print(get_all_json());
}