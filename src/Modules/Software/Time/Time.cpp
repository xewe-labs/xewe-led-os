// src/Modules/Software/Time/Time.cpp
#include "Time.h"
#include "../../Module/ModuleController.h"

Time::Time(ModuleController& controller)
    : Module(controller, "time", "Time", "Handles NTP and Timezone", true, true, true)
{
    commands_storage.push_back(Command{
        "set_zone",
        "Set timezone offset (e.g. GMT-0800)",
        "$time set_zone GMT-0800",
        1,
        [this](std::span<const std::string> args){ cli_timezone(args); }
    });
}

void Time::begin_routines_init(const ModuleConfig& cfg) {
    bool tz_valid = false;
    int32_t parsed_bias = 0;
    std::string tz_input;

    while (!tz_valid) {
        tz_input = controller.serial_port.get_string("Enter your timezone offset (e.g. GMT-0800)\nFor support visit:\nhttps://webbrowsertools.com/timezone/", 4, 15, 0, 0, "GMT+0000");
        if (xewe::str::parse_gmt_offset(tz_input, parsed_bias)) {
            tz_valid = true;
            controller.nvs.write<std::string>(id, "tz", xewe::str::upper(tz_input));
            controller.nvs.write<std::string>(id, "tz_min", std::to_string(parsed_bias));
            controller.serial_port.printf("Timezone set to %s", xewe::str::upper(tz_input).c_str());
        }
    }
}

void Time::begin_routines_common(const ModuleConfig& cfg) {
    std::string tz_str = controller.nvs.read<std::string>(id, "tz_min");
    if (!tz_str.empty()) {
        apply_timezone(std::stoi(tz_str));
        timezone_ready = true;
    }

    get_time_from_web();
}

bool Time::get_time_from_web(bool verbose) {

    esp_sntp_config_t sntp_cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    sntp_cfg.start = false;
    sntp_cfg.wait_for_sync = true;
    esp_netif_sntp_init(&sntp_cfg);
    esp_sntp_setservername(1, "time.google.com");
    esp_sntp_setservername(2, "time.cloudflare.com");
    esp_netif_sntp_start();

    controller.serial_port.print("Syncing time from server", "");
    while (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(200)) != ESP_OK) {
        controller.serial_port.print(".", "");
    }
    controller.serial_port.print("");

    print_current_time();

    time_ready = true;
    return true;
}

void Time::loop() {}

void Time::reset(bool verbose, bool do_restart, bool keep_enabled) {
    controller.nvs.remove(id, "tz");
    controller.nvs.remove(id, "tz_min");
    time_ready = timezone_ready = false;
    Module::reset(verbose, do_restart, keep_enabled);
}

std::string Time::status(bool verbose) const {
    if (is_disabled()) return "Time module disabled";
    return get_current_time().has_value() ? "Time Ready. TZ: " + xewe::str::format_gmt_offset(active_timezone_bias_min) : "Waiting on NTP sync.";
}

void Time::apply_timezone(int32_t bias_minutes) {
    char tz_env[32];
    snprintf(tz_env, sizeof(tz_env), "GMT%c%d:%02d",
             bias_minutes >= 0 ? '-' : '+', std::abs(bias_minutes) / 60, std::abs(bias_minutes) % 60);
    setenv("TZ", tz_env, 1);
    tzset();
    active_timezone_bias_min = bias_minutes;
}

std::optional<Time::CurrentTimeInfo> Time::get_current_time() const {
    const time_t now = time(nullptr);
    tm tm_now{};

    if (!localtime_r(&now, &tm_now) || (tm_now.tm_year + 1900) < 2024) {
        return std::nullopt;
    }

    CurrentTimeInfo out;
    out.day = (tm_now.tm_wday + 6) % 7; // 0 = Monday
    out.minute_of_day = (tm_now.tm_hour * 60) + tm_now.tm_min;
    out.daystamp = ((tm_now.tm_year + 1900) * 1000) + tm_now.tm_yday;
    out.year = tm_now.tm_year + 1900;
    out.month = tm_now.tm_mon + 1;
    out.day_of_month = tm_now.tm_mday;
    out.hour = tm_now.tm_hour;
    out.minute = tm_now.tm_min;
    out.second = tm_now.tm_sec;

    return out;
}

void Time::cli_timezone(std::span<const std::string> args) {
    int32_t bias = 0;
    if (xewe::str::parse_gmt_offset(args[0], bias)) {
        apply_timezone(bias);
        controller.nvs.write<std::string>(id, "tz", args[0]);
        controller.nvs.write<std::string>(id, "tz_min", std::to_string(bias));
        timezone_ready = true;
        controller.serial_port.print("Timezone updated.");
    }
}

void Time::print_current_time() {
    std::optional<Time::CurrentTimeInfo> current_time = get_current_time();
    if (current_time.has_value()) {
        char time_str[64];
        snprintf(time_str, sizeof(time_str), "Current time: %04d-%02d-%02d %02d:%02d:%02d",
                 current_time->year, current_time->month, current_time->day_of_month,
                 current_time->hour, current_time->minute, current_time->second);
        controller.serial_port.print(time_str);
    } else {
        controller.serial_port.print("Time Error: Failed to parse current time.");
    }
}