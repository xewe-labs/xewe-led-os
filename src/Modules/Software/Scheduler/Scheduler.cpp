// src/Modules/Software/Scheduler/Scheduler.cpp
#include "Scheduler.h"
#include "../../../SystemController/SystemController.h"

Scheduler::Scheduler(SystemController& controller)
    : Module(controller, "Scheduler", "Scheduled CLI executions", "schedule", true, true, true)
{
    DBG_PRINTF(Scheduler, "Scheduler(): constructing module.\n");
    commands_storage.push_back({"add", "Add schedule: config_string", "schedule add #FF00FF TH 21:41 22:00 \"\\\"cmd\\\"\"", 5, [this](std::string args){ cli_add(args); }});
    commands_storage.push_back({"remove", "Remove schedule by ID", "schedule remove 1", 1, [this](std::string args){ cli_remove(args); }});
    commands_storage.push_back({"json", "Get JSON schedules", "schedule json", 0, [this](std::string args){ cli_print_schedules(args); }});
    DBG_PRINTF(Scheduler, "Scheduler(): registered %u commands.\n", (unsigned)commands_storage.size());
}

void Scheduler::begin_routines_init(const ModuleConfig& cfg) {
    DBG_PRINTF(Scheduler, "begin_routines_init(): called.\n");
}

void Scheduler::begin_routines_common(const ModuleConfig& cfg) {
    DBG_PRINTF(Scheduler, "begin_routines_common(): enabled=%s, loaded_from_nvs=%s\n",
               is_enabled() ? "true" : "false",
               loaded_from_nvs ? "true" : "false");
    if (!is_enabled()) return;
    if (!loaded_from_nvs) {
        DBG_PRINTF(Scheduler, "begin_routines_common(): loading schedules from NVS.\n");
        load_from_nvs();
    }
}

void Scheduler::loop() {
    if (is_disabled() || !controller.time.is_time_ready()) return;

    auto now_opt = controller.time.get_current_time();
    if (!now_opt.has_value()) {
        DBG_PRINTF(Scheduler, "loop(): time ready but no current time available.\n");
        return;
    }

    const auto& now = now_opt.value();

    if (now.minute_of_day == last_executed_minute) return;
    DBG_PRINTF(Scheduler, "loop(): checking schedules for day=%u minute_of_day=%u last_executed_minute=%d\n",
               (unsigned)now.day, (unsigned)now.minute_of_day, (int)last_executed_minute);
    last_executed_minute = now.minute_of_day;

    for (const auto& sched : schedules) {
        DBG_PRINTF(Scheduler, "loop(): evaluating schedule id=%u day=%u start=%u end=%u commands=%u\n",
                   (unsigned)sched.id,
                   (unsigned)sched.day,
                   (unsigned)sched.start_time,
                   (unsigned)sched.end_time,
                   (unsigned)sched.commands.size());
        if (sched.day != now.day || sched.start_time != now.minute_of_day) {
            continue;
        }
        DBG_PRINTF(Scheduler, "loop(): schedule id=%u matched current time.\n", (unsigned)sched.id);

        for (const auto& cmd : sched.commands) {
            if (cmd.empty()) {
                DBG_PRINTF(Scheduler, "loop(): skipping empty command block for schedule id=%u.\n", (unsigned)sched.id);
                continue;
            }

            // Split the command string by the '$' delimiter
            std::istringstream cmd_stream(cmd);
            std::string sub_cmd;

            while (std::getline(cmd_stream, sub_cmd, '$')) {
                // Trim leading and trailing whitespace
                auto start_pos = sub_cmd.find_first_not_of(" \t\r\n");
                if (start_pos == std::string::npos) continue; // Skip empty/whitespace-only chunks

                auto end_pos = sub_cmd.find_last_not_of(" \t\r\n");
                std::string exec_cmd = sub_cmd.substr(start_pos, end_pos - start_pos + 1);

                if (!exec_cmd.empty()) {
                    // Re-append the $ as expected by the parser
                    exec_cmd = "$" + exec_cmd;
                    DBG_PRINTF(Scheduler, "loop(): executing command for schedule id=%u: %s\n",
                               (unsigned)sched.id, exec_cmd.c_str());
                    controller.command_parser.parse(exec_cmd);
                }
            }
        }
    }
}

void Scheduler::reset(bool verbose, bool do_restart, bool keep_enabled) {
    DBG_PRINTF(Scheduler, "reset(): verbose=%s do_restart=%s keep_enabled=%s\n",
               verbose ? "true" : "false",
               do_restart ? "true" : "false",
               keep_enabled ? "true" : "false");
    nvs_clear_all();
    schedules.clear();
    loaded_from_nvs = false;
    Module::reset(verbose, do_restart, keep_enabled);
}

std::string Scheduler::status(bool verbose) const {
    DBG_PRINTF(Scheduler, "status(): disabled=%s schedule_count=%u verbose=%s\n",
               is_disabled() ? "true" : "false",
               (unsigned)schedules.size(),
               verbose ? "true" : "false");
    if (is_disabled()) return "Scheduler disabled";
    return schedules.empty() ? "No active schedules." : "Active schedules: " + std::to_string(schedules.size());
}

// --- NEW APIs ---

bool Scheduler::add(const std::string& config) {
    // Public add creates a new ID (0) and saves to NVS (true)
    return add_internal(config, 0, true);
}

