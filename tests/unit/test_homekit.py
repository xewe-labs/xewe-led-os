# SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
# SPDX-License-Identifier: GPL-3.0-only
# xewe-led-os/tests/unit/test_homekit.py
"""HomeKit unit tests: no board, no firmware build. Run from the project:
`build/tools/.venv/bin/python -m xewe test --unit-only`.

The value mapping is run on the host (host_homekit_map.cpp). HomeKit.cpp itself is compiled on the
host against recording stubs and every function is run (the scenes at the end). Source checks pin
the xewe-led-os 2.3.0 behaviour of the module (HomeSpan configuration, printed texts, the pairing
wait, reset and status) as literals.
"""

import re
import shutil
import subprocess
from pathlib import Path

import pytest

ID = "homekit"
NAME = "HomeKit"
PROJECT = Path(__file__).resolve().parents[2]
SRC = PROJECT / "src" / "HomeKit"
CPP = SRC / "HomeKit.cpp"
HDR = SRC / "HomeKit.h"
MAP = SRC / "HomeKitMap.h"

QR_PATH = "static/media/HomeKit_Connect_QR.png"
QR_URL = f"https://github.com/xewe-labs/xewe-led-os/raw/main/{QR_PATH}"


def _code(path: Path) -> str:
    """The file without // comments."""
    return re.sub(r"//[^\n]*", "", path.read_text())


def _body(name: str) -> str:
    """The body of `HomeKit::<name>(...)` in HomeKit.cpp (comments removed)."""
    m = re.search(
        rf"\bHomeKit::{re.escape(name)}\([^)]*\)[^{{;]*\{{(.*?)\n\}}",
        _code(CPP),
        re.DOTALL,
    )
    assert m, name
    return m[1]


def _squeeze(text: str) -> str:
    """The text with every run of whitespace replaced by one space, trimmed."""
    return " ".join(text.split())


def _in_order(text: str, *parts: str) -> None:
    at = -1
    for part in parts:
        found = text.find(part, at + 1)
        assert found > at, f"{part!r} missing or out of order"
        at = found


@pytest.mark.unit
def test_value_mapping_gpp(run_host):
    # every byte both ways, whole inputs, the float half-way inputs, clamps, round trips
    out = run_host("host_homekit_map")
    assert re.search(r"^1726 checks, 0 failures$", out, re.MULTILINE), out


@pytest.mark.unit
def test_map_header_float_formulas():
    text = MAP.read_text()
    code = _code(MAP)
    assert set(re.findall(r"#include <([^>]+)>", text)) <= {"cmath", "cstdint"}
    assert "double" not in code and "lround" not in code
    for expr in (
        "std::round((hue8 / 255.0f) * 360.0f)",
        "std::round((sat8 / 255.0f) * 100.0f)",
        "std::round((bri8 / 255.0f) * 100.0f)",
        "static_cast<uint8_t>(std::round((deg / 360.0f) * 255.0f))",
        "static_cast<uint8_t>(std::round((pct / 100.0f) * 255.0f))",
    ):
        assert expr in code, expr
    # no clamp of brightness 0 to HomeKit's minimum 1
    bri = re.search(r"int bri_to_pct\(uint8_t bri8\) \{(.*?)\n\}", code, re.DOTALL)[1]
    assert (
        _squeeze(bri)
        == "return static_cast<int>(std::round((bri8 / 255.0f) * 100.0f));"
    )


@pytest.mark.unit
def test_identity():
    code = _code(CPP)
    assert re.search(r'/\* id\s+\*/ "homekit"', CPP.read_text())
    assert re.search(r'/\* name\s+\*/ "HomeKit"', CPP.read_text())
    assert (
        '"Allows to control the LED via Apple Home App.\\nREQUIRES Apple Hub '
        '(Speaker/Apple TV)"' in code
    )
    for flag in ("requires_init_setup", "can_be_disabled", "has_cli_cmds"):
        assert re.search(rf"/\* {flag}\s+\*/ true", CPP.read_text()), flag
    assert "HomeKit::HomeKit(xewe::Os& host, Wifi& wifi, Led& led)" in code
    assert "add_requirement(wifi);" in code
    assert re.search(
        r"class HomeKit : public xewe::Module, public LedListener", HDR.read_text()
    )


