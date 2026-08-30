#pragma once

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include <set>
#include <string>
#include <string_view>

#include "../../../Module/SyncModule.h"

struct HomeAssistantConfig : public ModuleConfig {};

class HomeAssistant : public SyncModule {
public:
    explicit                    HomeAssistant               (ModuleController& controller);

    // required implementation (device-originated pushes -> MQTT)
    void                        sync_color                  (std::array<uint8_t,3> color)   override;
    void                        sync_brightness             (uint8_t brightness)            override;
    void                        sync_state                  (bool state)                    override;
    void                        sync_mode                   (uint8_t mode)                  override;
    void                        sync_length                 (uint16_t length)               override;

    // optional implementation
    void                        sync_all                    (std::array<uint8_t,3> color,
                                                             uint8_t brightness,
                                                             bool state,
                                                             uint8_t mode,
                                                             uint16_t length)               override;

    void                        begin_routines_init         (const ModuleConfig& cfg)       override;
    void                        begin_routines_required     (const ModuleConfig& cfg)       override;
    void                        begin_routines_common       (const ModuleConfig& cfg)       override;
    void                        loop                        ()                              override;
    void                        reset                       (const bool verbose=false,
                                                             const bool do_restart=true,
                                                             const bool keep_enabled=true)  override;

    std::string                 status                      (const bool verbose=false)      const override;

    // Params are not covered by the 5 standard sync_ hooks (mirrors
    // WebInterface::sync_param): whatever changes a mode param elsewhere should
    // call this so HA sees the new value.
    void                        sync_param                  (std::string_view key, uint16_t value);

    // Provisioning entry points (driven by the /provision, /deprovision HTTP
    // endpoints registered on the shared WebInterface server).
    void                        set_broker                  (const std::string& host, uint16_t port,
                                                             const std::string& user, const std::string& pass);
    void                        clear_broker                ();

private:
    // ---- connection ---------------------------------------------------------
    void                        connect                     ();
    void                        on_message                  (char* topic, uint8_t* payload, unsigned int length);

    // ---- inbound (HA -> device) --------------------------------------------
    void                        handle_light_command        (const std::string& json);
    void                        handle_provision            ();
    void                        handle_deprovision          ();

    // ---- outbound (device -> HA) -------------------------------------------
    void                        publish_light_discovery     ();
    void                        publish_mode_discovery      ();
    void                        publish_light_state         ();
    void                        publish_mode_state          ();
    void                        reconcile_params            ();  // publish current mode's params, clear the rest
    void                        publish_param_discovery     (JsonObjectConst p);
    void                        publish_param_state         (const std::string& key, uint16_t value);
    void                        clear_param                 (const std::string& key);
    void                        clear_all_retained          ();
    void                        add_device                  (JsonDocument& doc) const;

    // ---- helpers ------------------------------------------------------------
    void                        build_topics                ();
    std::string                 mac_to_hex                  () const;
    std::string                 param_state_topic           (const std::string& key) const;
    std::string                 param_discovery_topic       (const std::string& key) const;
    void                        load_creds                  ();
    void                        save_creds                  () const;

    // status() is const but PubSubClient::connected() is not, so keep the
    // client mutable.
    WiFiClient                  net;
    mutable PubSubClient        mqtt                        {net};

    std::string                 mac_hex;
    std::string                 device_id;                  // "xewe_led_os_<mac>"
    std::string                 base_topic;                 // "xewe_led_os/<device_id>"
    std::string                 avail_topic;
    std::string                 light_cmd_topic, light_state_topic, light_discovery_topic;
    std::string                 mode_cmd_topic, mode_state_topic, mode_discovery_topic;

    std::string                 mqtt_host, mqtt_user, mqtt_pass;
    uint16_t                    mqtt_port                   = 1883;
    bool                        provisioned                 = false;

    std::set<std::string>       published_param_keys;       // to clear on mode change / deprovision
    uint32_t                    last_reconnect_ms           = 0;

    static constexpr const char* DISCOVERY_PREFIX           = "homeassistant";
    static constexpr const char* SW_VERSION                 = "0.4.0";
    static constexpr uint16_t   MQTT_BUFFER_SIZE            = 2048;
    static constexpr uint32_t   RECONNECT_INTERVAL_MS       = 3000;
};
