#include "Time.h"
#include "../../Module/ModuleController.h"
#include <cmath>

static const char* TZ_ENDPOINTS[] = {
    "http://ipwho.is/?fields=success,timezone.offset,timezone.utc",
    "http://ipwhois.app/json/",
    "http://api.ip2location.io/"
};

// Flexible search keys to match whitespace variations in JSON
static const char* TZ_SEARCH_KEYS[] = {
    "\"utc\"",          // For ipwho.is
    "\"timezone_gmt\"", // For ipwhois.app
    "\"time_zone\""     // For api.ip2location.io
};

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

void Time::http_tz_task(void* pvParameters) {
    TzTaskParam* param = static_cast<TzTaskParam*>(pvParameters);

    esp_http_client_config_t config = {};
    config.url = param->url;
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 5000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client) {
        esp_http_client_set_header(client, "User-Agent", "ESP32-Time-Module/1.0");
        esp_http_client_set_header(client, "Accept", "application/json");

        if (esp_http_client_open(client, 0) == ESP_OK) {
            esp_http_client_fetch_headers(client);

            char buffer[1024] = {0};
            int total_read = 0;
            int read_len = 0;

            while ((read_len = esp_http_client_read(client, buffer + total_read, sizeof(buffer) - total_read - 1)) > 0) {
                total_read += read_len;
                if (total_read >= sizeof(buffer) - 1) break;
            }

            if (total_read > 0) {
                buffer[total_read] = '\0';
                std::string resp(buffer);

                size_t pos = resp.find(param->search_key);
                if (pos != std::string::npos) {
                    size_t start_quote = resp.find('"', pos + std::strlen(param->search_key));
                    if (start_quote != std::string::npos && start_quote + 7 <= resp.length()) {
                        std::string offset_str = resp.substr(start_quote + 1, 6);

                        try {
                            int hours = std::stoi(offset_str.substr(1, 2));
                            int mins = std::stoi(offset_str.substr(4, 2));
                            int32_t total_mins = (hours * 60) + mins;

                            if (offset_str[0] == '-') {
                                total_mins = -total_mins;
                            }

                            if (param->result_queue != NULL) {
                                xQueueSend(param->result_queue, &total_mins, 0);
                            }
                        } catch (...) {}
                    }
                }
            }
        }
        esp_http_client_cleanup(client);
    }

    delete param;
    vTaskDelete(NULL);
}