@pytest.mark.unit
def test_no_settings_or_own_nvs_keys():
    # 2.3.0 stored nothing of its own (HomeSpan keeps pairings and values in its own NVS); the
    # only NVS access is the pairing-list erase when HomeSpan never started on this boot
    code = _code(CPP) + _code(HDR)
    assert re.findall(r"os\.nvs\.\w+\([^)]*\)", code) == [
        'os.nvs.remove("HAP", "CONTROLLERS")'
    ]
    assert "settings()" not in code
    assert not re.search(r"register_command\(", code)


@pytest.mark.unit
def test_declared_in_sketch():
    ino = (PROJECT / "xewe-led-os.ino").read_text()
    decl = re.search(r"^HomeKit homekit\(os, wifi, led\);$", ino, re.MULTILINE)
    if decl is None:
        pytest.skip("HomeKit is not declared in xewe-led-os.ino yet")
    assert re.search(r'^#include "src/HomeKit/HomeKit.h"$', ino, re.MULTILINE)
    assert decl.start() > ino.index('#include "src/Modules.h"')
    # Home Assistant first (its MDNS.end() must not stop HomeSpan's responder), Alexa after
    ha = re.search(r"^HomeAssistant homeassistant\(", ino, re.MULTILINE)
    alexa = re.search(r"^Alexa alexa\(", ino, re.MULTILINE)
    assert ha is None or ha.start() < decl.start()
    assert alexa is None or alexa.start() > decl.start()


@pytest.mark.unit
def test_homespan_setup():
    body = _body("begin_routines_required")
    _in_order(
        body,
        "instance = this;",
        "homeSpan.setPortNum(1201);",
        "homeSpan.setSerialInputDisable(true);",
        "homeSpan.setLogLevel(-1);",
        "homeSpan.begin(Category::Lighting, device_name.c_str());",
        "SPAN_ACCESSORY();",
        "SPAN_ACCESSORY(device_name.c_str());",
        "device = new NeoPixel_RGB(this);",
        "led.listeners.add(this)",
    )
    assert "os.system.get_device_name()" in body
    code = _code(CPP) + _code(HDR)
    # HomeSpan's defaults are the setup code 466-37-726 and setup ID HSPN the QR encodes
    for absent in ("setPairingCode", "setQRID", "setHostNameSuffix", "autoPoll"):
        assert absent not in code, absent
    hdr = _code(HDR)
    assert "struct NeoPixel_RGB : Service::LightBulb" in hdr
    for chr_ in (
        "Characteristic::On power{0, true};",
        "Characteristic::Hue H{0, true};",
        "Characteristic::Saturation S{0, true};",
        "Characteristic::Brightness V{100, true};",
    ):
        assert chr_ in hdr, chr_
    assert "V.setRange(1, 100, 1);" in _code(CPP)
    assert _squeeze(_body("loop")) == "if (is_disabled()) return; homeSpan.poll();"


