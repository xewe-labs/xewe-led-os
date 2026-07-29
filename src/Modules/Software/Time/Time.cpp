#include "Time.h"
#include "../../Module/ModuleController.h"
#include <cmath>


Time::Time(ModuleController& controller)
    : Module(controller, "time", "Time", "Handles NTP and Timezone", true, true, true)
{
    commands_storage.push_back(Command{
        "set_zone",
        "Set timezone offset (e.g. GMT-08:00)",
        "$time set_zone GMT-0800",
        1,
        [this](std::span<const std::string> args){ cli_set_timezone(args); }
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
                            int16_t total_mins = (hours * 60) + mins;

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
    controller.serial_port.print("Initializing Time & Detecting Timezone...\n");

    esp_netif_sntp_deinit();

    esp_sntp_config_t sntp_cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(
        3, ESP_SNTP_SERVER_LIST("pool.ntp.org", "time.google.com", "time.cloudflare.com")
    );
    esp_netif_sntp_init(&sntp_cfg);

    const char* TZ_ENDPOINTS[] = {
        "http://ipwho.is/?fields=success,timezone.offset,timezone.utc",
        "http://ipwhois.app/json/",
        "http://api.ip2location.io/"
    };

    const char* TZ_SEARCH_KEYS[] = {
        "\"utc\"",
        "\"timezone_gmt\"",
        "\"time_zone\""
    };

    QueueHandle_t tz_queue = xQueueCreate(3, sizeof(int16_t));
    for (int i = 0; i < 3; i++) {
        TzTaskParam* param = new TzTaskParam{TZ_ENDPOINTS[i], TZ_SEARCH_KEYS[i], tz_queue};
        xTaskCreate(http_tz_task, "http_tz_task", 4096, param, 5, NULL);
    }

    // 3. Wait up to 6s for the fastest HTTP response
    int16_t detected_tz_min = 0;
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
    controller.serial_port.print();

    vQueueDelete(tz_queue);

    // 5. Prompt user if auto-detection and NTP succeeded
    if (tz_found && sync_err == ESP_OK) {
        apply_timezone(detected_tz_min);
        tm ct = get_current_time();
        char gmt_str[16];
        snprintf(gmt_str, sizeof(gmt_str), "GMT%c%02d%02d",
                 detected_tz_min >= 0 ? '+' : '-', std::abs(detected_tz_min) / 60, std::abs(detected_tz_min) % 60);

        char prompt[128];
        snprintf(prompt, sizeof(prompt), "Is your time: %04d-%02d-%02d %02d:%02d:%02d (%s)?",
                 ct.tm_year + 1900, ct.tm_mon + 1, ct.tm_mday, ct.tm_hour, ct.tm_min, ct.tm_sec, gmt_str);

        if (controller.serial_port.get_yn(prompt)) {
            controller.nvs.write<std::string>(id, "tz_gmt_str", xewe::str::upper(gmt_str));
            controller.nvs.write<int16_t>(id, "tz_min_bias", detected_tz_min);
            controller.serial_port.print("Timezone set");
            return;
        }
    }

    while (true) {
        int16_t parsed_bias = 0;
        std::string tz_input = controller.serial_port.get_string("Enter your timezone offset (e.g. GMT-0800)\nFor support visit:\nhttps://webbrowsertools.com/timezone/");

        if (xewe::str::parse_gmt_offset(tz_input, parsed_bias)) {
            apply_timezone(parsed_bias);
            controller.nvs.write<std::string>(id, "tz_gmt_str", xewe::str::upper(tz_input));
            controller.nvs.write<int16_t>(id, "tz_min_bias", parsed_bias);
            controller.serial_port.printf("Timezone set to %s\n", xewe::str::upper(tz_input));
            return;
        }
    }
}

void Time::begin_routines_regular(const ModuleConfig& cfg) {
    apply_timezone(controller.nvs.read<int16_t>(id, "tz_min_bias"));
    get_time_from_web(true);
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

    print_current_time();

    return (sync_err == ESP_OK);
}

void Time::reset(bool verbose, bool do_restart, bool keep_enabled) {
    Module::reset(verbose, do_restart, keep_enabled);
}

std::string Time::status(bool verbose) const {
    if (is_disabled()) return Module::status();
    std::string_view current_time_str = get_current_time_str();
    if (verbose) controller.serial_port.print(current_time_str);
    return current_time_str;
}

void Time::apply_timezone(int16_t bias_minutes) {
    active_timezone_bias_min = bias_minutes;
}

tm Time::get_current_time() const {
    const time_t now = time(nullptr);
    tm tm_now{};
    localtime_r(&now, &tm_now);
    return tm_now;
}

str::string_view Time::get_current_time_str() const {
    tm current_time = get_current_time();
    char time_str[64];
    snprintf(time_str, sizeof(time_str), "Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
             current_time.tm_year + 1900, current_time.tm_mon + 1, current_time.tm_mday,
             current_time.tm_hour, current_time.tm_min, current_time.tm_sec);
    return std::string_view(time_str);
}

void Time::cli_set_timezone(std::span<const std::string> args) {
    int16_t bias = 0;
    if (xewe::str::parse_gmt_offset(args[0], bias)) {
        apply_timezone(bias);
        controller.nvs.write<std::string>(id, "tz_gmt_str", args[0]);
        controller.nvs.write<int16_t>(id, "tz_min_bias", bias);
        controller.serial_port.print("Timezone updated.");
    }
}

void Time::print_current_time() {
    controller.serial_port.print(get_current_time_str());
}