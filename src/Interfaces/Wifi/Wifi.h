#ifndef WIFI_H
#define WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include <vector>

class Wifi {
public:
    explicit Wifi                                   (const char* hostname = "ESP32-C3-Device");

    std::vector<String>     get_available_networks  ();
    bool                    connect                 (const String& ssid, const String& password);
    bool                    disconnect              ();
    bool                    is_connected            () const;
    String                  get_local_ip            () const;
    String                  get_ssid                () const;
    String                  get_mac_address         () const;
};

#endif // WIFI_H