@pytest.mark.unit
def test_pairing_text_and_wait():
    body = _body("begin_routines_init")
    hdr = HDR.read_text()
    url = "".join(
        re.findall(
            r'"([^"]*)"', hdr.split("#define HOMEKIT_QR_URL", 1)[1].split("#endif")[0]
        )
    )
    assert url == QR_URL
    # the link points at the image this repository holds
    assert (PROJECT / QR_PATH).is_file()
    _in_order(
        body,
        "homeSpan.setStatusCallback(&HomeKit::status_callback);",
        '"\\nOpen the link with the Setup QR below and scan it\\nwith your iPhone/iPad"',
        "os.serial.print(HOMEKIT_QR_URL);",
        '"If using Mac, go to the Home App and add device\\nusing code 4663-7726"',
        '"\\nThe setup process will continue automatically\\nafter device is pared with '
        'HomeKit"',
        'os.serial.print("TO ABORT PRESS (x): ");',
        "while (hs_status != HS_PAIRED) {",
        "homeSpan.poll();",
        "os.serial.loop();",
        "if (os.serial.has_line()) {",
        "std::string input = os.serial.read_line();",
        "if (input[0] == 'x') {",
        "disable(false, true);",
        "return;",
        'os.serial.print("\\n(x)?: ");',
        'os.serial.print("Setting up HomeKit");',
        "run_with_dots([] { homeSpan.poll(); }, 3000);",
        '"\\nDevice successfully paired with HomeKit.\\nNote, it will stop working with "',
        '"HomeKit App if you dont have a hub"',
    )
    assert "millis()" not in body and "delay(" not in body  # no timeout, no delay
    status_cb = _body("status_callback")
    assert "instance->hs_status = static_cast<uint8_t>(s);" in status_cb


@pytest.mark.unit
def test_update_applies_colour_brightness_state():
    body = re.search(
        r"boolean HomeKit::NeoPixel_RGB::update\(\) \{(.*?)\n\}", _code(CPP), re.DOTALL
    )[1]
    _in_order(
        body,
        "if (!owner)",
        "return false;",
        "const bool state = power.getNewVal();",
        "const float hue_deg = H.getNewVal<float>();",
        "const float sat_pct = S.getNewVal<float>();",
        "const float bri_pct = V.getNewVal<float>();",
        "led_map::deg_to_hue(hue_deg)",
        "led_map::pct_to_byte(sat_pct)",
        "led_map::pct_to_byte(bri_pct)",
        "xewe::color::hsv_to_rgb({h_byte, s_byte, 255})",
        "owner->led.sync_color(rgb, owner);",
        "owner->led.sync_brightness(bri_byte, owner);",
        "owner->led.sync_state(state, owner);",
        "return true;",
    )
    # applied on every write: nothing in between is conditional
    assert body.count("if (") == 1 and "updated()" not in body


@pytest.mark.unit
def test_led_to_homekit():
    for hook in ("on_color", "on_brightness", "on_state"):
        _in_order(
            _body(hook),
            "if (origin == this)",
            "if (is_disabled())",
            "if (!device)",
        )
    _in_order(
        _body("on_color"),
        "xewe::color::rgb_to_hsv(rgb)",
        "led_map::hue_to_deg(hsv[0])",
        "led_map::sat_to_pct(hsv[1])",
        "device->H.setVal(hue_deg);",
        "device->S.setVal(sat_pct);",
    )
    bri = _body("on_brightness")
    assert (
        "led_map::bri_to_pct(brightness)" in bri and "device->V.setVal(bri_pct);" in bri
    )
    assert "<" not in bri.replace("static_cast<float>", "")  # 0 passes through
    assert "device->power.setVal(on);" in _body("on_state")
    assert "setVal" not in _body("on_mode")
    # `$led reset`: state, brightness, colour (2.3.0 sync_all order)
    _in_order(
        _body("on_all"),
        "if (is_disabled())",
        "if (!device)",
        "on_state(on, origin);",
        "on_brightness(brightness, origin);",
        "on_color(rgb, origin);",
    )
    # registered from begin_routines_required only, so a disabled module receives nothing
    assert _code(CPP).count("listeners.add(") == 1
    ctor = re.search(r"HomeKit::HomeKit\(.*?\{(.*?)\n\}", _code(CPP), re.DOTALL)[1]
    assert "listeners" not in ctor


@pytest.mark.unit
def test_reset_erases_pairings_only():
    body = _body("reset")
    _in_order(
        body,
        "if (verbose)",
        'os.serial.print("You also need to remove the device from the Home App manually");',
        "if (device)",
        'homeSpan.processSerialCommand("U");',
        "else",
        'os.nvs.remove("HAP", "CONTROLLERS");',
        "delay(100);",
        "xewe::Module::reset(verbose, do_restart, keep_enabled);",
    )
    assert 'processSerialCommand("F")' not in _code(CPP)