bool Scheduler::add_internal(const std::string& config, uint8_t forced_id, bool save_to_nvs) {
    DBG_PRINTF(Scheduler, "add_internal(): config='%s', forced_id=%u\n", config.c_str(), (unsigned)forced_id);
    std::istringstream iss(config);
    std::string color_str, day_str, start_str, end_str, blob;

    if (!(iss >> color_str >> day_str >> start_str >> end_str)) {
        DBG_PRINTF(Scheduler, "add_internal(): failed to parse color/day/start/end tokens.\n");
        return false;
    }
    std::getline(iss, blob);

    if (!color_str.empty() && color_str[0] == '#') color_str = color_str.substr(1);
    if (color_str.length() != 6) {
        DBG_PRINTF(Scheduler, "add_internal(): invalid color length for '%s'\n", color_str.c_str());
        return false;
    }

    uint8_t day;
    if (!xewe::str::parse_day(day_str, day)) {
        DBG_PRINTF(Scheduler, "add_internal(): failed to parse day '%s'\n", day_str.c_str());
        return false;
    }

    uint16_t start, end;
    if (!xewe::str::parse_time(start_str, start) || !xewe::str::parse_time(end_str, end)) {
        DBG_PRINTF(Scheduler, "add_internal(): failed to parse start/end time ('%s','%s')\n",
                   start_str.c_str(), end_str.c_str());
        return false;
    }
    if (start >= end) {
        DBG_PRINTF(Scheduler, "add_internal(): invalid range start=%u end=%u\n", (unsigned)start, (unsigned)end);
        return false;
    }

    auto cmds = xewe::str::extract_commands(blob);
    if (cmds.empty()) {
        DBG_PRINTF(Scheduler, "add_internal(): no commands extracted from blob.\n");
        return false;
    }

    uint8_t next_id = forced_id;
    if (next_id == 0) {
        next_id = 1;
        for (const auto& s : schedules) {
            if (s.id >= next_id) next_id = s.id + 1;
        }
    }

    DBG_PRINTF(Scheduler, "add_internal(): assigning id=%u day=%u start=%u end=%u color='%s'\n",
               (unsigned)next_id, (unsigned)day, (unsigned)start, (unsigned)end, color_str.c_str());

    schedules.push_back({next_id, start, end, day, color_str, cmds});
    std::sort(schedules.begin(), schedules.end(), [](const ScheduleBlock& a, const ScheduleBlock& b) {
        return (a.day == b.day) ? (a.start_time < b.start_time) : (a.day < b.day);
    });

    if (save_to_nvs) {
        nvs_save_config(next_id, config);
        nvs_save_active_ids();
    }
    return true;
}

void Scheduler::remove(uint8_t id) {
    DBG_PRINTF(Scheduler, "remove(): id=%u\n", (unsigned)id);
    auto it = std::remove_if(schedules.begin(), schedules.end(), [id](const ScheduleBlock& s) { return s.id == id; });
    if (it != schedules.end()) {
        DBG_PRINTF(Scheduler, "remove(): schedule found, erasing.\n");
        schedules.erase(it, schedules.end());
        nvs_delete_config(id);
        nvs_save_active_ids();
        DBG_PRINTF(Scheduler, "remove(): schedule removed. total schedules=%u\n", (unsigned)schedules.size());
    } else {
        DBG_PRINTF(Scheduler, "remove(): schedule id=%u not found.\n", (unsigned)id);
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
    DBG_PRINTF(Scheduler, "load_from_nvs(): loading schedules from NVS.\n");
    schedules.clear();
    std::string ids_str = controller.nvs.read_str(nvs_key, "ids");
    DBG_PRINTF(Scheduler, "load_from_nvs(): ids='%s'\n", ids_str.c_str());

    auto ids = xewe::str::split_lines_sv(ids_str, ',');

    for (const auto& id_sv : ids) {
        if (id_sv.empty()) continue;

        std::string id_str(id_sv);
        uint8_t parsed_id = 0;
        try {
            parsed_id = static_cast<uint8_t>(std::stoul(id_str));
        } catch (...) {
            continue; // Prevent crash on corrupted NVS string
        }

        std::string cfg = controller.nvs.read_str(nvs_key, "cfg_" + id_str);
        if (!cfg.empty()) {
            DBG_PRINTF(Scheduler, "load_from_nvs(): loaded config for id=%u\n", (unsigned)parsed_id);
            // Pass the parsed ID, and false to ensure we don't save to NVS while loading
            add_internal(cfg, parsed_id, false);
        }
    }
    loaded_from_nvs = true;
    DBG_PRINTF(Scheduler, "load_from_nvs(): done. loaded_from_nvs=true schedules=%u\n", (unsigned)schedules.size());
}

void Scheduler::nvs_save_active_ids() {
    std::string ids_str;
    for (size_t i = 0; i < schedules.size(); ++i) {
        ids_str += std::to_string(schedules[i].id);
        if (i < schedules.size() - 1) ids_str += ",";
    }
    DBG_PRINTF(Scheduler, "nvs_save_active_ids(): writing ids='%s'\n", ids_str.c_str());
    controller.nvs.write_str(nvs_key, "ids", ids_str);
}

void Scheduler::nvs_save_config(uint8_t id, const std::string& cfg) {
    DBG_PRINTF(Scheduler, "nvs_save_config(): id=%u cfg='%s'\n", (unsigned)id, cfg.c_str());
    controller.nvs.write_str(nvs_key, "cfg_" + std::to_string(id), cfg);
}

void Scheduler::nvs_delete_config(uint8_t id) {
    DBG_PRINTF(Scheduler, "nvs_delete_config(): id=%u\n", (unsigned)id);
    controller.nvs.remove(nvs_key, "cfg_" + std::to_string(id));
}

void Scheduler::nvs_clear_all() {
    DBG_PRINTF(Scheduler, "nvs_clear_all(): clearing %u schedules.\n", (unsigned)schedules.size());
    for (const auto& sched : schedules) nvs_delete_config(sched.id);
    controller.nvs.remove(nvs_key, "ids");
    DBG_PRINTF(Scheduler, "nvs_clear_all(): removed ids key.\n");
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