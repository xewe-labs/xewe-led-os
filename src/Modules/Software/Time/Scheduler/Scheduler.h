// src/Modules/Software/Scheduler/Scheduler.h
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "../../../Module/Module.h"
#include "../../Core/Nvs/FlexData.h"

struct SchedulerConfig : public ModuleConfig {};

class Scheduler : public Module {
public:
    using CommandHandler = std::function<void(std::string_view)>;

    explicit Scheduler(ModuleController& controller);

    void begin_routines_regular(const ModuleConfig& cfg) override;

    void loop() override;
    void reset(bool verbose = false,
               bool do_restart = true,
               bool keep_enabled = true) override;
    std::string status(bool verbose = false) const override;

    // 'id' is automatically assigned, so it is removed from the parameter list.
    bool add(uint16_t start_time,
             uint16_t end_time,
             uint8_t day,
             std::string displayed_color,
             std::vector<std::string> commands);

    bool remove(uint8_t schedule_id);

    void load_from_nvs();
    void save_to_nvs();

    void set_command_handler(CommandHandler handler);

private:
    struct ScheduleBlock : FlexData<ScheduleBlock> {
        uint8_t                  id = 0;
        uint16_t                 start_time = 0;  // minutes from midnight
        uint16_t                 end_time = 0;    // minutes from midnight
        uint8_t                  day = 0;         // 0=Sunday ... 6=Saturday
        std::string              displayed_color = "000000";
        std::vector<std::string> commands;

        static constexpr auto fields() {
            return std::make_tuple(
                fld("id", &ScheduleBlock::id),
                fld("start_time", &ScheduleBlock::start_time),
                fld("end_time", &ScheduleBlock::end_time),
                fld("day", &ScheduleBlock::day),
                fld("displayed_color", &ScheduleBlock::displayed_color),
                fld("commands", &ScheduleBlock::commands)
            );
        }
    };

    // FlexData wrapper to allow storing/retrieving the full array via Nvs
    struct SchedulerData : FlexData<SchedulerData> {
        std::vector<ScheduleBlock> schedules;

        static constexpr auto fields() {
            return std::make_tuple(
                fld("schedules", &SchedulerData::schedules)
            );
        }
    };

    std::vector<ScheduleBlock> schedules;
    CommandHandler             command_handler;
    int16_t                    last_processed_minute = -1;

    void execute(const ScheduleBlock& schedule);

    // CLI callbacks. Expected formats:
    // add:    <start> <end> <day> <RRGGBB> [cmd1|cmd2|...]
    // remove: <id>
    void cli_add(std::string_view args);
    void cli_remove(std::string_view args);
};