@pytest.mark.unit
def test_status_dump():
    body = _body("status")
    _in_order(
        body,
        "if (is_disabled())",
        'return std::string("HomeKit module disabled");',
        "homeSpan.setLogLevel(2);",
        'homeSpan.processSerialCommand("s");',
        'homeSpan.processSerialCommand("i");',
        "homeSpan.setLogLevel(-1);",
        "return xewe::Module::status(verbose);",
    )
    assert "os.serial" not in body  # nothing printed when disabled


@pytest.mark.unit
def test_headers_and_comments():
    for path in (CPP, HDR, MAP, PROJECT / "tests" / "unit" / "host_homekit_map.cpp"):
        head = path.read_text().splitlines()[:2]
        assert head == [
            "// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)",
            "// SPDX-License-Identifier: GPL-3.0-only",
        ], path
        text = path.read_text()
        for word in ("DECISIONS", "PORT-FIX", "PORT-QUIRK", "led-migration"):
            assert word not in text, (path, word)


# --- host run of HomeKit.cpp ------------------------------------------------------------------
# HomeKit.cpp is compiled unchanged against stubs of HomeSpan, XeWeCore, Led and Wifi that record
# every call (the real core colour functions, LedListener and HomeKitMap.h are used). The driver
# runs every function of the module and prints the recorded calls per scene; the tests compare
# whole scenes, so a statement added, removed, reordered or guarded differently fails.

