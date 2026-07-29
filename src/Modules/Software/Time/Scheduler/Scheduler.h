// src/Modules/Software/Time/Scheduler/Scheduler.h
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>
#include "../Time.h"
#include <sstream>
#include <algorithm>
#include <optional>

#include "../../../Module/Module.h"
#include "../../../Core/Nvs/FlexData.h"

struct SchedulerConfig : public ModuleConfig {};

class Scheduler : public Module {
public:
    explicit Scheduler(ModuleController& controller);

    void begin_routines_init(const ModuleConfig& cfg) override;
    void begin_routines_regular(const ModuleConfig& cfg) override;

    void loop() override;
    void reset(bool verbose = false,
               bool do_restart = true,
               bool keep_enabled = true) override;
    std::string status(bool verbose = false) const override;

    bool add(uint16_t start_time,
             uint16_t end_time,
             uint8_t day,
             std::string displayed_color,
             std::vector<std::string> commands);

    bool remove(uint8_t schedule_id);

    uint16_t    load_from_nvs();
    void        save_to_nvs();

private:
    struct ScheduleBlock : FlexData<ScheduleBlock> {
        uint8_t                  id = 0;
        uint16_t                 start_time = 0;  // minutes from midnight
        uint16_t                 end_time = 0;    // minutes from midnight
        uint8_t                  day = 0;         // 0=Monday ... 6=Sunday
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

    struct SchedulerData : FlexData<SchedulerData> {
        std::vector<ScheduleBlock> schedules;

        static constexpr auto fields() {
            return std::make_tuple(
                fld("schedules", &SchedulerData::schedules)
            );
        }
    };

    SchedulerData              data;
    int16_t                    last_processed_minute = -1;

    void execute(const ScheduleBlock& schedule);

    // CLI callbacks
    void cli_add(std::span<const std::string> args);
    void cli_remove(std::span<const std::string> args);
};