void Time::begin_routines_init(const ModuleConfig& cfg) {
    controller.serial_port.print("Initializing Time & Auto-detecting Timezone...\n");

    // Clean up SNTP to prevent ESP_ERR_INVALID_STATE (0x103)
    esp_netif_sntp_deinit();

    // 1. Configure and start SNTP cleanly using IDF default macro helper
    esp_sntp_config_t sntp_cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(
        3, ESP_SNTP_SERVER_LIST("pool.ntp.org", "time.google.com", "time.cloudflare.com")
    );
    esp_netif_sntp_init(&sntp_cfg);

    // 2. Dispatch HTTP timezone tasks in parallel
    QueueHandle_t tz_queue = xQueueCreate(3, sizeof(int32_t));
    for (int i = 0; i < 3; i++) {
        TzTaskParam* param = new TzTaskParam{TZ_ENDPOINTS[i], TZ_SEARCH_KEYS[i], tz_queue};
        xTaskCreate(http_tz_task, "http_tz_task", 4096, param, 5, NULL);
    }

    // 3. Wait up to 6s for the fastest HTTP response
    int32_t detected_tz_min = 0;
    bool tz_found = (xQueueReceive(tz_queue, &detected_tz_min, pdMS_TO_TICKS(6000)) == pdTRUE);

    // 4. Wait for NTP sync
    controller.serial_port.print("Waiting for NTP sync", "");
    int retries = 0;
    esp_err_t sync_err = ESP_FAIL;
    while ((sync_err = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(200))) != ESP_OK && retries < 50) {
        controller.serial_port.print(".", "");
        vTaskDelay(pdMS_TO_TICKS(200));
        retries++;
    }
    controller.serial_port.print("\n");
    time_ready = (sync_err == ESP_OK);

    // 5. Prompt user if auto-detection and NTP succeeded
    bool user_accepted = false;
    if (tz_found && time_ready) {
        apply_timezone(detected_tz_min);
        std::optional<CurrentTimeInfo> ct = get_current_time();

        if (ct.has_value()) {
            char gmt_str[16];
            snprintf(gmt_str, sizeof(gmt_str), "GMT%c%02d%02d",
                     detected_tz_min >= 0 ? '+' : '-', std::abs(detected_tz_min) / 60, std::abs(detected_tz_min) % 60);

            char prompt[128];
            snprintf(prompt, sizeof(prompt), "Is your time: %04d-%02d-%02d %02d:%02d:%02d (%s)? (y/n)\n",
                     ct->year, ct->month, ct->day_of_month, ct->hour, ct->minute, ct->second, gmt_str);

            std::string ans = controller.serial_port.get_string(prompt, 1, 1, 0, 0, "");

            if (ans == "y" || ans == "Y") {
                controller.nvs.write<std::string>(id, "tz", std::string(gmt_str));
                controller.nvs.write<std::string>(id, "tz_min", std::to_string(detected_tz_min));
                timezone_ready = true;
                user_accepted = true;
                controller.serial_port.print("Timezone saved.\n");
            }
        }
    }

    // Delay queue cleanup to ensure lingering tasks finish writing safely
    vTaskDelay(pdMS_TO_TICKS(1000));
    vQueueDelete(tz_queue);

    // 6. Fallback to manual entry if detection failed or user rejected
    if (!user_accepted) {
        bool tz_valid = false;
        int32_t parsed_bias = 0;
        std::string tz_input;

        while (!tz_valid) {
            tz_input = controller.serial_port.get_string("Enter your timezone offset (e.g. GMT-0800)\nFor support visit:\nhttps://webbrowsertools.com/timezone/", 4, 15, 0, 0, "GMT+0000");
            if (xewe::str::parse_gmt_offset(tz_input, parsed_bias)) {
                tz_valid = true;
                apply_timezone(parsed_bias);
                controller.nvs.write<std::string>(id, "tz", xewe::str::upper(tz_input));
                controller.nvs.write<std::string>(id, "tz_min", std::to_string(parsed_bias));
                timezone_ready = true;
                controller.serial_port.printf("Timezone set to %s\n", xewe::str::upper(tz_input).c_str());
            }
        }
    }
}

void Time::begin_routines_regular(const ModuleConfig& cfg) {
    std::string tz_str = controller.nvs.read<std::string>(id, "tz_min");
    if (!tz_str.empty()) {
        apply_timezone(std::stoi(tz_str));
        timezone_ready = true;
    }

    get_time_from_web();
}

bool Time::get_time_from_web(bool verbose) {
    esp_netif_sntp_deinit();

    esp_sntp_config_t sntp_cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(
        3, ESP_SNTP_SERVER_LIST("pool.ntp.org", "time.google.com", "time.cloudflare.com")
    );
    esp_netif_sntp_init(&sntp_cfg);

    if (verbose) controller.serial_port.print("Syncing time from server", "");
    int retries = 0;
    esp_err_t sync_err = ESP_FAIL;
    while ((sync_err = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(200))) != ESP_OK && retries < 50) {
        if (verbose) controller.serial_port.print(".", "");
        vTaskDelay(pdMS_TO_TICKS(200));
        retries++;
    }
    if (verbose) controller.serial_port.print("\n");

    print_current_time();

    time_ready = (sync_err == ESP_OK);
    return time_ready;
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
        snprintf(time_str, sizeof(time_str), "Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
                 current_time->year, current_time->month, current_time->day_of_month,
                 current_time->hour, current_time->minute, current_time->second);
        controller.serial_port.print(time_str);
    } else {
        controller.serial_port.print("Time Error: Failed to parse current time.\n");
    }
}