STUB = r"""
#pragma once
#include <array>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <Led/LedListener.h>
#include <XeWeCore/Utils/Color.h>

std::vector<std::string> trace;
const void* g_self = nullptr;
const LedListener* g_listener = nullptr;

inline void note(const char* fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    std::string out;
    for (const char* c = buf; *c; ++c)
        out += (*c == '\n') ? std::string("\\n") : std::string(1, *c);
    trace.push_back(out);
}
inline const char* who(const void* p) {
    return p == nullptr ? "null" : (p == g_self ? "self" : "other");
}

// Arduino / HomeSpan
typedef bool boolean;
inline void delay(uint32_t ms) { note("delay %u", static_cast<unsigned>(ms)); }
enum HS_STATUS { HS_WIFI_NEEDED, HS_WIFI_CONNECTING, HS_PAIRING_NEEDED, HS_PAIRED };
enum class Category { Lighting, Other };
struct Span {
    void (*status_cb)(HS_STATUS) = nullptr;
    std::deque<int> statuses; // reported by successive polls, -1: none
    int polls = 0;
    void setPortNum(uint16_t port) { note("homeSpan.setPortNum %u", static_cast<unsigned>(port)); }
    void setSerialInputDisable(bool off) { note("homeSpan.setSerialInputDisable %d", off); }
    void setLogLevel(int level) { note("homeSpan.setLogLevel %d", level); }
    void begin(Category c, const char* name) {
        note("homeSpan.begin %s %s", c == Category::Lighting ? "Lighting" : "other", name);
    }
    void setStatusCallback(void (*cb)(HS_STATUS)) {
        status_cb = cb;
        note("homeSpan.setStatusCallback");
    }
    void poll() {
        note("homeSpan.poll");
        if (++polls > 1000) {
            std::printf("STUCK in a poll loop\n");
            std::exit(2);
        }
        if (!statuses.empty()) {
            const int s = statuses.front();
            statuses.pop_front();
            if (s >= 0 && status_cb)
                status_cb(static_cast<HS_STATUS>(s));
        }
    }
    void processSerialCommand(const char* c) { note("homeSpan.processSerialCommand %s", c); }
};
Span homeSpan;
inline void span_accessory(const char* name = nullptr) {
    note("SPAN_ACCESSORY %s", name ? name : "-");
}
#define SPAN_ACCESSORY(...) span_accessory(__VA_ARGS__)

struct StubChar {
    const char* name;
    double newval = 0;
    StubChar(const char* n, int v, bool nvs) : name(n) { note("%s(%d, %d)", n, v, nvs); }
    template <typename T = int>
    T getNewVal() const {
        return static_cast<T>(newval);
    }
    template <typename T>
    void setVal(T v) {
        note("%s.setVal %g", name, static_cast<double>(v));
    }
    void setRange(int lo, int hi, int step) { note("%s.setRange %d %d %d", name, lo, hi, step); }
};
namespace Characteristic {
struct On : StubChar {
    On(int v, bool nvs) : StubChar("On", v, nvs) {}
};
struct Hue : StubChar {
    Hue(int v, bool nvs) : StubChar("Hue", v, nvs) {}
};
struct Saturation : StubChar {
    Saturation(int v, bool nvs) : StubChar("Saturation", v, nvs) {}
};
struct Brightness : StubChar {
    Brightness(int v, bool nvs) : StubChar("Brightness", v, nvs) {}
};
} // namespace Characteristic
namespace Service {
struct LightBulb {
    LightBulb() { note("Service::LightBulb"); }
    virtual ~LightBulb() = default;
    virtual boolean update() { return true; }
};
} // namespace Service

// XeWeCore
namespace xewe {
struct SerialPort {
    std::deque<std::string> lines;
    void print(std::string_view m = {}) {
        note("print %.*s", static_cast<int>(m.size()), m.data());
    }
    void loop() { note("serial.loop"); }
    bool has_line() const {
        note("serial.has_line %d", !lines.empty());
        return !lines.empty();
    }
    std::string read_line() {
        std::string s = lines.front();
        lines.pop_front();
        note("serial.read_line %s", s.c_str());
        return s;
    }
};
struct Nvs {
    void remove(std::string_view ns, std::string_view key) {
        note("nvs.remove %.*s %.*s", static_cast<int>(ns.size()), ns.data(),
            static_cast<int>(key.size()), key.data());
    }
};
struct System {
    std::string get_device_name() {
        note("system.get_device_name");
        return "Strip";
    }
};
class Os {
public:
    SerialPort serial;
    Nvs nvs;
    System system;
    void report_error(const char* fmt, ...) {
        char buf[256];
        va_list ap;
        va_start(ap, fmt);
        std::vsnprintf(buf, sizeof buf, fmt, ap);
        va_end(ap);
        note("report_error %s", buf);
    }
};
class Module {
public:
    Module(Os& host, std::string id, std::string name_, std::string description,
        bool requires_init_setup, bool can_be_disabled, bool has_cli_commands)
        : os(host), name(name_) {
        note("Module %s %s %s %d %d %d", id.c_str(), name_.c_str(), description.c_str(),
            requires_init_setup, can_be_disabled, has_cli_commands);
    }
    virtual ~Module() = default;
    virtual void begin_routines_required() {}
    virtual void begin_routines_init() {}
    virtual void begin_routines_regular() {}
    virtual void begin_routines_common() {}
    void add_requirement(Module& other) { note("add_requirement %s", other.name.c_str()); }
    virtual void loop() {}
    virtual void disable(const bool verbose = false, const bool do_restart = true) {
        note("Module::disable %d %d", verbose, do_restart);
    }
    virtual void reset(
        const bool verbose = false, const bool do_restart = true, const bool keep_enabled = true) {
        note("Module::reset %d %d %d", verbose, do_restart, keep_enabled);
    }
    virtual std::string status(const bool verbose = false) const {
        note("Module::status %d", verbose);
        return name + " module enabled";
    }
    bool is_disabled(const bool verbose = false) const {
        (void)verbose;
        return !enabled;
    }
    bool enabled = true;

protected:
    Os& os;
    std::string name;
    void run_with_dots(const std::function<void()>& work, uint32_t duration_ms = 1000,
        uint32_t dot_interval_ms = 200) {
        note("run_with_dots %u %u", static_cast<unsigned>(duration_ms),
            static_cast<unsigned>(dot_interval_ms));
        work();
    }
};
} // namespace xewe

// Wifi and Led modules
class Wifi : public xewe::Module {
public:
    explicit Wifi(xewe::Os& host) : xewe::Module(host, "wifi", "WiFi", "", false, true, true) {}
};
struct StubListeners {
    bool full = false;
    bool add(LedListener* l) {
        note("led.listeners.add %s", l == g_listener ? "self" : "other");
        return !full;
    }
};
class Led {
public:
    StubListeners listeners;
    void sync_color(const std::array<uint8_t, 3> rgb, const void* origin) {
        note("led.sync_color %u %u %u %s", rgb[0], rgb[1], rgb[2], who(origin));
    }
    void sync_brightness(const uint8_t brightness, const void* origin) {
        note("led.sync_brightness %u %s", brightness, who(origin));
    }
    void sync_state(const bool state, const void* origin) {
        note("led.sync_state %d %s", state, who(origin));
    }
};
"""

