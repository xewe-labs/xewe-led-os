#pragma once

#include "../../Module/Module.h"
#include <string>
#include <string_view>

class Time : public Module {
public:
    struct TimeConfig : public ModuleConfig {};

    struct CurrentTimeInfo {
        bool is_valid{false};
        uint8_t day{0};            // 0 = Monday
        uint16_t minute_of_day{0}; // Minutes from midnight
        int32_t daystamp{0};
        int year{0}, month{0}, day_of_month{0}, hour{0}, minute{0}, second{0};
    };

    explicit Time(SystemController& controller);

    void begin_routines_init(const ModuleConfig& cfg) override;
    void begin_routines_common(const ModuleConfig& cfg) override;

    void loop() override;
    void reset(bool verbose=false, bool do_restart=true, bool keep_enabled=true) override;
    string status(bool verbose=false) const override;

    // Public APIs for other modules
    bool is_time_ready() const { return time_ready && timezone_ready; }
    CurrentTimeInfo get_current_time() const;

private:
    bool time_ready{false};
    bool timezone_ready{false};
    int32_t active_timezone_bias_min{0};

    void apply_timezone(int32_t bias_minutes);
    void cli_timezone(std::string_view args);
};