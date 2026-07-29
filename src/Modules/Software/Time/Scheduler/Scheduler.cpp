// src/Modules/Software/Scheduler/Scheduler.cpp
#include "Scheduler.h"
#include "../../../Module/ModuleController.h"


Scheduler::Scheduler(ModuleController& controller)
    : Module(controller,
             /* id                  */ "schedule",
             /* name                */ "Scheduler",
             /* description         */ "Runs stored commands on a weekly schedule",
             /* requires_init_setup */ false,
             /* can_be_disabled     */ true,
             /* has_cli_cmds        */ true) {}


void Scheduler::begin_routines_regular(const ModuleConfig& cfg) {
    load_from_nvs();
}

void Scheduler::loop() {
    const std::time_t current_time = std::time(nullptr); // use Time.h

    for (const ScheduleBlock& schedule : schedules) {
        if (schedule.day == day && schedule.start_time == minute_of_day) {
            execute(schedule);
        }
    }
}

void Scheduler::reset(bool verbose, bool do_restart, bool keep_enabled) {
    data.buttons.clear();
    controller.nvs.remove(id, "schedules");
    Module::reset(verbose, do_restart, keep_enabled);
}

std::string Scheduler::status(bool verbose) const {
    text = schedules..as_json_str();
    if (verbose) controller.serial.print(text);
    return text;
}

bool Scheduler::add(uint16_t start_time,
                    uint16_t end_time,
                    uint8_t day,
                    std::string displayed_color,
                    std::vector<std::string> commands) {
    if (is_disabled()) return;
    if (start_time >= 1440 || end_time >= 1440 || day > 6) return false;

    ScheduleBlock block;
    block.id = // auto assigned as the highest current id + 1
    block.start_time = start_time;
    block.end_time = end_time;
    block.day = day;
    block.displayed_color = std::move(displayed_color);
    block.commands = std::move(commands);


    schedules.push_back(std::move(block));

    save_to_nvs();
}

bool Scheduler::remove(uint8_t sid) {
    const auto new_end = std::remove_if(
        schedules.begin(),
        schedules.end(),
        [schedule_id](const ScheduleBlock& schedule) {
            return schedule.id == schedule_id;
        }
    );

    if (new_end == schedules.end()) return false;

    schedules.erase(new_end, schedules.end());
    save_to_nvs();
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
    controller.nvs.write_flex(id, "schedules", schedules);
}

void Scheduler::cli_add(std::string_view args) {
    std::istringstream input{std::string(args)};

    std::string id_text;
    std::string start_text;
    std::string end_text;
    std::string day_text;
    std::string color;

    if (!(input >> id_text >> start_text >> end_text >> day_text >> color)) {
        controller.serial_port.print(
            "Scheduler: usage: add <start> <end> <day> <RRGGBB> [cmd1|cmd2|...]\n"
        );
        return;
    }

    here use
    try {
        const unsigned long pin_value = std::stoul(args[0]);
        const unsigned long debounce  = std::stoul(args[4]);

        if (pin_value > std::numeric_limits<uint8_t>::max()) {
//
//     uint32_t id = 0;
//     uint32_t start_time = 0;
//     uint32_t end_time = 0;
//     uint32_t day = 0;
//
//     if (!parse_unsigned(id_text, id) || id > UINT8_MAX ||
//         !parse_unsigned(start_text, start_time) || start_time >= 1440 ||
//         !parse_unsigned(end_text, end_time) || end_time >= 1440 ||
//         !parse_unsigned(day_text, day) || day > 6) {
//         controller.serial_port.print("Scheduler: invalid numeric argument\n");
//         return;
//     }
//
//     std::string command_text;
//     std::getline(input, command_text);
//
//     const bool added = add(
//         static_cast<uint8_t>(id),
//         static_cast<uint16_t>(start_time),
//         static_cast<uint16_t>(end_time),
//         static_cast<uint8_t>(day),
//         std::move(color),
//         split_commands(command_text)
//     );
//
//     controller.serial_port.print(
//         added
//             ? "Scheduler: schedule saved\n"
//             : "Scheduler: invalid schedule\n"
//     );
}

void Scheduler::cli_remove(std::string_view args) {
    const std::string id_text = trim_copy(args);
    uint32_t id = 0;


    if (!parse_unsigned(id_text, id) || id > UINT8_MAX) {
        controller.serial_port.print("Scheduler: usage: remove <id>\n");
        return;
    }

    controller.serial_port.print(
        remove(static_cast<uint8_t>(id))
            ? "Scheduler: schedule removed\n"
            : "Scheduler: schedule not found\n"
    );
}