DRIVER = r"""
#include <HomeSpan.h>
// the driver reaches the module's private device pointer
#define private public
#include "HomeKit.cpp"
#undef private

static void scene(const char* name) {
    std::printf("== %s\n", name);
    for (const std::string& line : trace)
        std::printf("%s\n", line.c_str());
    trace.clear();
}

static void all_hooks(HomeKit& hk, const void* origin) {
    hk.on_color({255, 0, 0}, origin);
    hk.on_brightness(10, origin);
    hk.on_state(true, origin);
    hk.on_mode(1, origin);
    hk.on_length(5, origin);
    hk.on_all({0, 0, 255}, 20, false, 2, 30, origin);
}

int main() {
    xewe::Os os;
    Wifi wifi(os);
    Led led;
    trace.clear();

    HomeKit hk(os, wifi, led);
    g_self = &hk;
    g_listener = &hk;
    scene("construct");

    all_hooks(hk, nullptr);
    scene("hooks before begin");
    hk.reset(true, true, true);
    scene("reset before begin");
    hk.reset(false, false, false);
    scene("reset quiet before begin");
    hk.enabled = false;
    hk.loop();
    note("-> %s", hk.status(true).c_str());
    scene("disabled before begin");
    hk.enabled = true;

    hk.begin_routines_required();
    scene("begin required");
    hk.reset(false, true, false);
    scene("reset after begin");
    hk.reset(true, false, true);
    scene("reset verbose after begin");

    hk.on_color({255, 0, 0}, nullptr);
    hk.on_color({0, 0, 255}, &led);
    hk.on_color({10, 20, 30}, &hk);
    scene("on_color");
    hk.on_brightness(0, nullptr);
    hk.on_brightness(1, nullptr);
    hk.on_brightness(128, &led);
    hk.on_brightness(255, &hk);
    scene("on_brightness");
    hk.on_state(false, nullptr);
    hk.on_state(true, &led);
    hk.on_state(false, &hk);
    scene("on_state");
    hk.on_mode(3, nullptr);
    hk.on_length(10, nullptr);
    scene("on_mode on_length");
    hk.on_all({0, 255, 0}, 128, true, 2, 60, nullptr);
    scene("on_all");
    hk.on_all({0, 255, 0}, 128, true, 2, 60, &hk);
    scene("on_all own origin");

    hk.enabled = false;
    all_hooks(hk, nullptr);
    hk.loop();
    note("-> %s", hk.status(false).c_str());
    hk.reset(false, false, false);
    scene("disabled after begin");
    hk.enabled = true;
    note("-> %s", hk.status(false).c_str());
    scene("status");
    hk.loop();
    scene("loop");

    hk.device->power.newval = 1;
    hk.device->H.newval = 195.5294f;
    hk.device->S.newval = 50;
    hk.device->V.newval = 1;
    note("-> %d", hk.device->update());
    hk.device->power.newval = 0;
    hk.device->H.newval = 120;
    hk.device->S.newval = 100;
    hk.device->V.newval = 100;
    note("-> %d", hk.device->update());
    scene("update");
    hk.device->owner = nullptr;
    note("-> %d", hk.device->update());
    scene("update without owner");
    hk.device->owner = &hk;

    os.serial.lines = {"?", "xyz"};
    hk.begin_routines_init();
    scene("pairing wait aborted");
    homeSpan.statuses = {HS_PAIRING_NEEDED, -1, HS_PAIRED, HS_PAIRING_NEEDED};
    os.serial.lines = {""};
    hk.begin_routines_init();
    scene("pairing wait paired");

    HomeKit second(os, wifi, led);
    led.listeners.full = true;
    trace.clear();
    second.begin_routines_required();
    scene("listener list full");
    return 0;
}
"""

