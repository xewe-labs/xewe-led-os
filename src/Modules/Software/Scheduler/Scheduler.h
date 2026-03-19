// src/Modules/System/Scheduler/Scheduler.h
#pragma once

#include "../../Module/Module.h"

struct SchedulerConfig : public ModuleConfig {};

class Scheduler : public Module {
public:
    explicit                    Scheduler                   (SystemController& controller);

    void                        begin_routines_regular      (const ModuleConfig& cfg)         override;

    void                        loop                        ()                                override;

    void                        reset                       (const bool verbose=false,
                                                             const bool do_restart=true,
                                                             const bool keep_enabled=true)      override;

    string                      status                      (const bool verbose=false)        const override;

    void                        load_configs                (const std::vector<std::string>& configs);
    bool                        add_schedule_from_config    (const std::string& config);
    void                        remove_schedule             (uint32_t schedule_id);

private:
    struct ScheduleBlock {
        uint32_t                    s_id                    {0};
        uint8_t                     day                     {0};      // 0 = Monday
        uint16_t                    minute_of_day           {0};      // 0..1439
        std::vector<std::string>    commands;
        std::string                 config_str;
        int32_t                     last_executed_daystamp  {-1};
    };

    struct CurrentTimeInfo {
        uint8_t                     day                     {0};      // 0 = Monday
        uint16_t                    minute_of_day           {0};      // 0..1439
        int32_t                     daystamp                {-1};     // unique-ish per calendar day
    };

    bool                        parse_config_string         (const std::string& config, ScheduleBlock& block) const;
    bool                        parse_commands_blob         (const std::string& blob,
                                                             std::vector<std::string>& commands) const;
    bool                        unwrap_outer_cmds_blob      (const std::string& in,
                                                             std::string& out) const;

    static bool                 parse_uint32_strict         (const std::string& s, uint32_t& value);
    static std::string          escape_command              (const std::string& cmd);
    static std::string          build_config_string         (uint8_t day,
                                                             uint16_t minute_of_day,
                                                             const std::vector<std::string>& commands);
    static std::string          format_minute_of_day        (uint16_t minute_of_day);

    bool                        get_current_time            (CurrentTimeInfo& out) const;

    void                        load_from_nvs               ();
    bool                        nvs_has_exact_config        (const std::string& config_str) const;
    bool                        nvs_remove_exact_config     (const std::string& config_str);
    void                        nvs_append_config           (const std::string& cfg);
    void                        nvs_clear_all               ();

    void                        scheduler_add_cli           (std::string_view args);
    void                        scheduler_remove_cli        (std::string_view args);

    std::vector<ScheduleBlock>  schedules;
    bool                        loaded_from_nvs             {false};
};