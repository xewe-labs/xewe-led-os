// src/Modules/System/Scheduler/Scheduler.cpp

#include "Scheduler.h"
#include "../../../SystemController/SystemController.h"

#include <algorithm>
#include <cctype>
#include <ctime>

Scheduler::Scheduler(SystemController& controller)
      : Module(controller,
               /* module_name         */ "Scheduler",
               /* module_description  */ "Allows to bind CLI cmds to scheduled weekday/time slots",
               /* nvs_key             */ "sched",
               /* requires_init_setup */ false,
               /* can_be_disabled     */ true,
               /* has_cli_cmds        */ true)
{
    commands_storage.push_back({
        "add",
        "Add a schedule block: <day> <minute_of_day> \"\\\"<cmd1>\\\" \\\"<cmd2>\\\" ...\"",
        std::string("$") + lower(module_name) + " add 0 480 \"\\\"$system reboot\\\" \\\"$net sync\\\"\"",
        3,
        [this](std::string_view args){ scheduler_add_cli(args); }
    });

    commands_storage.push_back({
        "remove",
        "Remove a schedule block by ID",
        std::string("$") + lower(module_name) + " remove 0",
        1,
        [this](std::string_view args){ scheduler_remove_cli(args); }
    });
}

void Scheduler::begin_routines_regular(const ModuleConfig& cfg) {
    (void)cfg;

    if (is_enabled() && !loaded_from_nvs) {
        load_from_nvs();
    }
}

void Scheduler::loop() {
    if (is_disabled()) return;

    CurrentTimeInfo now_info;
    if (!get_current_time(now_info)) return;

    for (auto& schedule : schedules) {
        if (schedule.day != now_info.day) continue;
        if (schedule.minute_of_day != now_info.minute_of_day) continue;
        if (schedule.last_executed_daystamp == now_info.daystamp) continue;

        for (const auto& cmd : schedule.commands) {
            if (cmd.empty()) continue;
            controller.command_parser.parse(cmd);
        }

        schedule.last_executed_daystamp = now_info.daystamp;
    }
}

void Scheduler::reset(const bool verbose, const bool do_restart, const bool keep_enabled) {
    nvs_clear_all();
    schedules.clear();
    Module::reset(verbose, do_restart, keep_enabled);
}

string Scheduler::status(const bool verbose) const {
    if (is_disabled()) return "Scheduler module disabled";

    std::string s;

    CurrentTimeInfo now_info;
    if (get_current_time(now_info)) {
        s += "Current time: Day " + std::to_string(now_info.day)
          + ", Minute " + std::to_string(now_info.minute_of_day)
          + " (" + format_minute_of_day(now_info.minute_of_day) + ")\n";
    } else {
        s += "Current time: unavailable\n";
    }

    if (schedules.empty()) {
        s += "No schedules are currently active in memory.";
    } else {
        s += "--- Active Schedule Blocks (Live) ---\n";
        for (const auto& sched : schedules) {
            s += "  - ID: " + std::to_string(sched.s_id)
              + ", Day: " + std::to_string(sched.day)
              + ", Minute: " + std::to_string(sched.minute_of_day)
              + " (" + format_minute_of_day(sched.minute_of_day) + ")"
              + ", Commands: " + std::to_string(sched.commands.size()) + "\n";

            for (size_t i = 0; i < sched.commands.size(); ++i) {
                s += "      [" + std::to_string(i) + "] \"" + sched.commands[i] + "\"\n";
            }
        }
        s += "-------------------------------------";
    }

    if (verbose) controller.serial_port.print(s);
    return s;
}

void Scheduler::load_configs(const std::vector<std::string>& configs) {
    if (is_disabled()) return;

    schedules.clear();

    for (const auto& cfg : configs) {
        if (!cfg.empty()) add_schedule_from_config(cfg);
    }

    loaded_from_nvs = true;
}