MOVE = "print You also need to remove the device from the Home App manually"
BEGIN = [
    "system.get_device_name",
    "homeSpan.setPortNum 1201",
    "homeSpan.setSerialInputDisable 1",
    "homeSpan.setLogLevel -1",
    "homeSpan.begin Lighting Strip",
    "SPAN_ACCESSORY -",
    "SPAN_ACCESSORY Strip",
    "Service::LightBulb",
    "On(0, 1)",
    "Hue(0, 1)",
    "Saturation(0, 1)",
    "Brightness(100, 1)",
    "Brightness.setRange 1 100 1",
    "led.listeners.add self",
]
PAIRING_TEXT = [
    "homeSpan.setStatusCallback",
    "print \\nOpen the link with the Setup QR below and scan it\\nwith your iPhone/iPad",
    f"print {QR_URL}",
    "print If using Mac, go to the Home App and add device\\nusing code 4663-7726",
    "print \\nThe setup process will continue automatically\\nafter device is pared with HomeKit",
    "print TO ABORT PRESS (x): ",
]
POLL = ["homeSpan.poll", "serial.loop"]

EXPECTED = {
    "construct": [
        (
            "Module homekit HomeKit Allows to control the LED via Apple Home App.\\nREQUIRES "
            "Apple Hub (Speaker/Apple TV) 1 1 1"
        ),
        "add_requirement WiFi",
    ],
    # never begun (disabled at boot): no device, the hooks do nothing, and the reset erases the
    # stored pairing list instead of sending 'U' to a HomeSpan that never started
    "hooks before begin": [],
    "reset before begin": [
        MOVE,
        "nvs.remove HAP CONTROLLERS",
        "delay 100",
        "Module::reset 1 1 1",
    ],
    "reset quiet before begin": [
        "nvs.remove HAP CONTROLLERS",
        "delay 100",
        "Module::reset 0 0 0",
    ],
    "disabled before begin": ["-> HomeKit module disabled"],
    "begin required": BEGIN,
    "reset after begin": [
        "homeSpan.processSerialCommand U",
        "delay 100",
        "Module::reset 0 1 0",
    ],
    "reset verbose after begin": [
        MOVE,
        "homeSpan.processSerialCommand U",
        "delay 100",
        "Module::reset 1 0 1",
    ],
    # rgb -> core rgb_to_hsv -> hue_to_deg / sat_to_pct; V ignored; own origin dropped
    "on_color": [
        "Hue.setVal 0",
        "Saturation.setVal 100",
        "Hue.setVal 240",
        "Saturation.setVal 100",
    ],
    # 0 passes through below HomeKit's minimum 1
    "on_brightness": [
        "Brightness.setVal 0",
        "Brightness.setVal 0",
        "Brightness.setVal 50",
    ],
    "on_state": ["On.setVal 0", "On.setVal 1"],
    "on_mode on_length": [],
    "on_all": [
        "On.setVal 1",
        "Brightness.setVal 50",
        "Hue.setVal 120",
        "Saturation.setVal 100",
    ],
    "on_all own origin": [],
    # disabled at run time: HomeSpan did start on this boot, so 'U' is sent
    "disabled after begin": [
        "-> HomeKit module disabled",
        "homeSpan.processSerialCommand U",
        "delay 100",
        "Module::reset 0 0 0",
    ],
    "status": [
        "homeSpan.setLogLevel 2",
        "homeSpan.processSerialCommand s",
        "homeSpan.processSerialCommand i",
        "homeSpan.setLogLevel -1",
        "Module::status 0",
        "-> HomeKit module enabled",
    ],
    "loop": ["homeSpan.poll"],
    # Home app -> led: colour at full value, brightness, state, on every write; float rounding
    # (hue 195.5294 -> 139, V 1 -> 3)
    "update": [
        "led.sync_color 126 220 255 self",
        "led.sync_brightness 3 self",
        "led.sync_state 1 self",
        "-> 1",
        "led.sync_color 0 255 0 self",
        "led.sync_brightness 255 self",
        "led.sync_state 0 self",
        "-> 1",
    ],
    "update without owner": ["-> 0"],
    "pairing wait aborted": [
        *PAIRING_TEXT,
        *POLL,
        "serial.has_line 1",
        "serial.read_line ?",
        "print \\n(x)?: ",
        *POLL,
        "serial.has_line 1",
        "serial.read_line xyz",
        "Module::disable 0 1",
    ],
    "pairing wait paired": [
        *PAIRING_TEXT,
        *POLL,
        "serial.has_line 1",
        "serial.read_line ",
        "print \\n(x)?: ",
        *POLL,
        "serial.has_line 0",
        *POLL,
        "serial.has_line 0",
        "print Setting up HomeKit",
        "run_with_dots 3000 200",
        "homeSpan.poll",
        (
            "print \\nDevice successfully paired with HomeKit.\\nNote, it will stop working "
            "with HomeKit App if you dont have a hub"
        ),
    ],
    "listener list full": [
        *BEGIN[:-1],
        "led.listeners.add other",
        "report_error ! HomeKit: led listener list full, Home app will not follow led changes",
    ],
}


