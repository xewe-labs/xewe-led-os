// src/Modules/Software/Time/Scheduler/Scheduler.cpp
#include "Scheduler.h"
#include "../../../Module/ModuleController.h"


Scheduler::Scheduler(ModuleController& controller)
    : Module(controller,
             /* id                  */ "schedule",
             /* name                */ "Scheduler",
             /* description         */ "Runs stored commands on a weekly schedule",
             /* requires_init_setup */ true,
             /* can_be_disabled     */ false,
             /* has_cli_cmds        */ true)
{
    commands_storage.push_back(Command{
        "add",
        "Add schedule: <start> <end> <day> <RRGGBB> \"<cmd1|cmd2>\"",
        std::string("$") + id + " add 480 1020 1 FF0000 \"led on|relay 1\"",
        5,
        [this](std::span<const std::string> args){ cli_add(args); }
    });

    commands_storage.push_back(Command{
        "remove",
        "Remove schedule: <id>",
        std::string("$") + id + " remove 1",
        1,
        [this](std::span<const std::string> args){ cli_remove(args); }
    });
}
void Scheduler::begin_routines_init(const ModuleConfig& cfg) {
    controller.serial_port.print("Will be available after auto-reboot.");
}

void Scheduler::begin_routines_regular(const ModuleConfig& cfg) {
    controller.serial_port.printf("Loaded %d schedule blocks.",  load_from_nvs());
}

void Scheduler::loop() {
    if (is_disabled() || data.schedules.empty()) return;

    // FIX: Get struct directly
    tm time_info = controller.time.get_current_time();

    // FIX: Verify time is synced (Year >= 1970)
    if (time_info.tm_year < 70) return;

    // FIX: Calculate minute_of_day manually
    int16_t current_minute_of_day = (time_info.tm_hour * 60) + time_info.tm_min;

    if (current_minute_of_day == last_processed_minute) return;
    last_processed_minute = current_minute_of_day;

    // FIX: tm_wday is 0=Sunday -> 6=Saturday. Map to Scheduler's 0=Monday -> 6=Sunday
    uint8_t current_day = (time_info.tm_wday + 6) % 7;

    for (const ScheduleBlock& schedule : data.schedules) {
        if (schedule.day == current_day && schedule.start_time == current_minute_of_day) {
            execute(schedule);
        }
    }
}

void Scheduler::reset(bool verbose, bool do_restart, bool keep_enabled) {
    data.schedules.clear();
    controller.nvs.remove(id, "schedules");
    Module::reset(verbose, do_restart, keep_enabled);
}

std::string Scheduler::status(bool verbose) const {
    if (is_disabled()) return "Scheduler disabled";

    std::string text = data.as_json_str();

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

    ScheduleBlock block;

    block.id = 0;
    for (const ScheduleBlock& s : data.schedules) {
        if (s.id >= block.id) {
            block.id = s.id + 1;
        }
    }

    block.start_time = start_time;
    block.end_time = end_time;
    block.day = day;
    block.displayed_color = std::move(displayed_color);
    block.commands = std::move(commands);

    data.schedules.push_back(std::move(block));
    save_to_nvs();

    return true;
}

bool Scheduler::remove(uint8_t sid) {
    const std::vector<ScheduleBlock>::iterator new_end = std::remove_if(
        data.schedules.begin(),
        data.schedules.end(),
        [sid](const ScheduleBlock& schedule) {
            return schedule.id == sid;
        }
    );

    if (new_end == data.schedules.end()) return false;

    data.schedules.erase(new_end, data.schedules.end());
    save_to_nvs();

    return true;
}

uint16_t Scheduler::load_from_nvs() {
    if (!controller.nvs.read_flex(id, "schedules", data))
        data.schedules.clear();
    return data.schedules.size();
}

void Scheduler::save_to_nvs() {
    if (is_disabled()) return;
    controller.nvs.write_flex(id, "schedules", data);
}

void Scheduler::execute(const ScheduleBlock& schedule) {
    for (const std::string& cmd : schedule.commands) {
        controller.command_executor.parse(cmd);
    }
}

void Scheduler::cli_add(std::span<const std::string> args) {
    std::optional<uint16_t> start_val = xewe::validate<uint16_t>(args[0], 0, 1439);
    std::optional<uint16_t> end_val   = xewe::validate<uint16_t>(args[1], 0, 1439);
    std::optional<uint8_t>  day_val   = xewe::validate<uint8_t>(args[2], 0, 6);
    std::optional<std::string> color_val = xewe::validate<std::string>(args[3], 6, 6);

    if (!start_val || !end_val || !day_val || !color_val) {
         controller.serial_port.print("Scheduler: invalid parameters or out of range (start/end 0-1439, day 0-6, color 6 chars)\n");
         return;
    }

    std::vector<std::string> commands;
    std::istringstream cmd_stream(args[4]);
    std::string token;

    while (std::getline(cmd_stream, token, '|')) {
        if (!token.empty()) {
            commands.push_back(token);
        }
    }

    const bool added = add(
        start_val.value(),
        end_val.value(),
        day_val.value(),
        color_val.value(),
        std::move(commands)
    );

    controller.serial_port.print(added ? "Scheduler: schedule saved\n" : "Scheduler: invalid schedule\n");
}

void Scheduler::cli_remove(std::span<const std::string> args) {
    std::optional<uint8_t> target_id = xewe::validate<uint8_t>(args[0], 0, 255);

    if (!target_id) {
        controller.serial_port.print("Scheduler: invalid ID or out of bounds\n");
        return;
    }

    if (remove(target_id.value())) {
        controller.serial_port.print("Scheduler: schedule removed\n");
    } else {
        controller.serial_port.print("Scheduler: schedule not found\n");
    }
}