bool Scheduler::add_schedule_from_config(const std::string& config) {
    if (is_disabled()) return false;

    ScheduleBlock new_schedule;
    if (!parse_config_string(config, new_schedule)) return false;

    uint32_t next_s_id = 0;
    for (const auto& s : schedules) {
        if (s.s_id >= next_s_id) {
            next_s_id = s.s_id + 1;
        }
    }
    new_schedule.s_id = next_s_id;

    schedules.push_back(std::move(new_schedule));

    std::sort(schedules.begin(), schedules.end(),
        [](const ScheduleBlock& a, const ScheduleBlock& b) {
            return a.s_id < b.s_id;
        });

    return true;
}

void Scheduler::remove_schedule(uint32_t schedule_id) {
    if (is_disabled()) return;

    schedules.erase(
        std::remove_if(schedules.begin(), schedules.end(),
            [schedule_id](const ScheduleBlock& sched) {
                return sched.s_id == schedule_id;
            }),
        schedules.end()
    );
}

bool Scheduler::parse_config_string(const std::string& config, ScheduleBlock& block) const {
    std::string s = config;
    trim(s);

    if (s.empty()) return false;

    const auto first_sp = s.find(' ');
    if (first_sp == std::string::npos) return false;

    const std::string day_str = s.substr(0, first_sp);
    s = s.substr(first_sp + 1);
    trim(s);

    const auto second_sp = s.find(' ');
    if (second_sp == std::string::npos) return false;

    const std::string minute_str = s.substr(0, second_sp);
    std::string cmds_blob = s.substr(second_sp + 1);
    trim(cmds_blob);

    uint32_t day_val = 0;
    uint32_t minute_val = 0;

    if (!parse_uint32_strict(day_str, day_val)) return false;
    if (!parse_uint32_strict(minute_str, minute_val)) return false;
    if (day_val > 6) return false;
    if (minute_val > 1439) return false;

    std::vector<std::string> commands;
    if (!parse_commands_blob(cmds_blob, commands)) return false;

    block.day = static_cast<uint8_t>(day_val);
    block.minute_of_day = static_cast<uint16_t>(minute_val);
    block.commands = std::move(commands);
    block.last_executed_daystamp = -1;
    block.config_str = build_config_string(block.day, block.minute_of_day, block.commands);

    return true;
}

bool Scheduler::parse_commands_blob(const std::string& blob, std::vector<std::string>& commands) const {
    commands.clear();

    std::string working = blob;
    trim(working);
    if (working.empty()) return false;

    std::string unwrapped;
    if (unwrap_outer_cmds_blob(working, unwrapped)) {
        working = std::move(unwrapped);
        trim(working);
    }

    if (working.empty()) return false;

    bool has_non_empty_command = false;
    size_t i = 0;

    while (i < working.size()) {
        while (i < working.size() && std::isspace(static_cast<unsigned char>(working[i]))) ++i;
        if (i >= working.size()) break;

        if (working[i] != '"') return false;
        ++i;

        std::string cmd;
        bool closed = false;

        while (i < working.size()) {
            const char ch = working[i++];

            if (ch == '\\') {
                if (i >= working.size()) return false;

                const char next = working[i++];
                if (next == '"' || next == '\\') {
                    cmd.push_back(next);
                } else {
                    cmd.push_back('\\');
                    cmd.push_back(next);
                }
                continue;
            }

            if (ch == '"') {
                closed = true;
                break;
            }

            cmd.push_back(ch);
        }

        if (!closed) return false;

        if (!cmd.empty()) has_non_empty_command = true;
        commands.push_back(std::move(cmd));

        while (i < working.size() && std::isspace(static_cast<unsigned char>(working[i]))) ++i;
    }

    if (commands.empty()) return false;
    if (!has_non_empty_command) return false;

    return true;
}