@pytest.fixture(scope="module")
def scenes(tmp_path_factory) -> dict[str, list[str]]:
    gpp = shutil.which("g++")
    if gpp is None:
        pytest.skip("g++ not installed")
    core = PROJECT / "build" / "libraries" / "XeWeCore" / "src"
    modules = PROJECT / "build" / "modules" / "src"
    if (
        not (core / "XeWeCore" / "Utils" / "Color.h").is_file()
        or not (modules / "Led" / "LedListener.h").is_file()
    ):
        pytest.skip("XeWeCore or the led module not installed (run ./setup.sh)")
    work = tmp_path_factory.mktemp("homekit_module")
    (work / "hk_stub.h").write_text(STUB)
    for header in ("HomeSpan.h", "XeWeCore.h"):
        (work / header).write_text('#pragma once\n#include "hk_stub.h"\n')
    for header in ("Led/Led.h", "Wifi/Wifi.h"):
        (work / header).parent.mkdir(exist_ok=True)
        (work / header).write_text('#pragma once\n#include "../hk_stub.h"\n')
    (work / "driver.cpp").write_text(DRIVER)
    exe = work / "driver"
    build = subprocess.run(
        [gpp, "-std=c++17", "-Wall", "-Wextra", "-Werror"]
        + [f"-I{work}", f"-I{SRC}", f"-I{modules}", f"-I{core}"]
        + ["-o", str(exe), str(work / "driver.cpp")],
        capture_output=True,
        text=True,
        check=False,
    )
    assert build.returncode == 0, build.stderr
    run = subprocess.run([str(exe)], capture_output=True, text=True, check=False)
    assert run.returncode == 0, run.stdout + run.stderr
    out: dict[str, list[str]] = {}
    current = None
    for line in run.stdout.splitlines():
        if line.startswith("== "):
            current = line[3:]
            out[current] = []
        else:
            out[current].append(line)
    return out


@pytest.mark.unit
def test_module_scenes_cover_every_function(scenes):
    assert list(scenes) == list(EXPECTED)


@pytest.mark.unit
@pytest.mark.parametrize("name", list(EXPECTED))
def test_module_scene(scenes, name):
    assert scenes[name] == EXPECTED[name]
