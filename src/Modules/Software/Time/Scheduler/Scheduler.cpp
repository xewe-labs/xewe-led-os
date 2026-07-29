// src/Modules/Software/Scheduler/Scheduler.cpp
#include "Scheduler.h"
#include "../../../Module/ModuleController.h"
#include "../Time/Time.h"
#include <sstream>
#include <algorithm>
#include <limits>

Scheduler::Scheduler(ModuleController& controller)
    : Module(controller,
             /* id                  */ "schedule",
             /* name                */ "Scheduler",
             /* description         */ "Runs stored commands on a weekly schedule",
             /* requires_init_setup */ false,
             /* can_be_disabled     */ true,
             /* has_cli_cmds        */ true)
{
    commands_storage.push_back({"add", "Add schedule: <start> <end> <day> <RRGGBB> [cmd1|cmd2]", "$schedule add 480 1020 1 FF0000 \"$led turn_on $led set_rgb 255 0 0\"", 5, [this](std::string args){ cli_add(args); }});
    commands_storage.push_back({"remove", "Remove schedule: <id>", "$schedule remove 1", 1, [this](std::string args){ cli_remove(args); }});
}

void Scheduler::begin_routines_regular(const ModuleConfig& cfg) {
    load_from_nvs();
}

void Scheduler::loop() {
    if (is_disabled() || schedules.empty()) return;

    // Use the Time module API to retrieve the current synchronized time
    auto time_info = controller.time.get_current_time();
    if (!time_info.has_value()) return;

    // Prevent executing multiple times within the same minute
    if (time_info->minute_of_day == last_processed_minute) return;
    last_processed_minute = time_info->minute_of_day;

    for (const ScheduleBlock& schedule : schedules) {
        if (schedule.day == time_info->day && schedule.start_time == time_info->minute_of_day) {
            execute(schedule);
        }
    }
}

void Scheduler::reset(bool verbose, bool do_restart, bool keep_enabled) {
    schedules.clear();
    controller.nvs.remove(id, "schedules");
    Module::reset(verbose, do_restart, keep_enabled);
}

std::string Scheduler::status(bool verbose) const {
    if (is_disabled()) return "Scheduler disabled";

    // Wrap the std::vector inside the FlexData struct to serialize as JSON
    std::string text = schedules.as_json_str();

    if (verbose) {
        controller.serial_port.print(text);
    }
    return text;
}

bool Scheduler::add(uint16_t start_time,
                    uint16_t end_time,
                    uint8_t day,
                    std::string displayed_color,
                    std::vector<std::string> commands) {
    if (is_disabled()) return false;
    if (start_time >= 1440 || end_time >= 1440 || day > 6) return false;

    ScheduleBlock block;

    // Auto-assign the ID as the highest current ID + 1
    block.id = 0;
    for (const auto& s : schedules) {
        if (s.id >= block.id) {
            block.id = s.id + 1;
        }
    }

    block.start_time = start_time;
    block.end_time = end_time;
    block.day = day;
    block.displayed_color = std::move(displayed_color);
    block.commands = std::move(commands);

    schedules.push_back(std::move(block));
    save_to_nvs();

    return true;
}

bool Scheduler::remove(uint8_t sid) {
    const auto new_end = std::remove_if(
        schedules.begin(),
        schedules.end(),
        [sid](const ScheduleBlock& schedule) {
            return schedule.id == sid;
        }
    );

    if (new_end == schedules.end()) return false;

    schedules.erase(new_end, schedules.end());
    save_to_nvs();

    return true;
}

void Scheduler::load_from_nvs() {
    SchedulerData data;
    if (controller.nvs.read_flex(id, "schedules", data)) {
        schedules = std::move(data.schedules);
    } else {
        schedules.clear();
    }
}

void Scheduler::save_to_nvs() {
    SchedulerData data;
    data.schedules = schedules;
    controller.nvs.write_flex(id, "schedules", data);
}

void Scheduler::set_command_handler(CommandHandler handler) {
    command_handler = std::move(handler);
}

void Scheduler::execute(const ScheduleBlock& schedule) {
    if (!command_handler) return;
    for (const std::string& cmd : schedule.commands) {
        command_handler(cmd);
    }
}

void Scheduler::cli_add(std::string_view args) {
    std::istringstream input{std::string(args)};
    std::string start_text, end_text, day_text, color, cmd_string;

    if (!(input >> start_text >> end_text >> day_text >> color)) {
        controller.serial_port.print("Scheduler: usage: add <start> <end> <day> <RRGGBB> [cmd1|cmd2|...]\n");
        return;
    }

    std::getline(input >> std::ws, cmd_string); // Read the rest of the line

    try {
        const unsigned long start_val = std::stoul(start_text);
        const unsigned long end_val   = std::stoul(end_text);
        const unsigned long day_val   = std::stoul(day_text);

        if (start_val >= 1440 || end_val >= 1440 || day_val > 6) {
             controller.serial_port.print("Scheduler: parameters out of range (start/end < 1440, day <= 6)\n");
             return;
        }

        std::vector<std::string> commands;
        std::istringstream cmd_stream(cmd_string);
        std::string token;
        while (std::getline(cmd_stream, token, '|')) {
            if (!token.empty()) {
                commands.push_back(token);
            }
        }

        const bool added = add(
            static_cast<uint16_t>(start_val),
            static_cast<uint16_t>(end_val),
            static_cast<uint8_t>(day_val),
            color,
            std::move(commands)
        );

        controller.serial_port.print(added ? "Scheduler: schedule saved\n" : "Scheduler: invalid schedule\n");

    } catch (const std::exception&) {
        controller.serial_port.print("Scheduler: invalid numeric argument\n");
    }
}

void Scheduler::cli_remove(std::string_view args) {
    try {
        const unsigned long target_id = std::stoul(std::string(args));

        if (target_id > std::numeric_limits<uint8_t>::max()) {
            controller.serial_port.print("Scheduler: ID out of bounds\n");
            return;
        }

        if (remove(static_cast<uint8_t>(target_id))) {
            controller.serial_port.print("Scheduler: schedule removed\n");
        } else {
            controller.serial_port.print("Scheduler: schedule not found\n");
        }

    } catch (const std::exception&) {
        controller.serial_port.print("Scheduler: usage: remove <id>\n");
    }
}