bool Scheduler::unwrap_outer_cmds_blob(const std::string& in, std::string& out) const {
    out.clear();

    if (in.size() < 2) return false;
    if (in.front() != '"' || in.back() != '"') return false;

    std::string inner;
    inner.reserve(in.size() - 2);

    bool escaped = false;
    for (size_t i = 1; i + 1 < in.size(); ++i) {
        const char ch = in[i];

        if (escaped) {
            inner.push_back(ch);
            escaped = false;
            continue;
        }

        if (ch == '\\') {
            escaped = true;
            continue;
        }

        inner.push_back(ch);
    }

    if (escaped) return false;

    std::string candidate = inner;
    trim(candidate);

    if (candidate.empty()) return false;
    if (candidate.front() != '"') return false;

    out = std::move(candidate);
    return true;
}

bool Scheduler::parse_uint32_strict(const std::string& s, uint32_t& value) {
    if (s.empty()) return false;

    try {
        size_t pos = 0;
        const unsigned long parsed = std::stoul(s, &pos, 10);
        if (pos != s.size()) return false;
        value = static_cast<uint32_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

std::string Scheduler::escape_command(const std::string& cmd) {
    std::string escaped;
    escaped.reserve(cmd.size());

    for (const char ch : cmd) {
        if (ch == '"' || ch == '\\') {
            escaped.push_back('\\');
        }
        escaped.push_back(ch);
    }

    return escaped;
}

std::string Scheduler::build_config_string(uint8_t day,
                                           uint16_t minute_of_day,
                                           const std::vector<std::string>& commands) {
    std::string cfg = std::to_string(day) + " " + std::to_string(minute_of_day);

    for (const auto& cmd : commands) {
        cfg += " \"";
        cfg += escape_command(cmd);
        cfg += "\"";
    }

    return cfg;
}

std::string Scheduler::format_minute_of_day(uint16_t minute_of_day) {
    const uint16_t hh = static_cast<uint16_t>(minute_of_day / 60);
    const uint16_t mm = static_cast<uint16_t>(minute_of_day % 60);

    std::string s;
    if (hh < 10) s += '0';
    s += std::to_string(hh);
    s += ':';
    if (mm < 10) s += '0';
    s += std::to_string(mm);

    return s;
}

bool Scheduler::get_current_time(CurrentTimeInfo& out) const {
    // Assumes the existing set_time() flow updates the system clock used by time()/localtime_r().
    const std::time_t now = std::time(nullptr);
    if (now <= 0) return false;

    std::tm tm_now {};
    if (localtime_r(&now, &tm_now) == nullptr) return false;

    out.day = static_cast<uint8_t>((tm_now.tm_wday + 6) % 7); // tm_wday: 0=Sunday -> 0=Monday
    out.minute_of_day = static_cast<uint16_t>((tm_now.tm_hour * 60) + tm_now.tm_min);
    out.daystamp = static_cast<int32_t>(((tm_now.tm_year + 1900) * 1000) + tm_now.tm_yday);

    return true;
}

/* --- NVS helpers --- */

void Scheduler::load_from_nvs() {
    if (is_disabled()) return;

    const int sched_count = controller.nvs.read_uint8(nvs_key, "sched_count", 0);

    std::vector<std::string> cfgs;
    cfgs.reserve(sched_count);

    for (int i = 0; i < sched_count; ++i) {
        const std::string key = "sched_cfg_" + std::to_string(i);
        std::string s = controller.nvs.read_str(nvs_key, key);
        if (!s.empty()) cfgs.emplace_back(std::move(s));
    }

    load_configs(cfgs);
}

bool Scheduler::nvs_has_exact_config(const std::string& config_str) const {
    if (is_disabled()) return false;

    const int sched_count = controller.nvs.read_uint8(nvs_key, "sched_count", 0);

    for (int i = 0; i < sched_count; ++i) {
        const std::string key = "sched_cfg_" + std::to_string(i);
        const std::string existing = controller.nvs.read_str(nvs_key, key);
        if (existing == config_str) return true;
    }

    return false;
}

bool Scheduler::nvs_remove_exact_config(const std::string& config_str) {
    if (is_disabled()) return false;

    const int sched_count = controller.nvs.read_uint8(nvs_key, "sched_count", 0);

    std::vector<std::string> kept_configs;
    kept_configs.reserve(sched_count);

    bool found = false;

    for (int i = 0; i < sched_count; ++i) {
        const std::string key = "sched_cfg_" + std::to_string(i);
        const std::string existing = controller.nvs.read_str(nvs_key, key);

        if (!found && existing == config_str) {
            found = true;
        } else {
            kept_configs.push_back(existing);
        }
    }

    if (!found) return false;

    nvs_clear_all();
    for (const auto& cfg : kept_configs) {
        if (!cfg.empty()) nvs_append_config(cfg);
    }

    return true;
}

void Scheduler::nvs_append_config(const std::string& cfg) {
    if (is_disabled()) return;

    const int sched_count = controller.nvs.read_uint8(nvs_key, "sched_count", 0);
    const std::string key = "sched_cfg_" + std::to_string(sched_count);

    controller.nvs.write_str(nvs_key, key, cfg);
    controller.nvs.write_uint8(nvs_key, "sched_count", sched_count + 1);
}

void Scheduler::nvs_clear_all() {
    if (is_disabled()) return;

    const int sched_count = controller.nvs.read_uint8(nvs_key, "sched_count", 0);

    for (int i = 0; i < sched_count; ++i) {
        const std::string key = "sched_cfg_" + std::to_string(i);
        controller.nvs.remove(nvs_key, key);
    }

    controller.nvs.write_uint8(nvs_key, "sched_count", 0);
}

/* --- CLI handlers --- */

void Scheduler::scheduler_add_cli(std::string_view args_sv) {
    if (is_disabled()) return;

    if (!is_enabled()) {
        controller.serial_port.print("Scheduler Module is disabled. Use '$scheduler enable'");
        return;
    }

    std::string args(args_sv);
    trim(args);

    ScheduleBlock parsed;
    if (!parse_config_string(args, parsed)) {
        controller.serial_port.print("Error: Invalid schedule configuration string.");
        return;
    }

    if (nvs_has_exact_config(parsed.config_str)) {
        controller.serial_port.print("Error: This exact schedule configuration already exists.");
        return;
    }

    if (!add_schedule_from_config(parsed.config_str)) {
        controller.serial_port.print("Error: Failed to add schedule block.");
        return;
    }

    nvs_append_config(parsed.config_str);
    controller.serial_port.print("Successfully added schedule block: " + parsed.config_str);
}

void Scheduler::scheduler_remove_cli(std::string_view args_sv) {
    if (is_disabled()) return;

    if (!is_enabled()) {
        controller.serial_port.print("Scheduler Module is disabled. Use '$scheduler enable'");
        return;
    }

    std::string id_str(args_sv);
    trim(id_str);

    uint32_t schedule_id = 0;
    if (!parse_uint32_strict(id_str, schedule_id)) {
        controller.serial_port.print("Error: Invalid schedule ID provided.");
        return;
    }

    auto it = std::find_if(schedules.begin(), schedules.end(),
        [schedule_id](const ScheduleBlock& sched) {
            return sched.s_id == schedule_id;
        });

    if (it == schedules.end()) {
        controller.serial_port.print("Error: No active schedule found with ID " + id_str);
        return;
    }

    const std::string cfg = it->config_str;
    const bool removed_from_nvs = nvs_remove_exact_config(cfg);

    remove_schedule(schedule_id);

    if (removed_from_nvs) {
        controller.serial_port.print("Successfully removed schedule block ID " + id_str);
    } else {
        controller.serial_port.print("Warning: Schedule removed from memory, but no matching NVS entry was found.");
    }
}