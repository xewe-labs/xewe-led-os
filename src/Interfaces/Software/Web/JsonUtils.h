#pragma once
#include <string>
#include <array>
#include <cstdint>
#include <cctype>

namespace xewe::json {

// ---- small helpers ----
inline uint8_t clamp_u8(int v){ return (v<0)?0: (v>255)?255: uint8_t(v); }

inline std::string quote(const std::string& s){ return std::string("\"") + s + "\""; }

// ---- extractors (very small JSON) ----
inline bool extract_number(const std::string& body, const std::string& key, int& out) {
    const std::string pat = "\"" + key + "\"";
    size_t k = body.find(pat); if (k == std::string::npos) return false;
    size_t colon = body.find(':', k + pat.size()); if (colon == std::string::npos) return false;
    size_t i = colon + 1; while (i<body.size() && std::isspace((unsigned char)body[i])) ++i;
    bool neg=false; if (i<body.size() && (body[i]=='-'||body[i]=='+')){ neg=(body[i]=='-'); ++i; }
    long v=0; bool any=false;
    while (i<body.size() && std::isdigit((unsigned char)body[i])) { any=true; v=v*10+(body[i]-'0'); ++i; }
    if (!any) return false;
    out = neg ? -int(v) : int(v); return true;
}

inline bool extract_bool(const std::string& body, const std::string& key, bool& out) {
    const std::string pat = "\"" + key + "\"";
    size_t k = body.find(pat); if (k == std::string::npos) return false;
    size_t colon = body.find(':', k + pat.size()); if (colon == std::string::npos) return false;
    size_t i = colon + 1; while (i<body.size() && std::isspace((unsigned char)body[i])) ++i;
    if (body.compare(i,4,"true")==0)  { out=true;  return true; }
    if (body.compare(i,5,"false")==0) { out=false; return true; }
    return false;
}

inline bool extract_string(const std::string& body, const std::string& key, std::string& out) {
    const std::string pat = "\"" + key + "\"";
    size_t k = body.find(pat); if (k == std::string::npos) return false;
    size_t colon = body.find(':', k + pat.size()); if (colon == std::string::npos) return false;
    size_t i = colon + 1; while (i<body.size() && std::isspace((unsigned char)body[i])) ++i;
    if (i>=body.size() || body[i] != '\"') return false; ++i;
    std::string val; while (i<body.size() && body[i] != '\"') val.push_back(body[i++]);
    if (i>=body.size()) return false; out = val; return true;
}

inline bool extract_rgb_array(const std::string& body, const std::string& key, std::array<uint8_t,3>& out) {
    const std::string pat = "\"" + key + "\"";
    size_t k = body.find(pat); if (k == std::string::npos) return false;
    size_t colon = body.find(':', k + pat.size()); if (colon == std::string::npos) return false;
    size_t i = colon + 1; while (i<body.size() && std::isspace((unsigned char)body[i])) ++i;
    if (i>=body.size() || body[i] != '[') return false; ++i;

    int vals[3]={0,0,0}, n=0;
    while (i<body.size() && n<3) {
        while (i<body.size() && std::isspace((unsigned char)body[i])) ++i;
        bool any=false; long v=0; bool neg=false;
        if (i<body.size() && (body[i]=='-'||body[i]=='+')) { neg=(body[i]=='-'); ++i; }
        while (i<body.size() && std::isdigit((unsigned char)body[i])) { any=true; v=v*10+(body[i]-'0'); ++i; }
        if (!any) return false;
        vals[n++] = neg ? -int(v) : int(v);
        while (i<body.size() && std::isspace((unsigned char)body[i])) ++i;
        if (i<body.size() && body[i]==',') { ++i; continue; }
        if (i<body.size() && body[i]==']') { ++i; break; }
    }
    if (n!=3) return false;
    out = { clamp_u8(vals[0]), clamp_u8(vals[1]), clamp_u8(vals[2]) };
    return true;
}

// ---- emitters (canonical payload; client converts) ----
inline const char* mode_to_string(uint8_t id){ (void)id; return "solid"; }

inline std::string make_color_patch_json(const std::array<uint8_t,3>& rgb) {
    return std::string("{\"rgb\":[")
        + std::to_string(rgb[0]) + "," + std::to_string(rgb[1]) + "," + std::to_string(rgb[2]) + "]}";
}
inline std::string make_brightness_patch_json(uint8_t b255) {
    return std::string("{\"brightness\":") + std::to_string(b255) + "}";
}
inline std::string make_power_patch_json(bool pwr) {
    return std::string("{\"power\":") + (pwr ? "true" : "false") + "}";
}
inline std::string make_mode_patch_json(uint8_t id) {
    return std::string("{\"mode\":") + quote(mode_to_string(id)) + "}";
}
inline std::string make_full_state_json(const std::array<uint8_t,3>& rgb,
                                        uint8_t brightness_255,
                                        bool power,
                                        uint8_t mode_id,
                                        const std::string* name = nullptr,
                                        const bool* online = nullptr) {
    std::string js = std::string("{\"rgb\":[")
        + std::to_string(rgb[0]) + "," + std::to_string(rgb[1]) + "," + std::to_string(rgb[2]) + "],"
        + "\"brightness\":" + std::to_string(brightness_255) + ","
        + "\"power\":" + (power ? "true" : "false") + ","
        + "\"mode\":" + quote(mode_to_string(mode_id));
    if (name)   js += std::string(",\"name\":")   + quote(*name);
    if (online) js += std::string(",\"online\":") + (*online ? "true":"false");
    js += "}";
    return js;
}

} // namespace xewe::json
