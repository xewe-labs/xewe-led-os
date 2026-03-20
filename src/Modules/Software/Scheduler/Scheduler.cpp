// src/Modules/Software/Scheduler/Scheduler.cpp
#include "Scheduler.h"
#include "../../../SystemController/SystemController.h"

Scheduler::Scheduler(SystemController& controller)
    : Module(controller, "Scheduler", "Scheduled CLI executions", "schedule", true, true, true)
{
    commands_storage.push_back({"add", "Add schedule: config_string", "schedule add #FF00FF TH 21:41 22:00 \"\\\"cmd\\\"\"", 5, [this](std::string args){ cli_add(args); }});
    commands_storage.push_back({"remove", "Remove schedule by ID", "schedule remove 1", 1, [this](std::string args){ cli_remove(args); }});
    commands_storage.push_back({"json", "Get JSON schedules", "schedule json", 0, [this](std::string args){ cli_print_schedules(args); }});
}

void Scheduler::begin_routines_init(const ModuleConfig& cfg) {}

void Scheduler::begin_routines_common(const ModuleConfig& cfg) {
    if (!is_enabled()) return;
    if (!loaded_from_nvs) load_from_nvs();
}

void Scheduler::loop() {
    if (is_disabled() || !controller.time.is_time_ready()) return;

    auto now_opt = controller.time.get_current_time();
    if (!now_opt.has_value()) return;

    const auto& now = now_opt.value();

    if (now.minute_of_day == last_executed_minute) return;
    last_executed_minute = now.minute_of_day;

    for (const auto& sched : schedules) {
        if (sched.day != now.day || sched.start_time != now.minute_of_day) {
            continue;
        }
        for (const auto& cmd : sched.commands) {
            if (!cmd.empty()) {
                std::string exec_cmd = cmd;
                if (exec_cmd.front() != '$') exec_cmd = "$" + exec_cmd;
                controller.command_parser.parse(exec_cmd);
            }
        }
    }
}

void Scheduler::reset(bool verbose, bool do_restart, bool keep_enabled) {
    nvs_clear_all();
    schedules.clear();
    loaded_from_nvs = false;
    Module::reset(verbose, do_restart, keep_enabled);
}

std::string Scheduler::status(bool verbose) const {
    if (is_disabled()) return "Scheduler disabled";
    return schedules.empty() ? "No active schedules." : "Active schedules: " + std::to_string(schedules.size());
}

// --- NEW APIs ---

bool Scheduler::add(const std::string& config) {
    std::istringstream iss(config);
    std::string color_str, day_str, start_str, end_str, blob;

    if (!(iss >> color_str >> day_str >> start_str >> end_str)) return false;
    std::getline(iss, blob);

    if (!color_str.empty() && color_str[0] == '#') color_str = color_str.substr(1);
    if (color_str.length() != 6) return false;

    uint8_t day;
    if (!xewe::str::parse_day(day_str, day)) return false;

    uint16_t start, end;
    if (!xewe::str::parse_time(start_str, start) || !xewe::str::parse_time(end_str, end)) return false;
    if (start >= end) return false;

    auto cmds = xewe::str::extract_commands(blob);
    if (cmds.empty()) return false;

    uint8_t next_id = 1;
    for (const auto& s : schedules) {
        if (s.id >= next_id) next_id = s.id + 1;
    }

    schedules.push_back({next_id, start, end, day, color_str, cmds});
    std::sort(schedules.begin(), schedules.end(), [](const ScheduleBlock& a, const ScheduleBlock& b) {
        return (a.day == b.day) ? (a.start_time < b.start_time) : (a.day < b.day);
    });

    nvs_save_config(next_id, config);
    nvs_save_active_ids();
    return true;
}

void Scheduler::remove(uint8_t id) {
    auto it = std::remove_if(schedules.begin(), schedules.end(), [id](const ScheduleBlock& s) { return s.id == id; });
    if (it != schedules.end()) {
        schedules.erase(it, schedules.end());
        nvs_delete_config(id);
        nvs_save_active_ids();
    }
}

std::string Scheduler::get_all_json() const {
    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < schedules.size(); ++i) {
        const auto& s = schedules[i];
        json << "{";
        json << "\"id\": " << (int)s.id << ", ";
        json << "\"start_time\": " << s.start_time << ", ";
        json << "\"end_time\": " << s.end_time << ", ";
        json << "\"day\": " << (int)s.day << ", ";
        json << "\"displayed_color\": \"" << s.displayed_color << "\", ";

        json << "\"commands\": [";
        for (size_t c = 0; c < s.commands.size(); ++c) {
            json << "\"" << xewe::str::escape_json(s.commands[c]) << "\"";
            if (c < s.commands.size() - 1) json << ", ";
        }
        json << "]";

        json << "}";
        if (i < schedules.size() - 1) json << ", ";
    }
    json << "]";
    return json.str();
}

// --- NVS Management ---

void Scheduler::load_from_nvs() {
    schedules.clear();
    std::string ids_str = controller.nvs.read_str(nvs_key, "ids");

    // REUSING YOUR EXISTING split_lines_sv!
    auto ids = xewe::str::split_lines_sv(ids_str, ',');

    for (const auto& id_sv : ids) {
        if (id_sv.empty()) continue; // Emulate original behavior of skipping empty tokens
        std::string id_str(id_sv);   // Convert view to string for concatenation

        std::string cfg = controller.nvs.read_str(nvs_key, "cfg_" + id_str);
        if (!cfg.empty()) {
            add(cfg);
        }
    }
    loaded_from_nvs = true;
}

void Scheduler::nvs_save_active_ids() {
    std::string ids_str;
    for (size_t i = 0; i < schedules.size(); ++i) {
        ids_str += std::to_string(schedules[i].id);
        if (i < schedules.size() - 1) ids_str += ",";
    }
    controller.nvs.write_str(nvs_key, "ids", ids_str);
}

void Scheduler::nvs_save_config(uint8_t id, const std::string& cfg) {
    controller.nvs.write_str(nvs_key, "cfg_" + std::to_string(id), cfg);
}

void Scheduler::nvs_delete_config(uint8_t id) {
    controller.nvs.remove(nvs_key, "cfg_" + std::to_string(id));
}

void Scheduler::nvs_clear_all() {
    for (const auto& sched : schedules) nvs_delete_config(sched.id);
    controller.nvs.remove(nvs_key, "ids");
}

// --- CLI Handlers ---

void Scheduler::cli_add(std::string_view args) {
    if (is_enabled() && add(std::string(args))) {
        controller.serial_port.print("Schedule added successfully.");
    } else {
        controller.serial_port.print("Error adding schedule.");
    }
}

void Scheduler::cli_remove(std::string_view args) {
    if (is_enabled()) {
        remove(static_cast<uint8_t>(std::stoul(std::string(args))));
        controller.serial_port.print("Schedule removed.");
    }
}

void Scheduler::cli_print_schedules(std::string_view args) {
    if (is_enabled()) controller.serial_port.print(get_all_json());
}