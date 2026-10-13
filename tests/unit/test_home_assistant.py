# SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
# SPDX-License-Identifier: GPL-3.0-only
# xewe-led-os/tests/unit/test_home_assistant.py
"""Home Assistant module (src/HomeAssistant/) against xewe-led-os 2.3.0
(src/Modules/Software/SmartHome/HomeAssistant/HomeAssistant.cpp at 91bd344).

The 2.3.0 values are pinned here as literals, so the tests need nothing outside the project.
- Topics, unique ids and every discovery / state payload: exact strings from the host driver
  (tests/unit/host_ha_discovery.cpp), key order and escaping included.
- With ArduinoJson and XeWeCore in build/libraries (after ./setup.sh) the driver also
  - compares the string builders with the 2.3.0 ArduinoJson builders over every mode parameter,
    many device names and the light states (every hue, saturation and brightness byte; the whole
    655k-state grid with XEWE_TEST_HA_FULL_GRID=1);
  - runs ha::Bridge (HaBridge.h: everything the module publishes and every MQTT command it
    applies) with a fake MQTT client, led and Os. Each scenario's ordered log of publishes (topic,
    payload, retain flag), subscriptions and led calls is compared with the 2.3.0 sequence.
- The rest of HomeAssistant.cpp (lifecycle, first-boot wait, connect, /provision, NVS, status,
  reset) needs the ESP32 core; every function definition in it is pinned whole, comments and
  layout aside.
The single intended difference from 2.3.0 is the firmware version: the real build version
(BUILD_VERSION) instead of "0.4.0" in the mDNS TXT `fw` and the discovery `sw_version`.
"""

import codecs
import json
import os
import re
from pathlib import Path

import pytest

PROJECT = Path(__file__).resolve().parents[2]
SRC = PROJECT / "src" / "HomeAssistant"
CPP = SRC / "HomeAssistant.cpp"
HDR = SRC / "HomeAssistant.h"
DISC = SRC / "HaDiscovery.h"
BRIDGE = SRC / "HaBridge.h"
LIBS = PROJECT / "build" / "libraries"

MAC = "aabbcc0deef1"
DEV = f"xewe_led_os_{MAC}"
BASE = f"xewe_led_os/{DEV}"
DEVICE = (
    '{"identifiers":["' + DEV + '"],"name":"Desk","manufacturer":"XeWe","model":"LED",'
    '"sw_version":"3.0.0"}'
)


@pytest.fixture(scope="module")
def host(run_host) -> dict:
    out = run_host(
        "host_ha_discovery",
        includes=("build/libraries/ArduinoJson/src", "build/libraries/XeWeCore/src"),
    )
    lines = [line for line in out.split("\n") if "\t" in line]
    return dict(line.split("\t", 1) for line in lines)


@pytest.fixture(scope="module")
def cpp() -> str:
    return CPP.read_text()


def _strip_comments(code: str) -> str:
    """`code` without // and /* */ comments (string and character literals kept)."""
    out = []
    i, n = 0, len(code)
    while i < n:
        c = code[i]
        if c in "\"'":
            j = i + 1
            while code[j] != c:
                j += 2 if code[j] == "\\" else 1
            out.append(code[i : j + 1])
            i = j + 1
        elif code.startswith("//", i):
            j = code.find("\n", i)
            i = n if j < 0 else j
        elif code.startswith("/*", i):
            i = code.index("*/", i) + 2
        else:
            out.append(c)
            i += 1
    return "".join(out)


def _norm(code: str) -> str:
    """Comments removed; outside string and character literals, whitespace dropped next to
    punctuation and every other whitespace run made one space (the layout is clang-format's)."""
    parts = re.split(r"(\"(?:[^\"\\]|\\.)*\"|'(?:[^'\\]|\\.)*')", _strip_comments(code))
    for i in range(0, len(parts), 2):
        parts[i] = re.sub(r" ?([^\w ]) ?", r"\1", re.sub(r"\s+", " ", parts[i]))
    return "".join(parts).strip()


def _definitions(src: str) -> dict[str, str]:
    """Every `HomeAssistant::<name>(...)` definition of `src`, from its first line to the closing
    brace of its body, keyed by name (comments removed)."""
    code = _strip_comments(src)
    found = {}
    for m in re.finditer(r"^(?:[\w:<>]+ )?HomeAssistant::(\w+)\(", code, re.MULTILINE):
        depth = 0
        for j in range(code.index("{", m.end()), len(code)):
            depth += {"{": 1, "}": -1}.get(code[j], 0)
            if depth == 0:
                break
        found[m.group(1)] = code[m.start() : j + 1]
    return found


def _c_text(code: str) -> str:
    """The concatenated C string literals of `code`, unescaped."""
    parts = re.findall(r'"((?:[^"\\]|\\.)*)"', code)
    return codecs.decode("".join(parts), "unicode_escape")


# ---- 2.3.0 payloads ----------------------------------------------------------------------

LIGHT = (
    '{"~":"' + BASE + '","schema":"json","name":null,"unique_id":"' + DEV + '_light",'
    '"command_topic":"~/light/set","state_topic":"~/light/state",'
    '"availability_topic":"~/avail","payload_available":"online",'
    '"payload_not_available":"offline","brightness":true,"color_mode":true,'
    '"supported_color_modes":["hs"],"device":' + DEVICE + "}"
)
MODE_NAMES = [
    "Solid",
    "Color Fade",
    "Color Fade Two Zone",
    "Brightness Fade",
    "Pulse",
    "Rainbow",
    "Christmas Lights",
]


def _mode(names: list[str]) -> str:
    return (
        '{"~":"' + BASE + '","name":"Mode","unique_id":"' + DEV + '_mode",'
        '"command_topic":"~/mode/set","state_topic":"~/mode/state",'
        '"availability_topic":"~/avail","options":['
        + ",".join(json.dumps(n) for n in names)
        + '],"device":'
        + DEVICE
        + "}"
    )


def _number(key: str, name: str, lo, hi, step, config: bool) -> str:
    return (
        '{"~":"' + BASE + '","name":' + name + ',"unique_id":"' + DEV + "_" + key + '",'
        '"command_topic":"~/param/'
        + key
        + '/set","state_topic":"~/param/'
        + key
        + '/state",'
        f'"availability_topic":"~/avail","min":{lo},"max":{hi},"step":{step}'
        + (',"entity_category":"config"' if config else "")
        + ',"device":'
        + DEVICE
        + "}"
    )


def _state(on: str, b: int, h: int, s: int) -> str:
    return (
        f'{{"state":"{on}","brightness":{b},"color_mode":"hs",'
        f'"color":{{"h":{h},"s":{s}}}}}'
    )


# ---- topics, ids, payloads (host driver) ---------------------------------------------------


@pytest.mark.unit
def test_topics_and_ids_match_2_3_0(host):
    assert host["mac"] == MAC
    assert host["mac_dash"] == "aa-bb"  # 2.3.0 drops only ':'
    assert host["mac_empty"] == "" and host["device_id_empty_mac"] == "xewe_led_os_"
    assert host["device_id"] == DEV and host["base"] == BASE
    assert host["avail"] == f"{BASE}/avail"
    assert host["light_cmd"] == f"{BASE}/light/set"
    assert host["light_state"] == f"{BASE}/light/state"
    assert host["mode_cmd"] == f"{BASE}/mode/set"
    assert host["mode_state"] == f"{BASE}/mode/state"
    assert host["light_discovery"] == f"homeassistant/light/{DEV}/config"
    assert host["mode_discovery"] == f"homeassistant/select/{DEV}/config"
    assert host["param_cmd"] == f"{BASE}/param/speed/set"
    assert host["param_state"] == f"{BASE}/param/speed/state"
    assert host["param_sub"] == f"{BASE}/param/+/set"
    assert host["param_discovery"] == f"homeassistant/number/{DEV}/speed/config"
    assert host["mdns_host"] == "xewe-led-os-0deef1"
    assert host["mdns_host_short"] == "xewe-led-os-abc"


@pytest.mark.unit
def test_param_topic_parsing(host):
    assert host["parse_ok"] == "min_bright"
    for case in ("parse_state", "parse_empty", "parse_other", "parse_nested"):
        assert host[case] == "<false>", case


@pytest.mark.unit
def test_device_and_light_discovery_exact(host):
    assert host["device"] == DEVICE
    assert host["light"] == LIGHT


@pytest.mark.unit
def test_mode_discovery_exact(host):
    assert host["mode"] == _mode(MODE_NAMES)
    assert host["mode_empty"] == _mode([])


@pytest.mark.unit
def test_param_discovery_exact(host):
    assert host["param_b"] == _number("hue", '"Hue"', 0, 255, 1, False)
    assert host["param_a"] == _number("speed", '"Speed"', 1, 50, 1, True)
    assert host["param_gap"] == _number(
        "h_gap", '"Color Variance"', 0, 65535, 100, True
    )
    # an entry without display_name/min/max/step copies JSON null, as 2.3.0 did
    assert host["param_null_name"] == _number(
        "x", "null", "null", "null", "null", False
    )


@pytest.mark.unit
def test_string_escaping_is_arduinojson(host):
    # ArduinoJson 7: \" \\ \b \f \n \r \t escaped, NUL -> \u0000; every other control byte, '/'
    # and UTF-8 are written raw
    assert host["escape"] == '"A\\"\\\\\\b\\f\\n\\r\\t\x01\x1f/é\\u0000z"'
    assert host["device_ctrl"] == (
        '{"identifiers":["' + DEV + '"],"name":"Desk \\"LED\\"\\n\\b\\f\x01",'
        '"manufacturer":"XeWe","model":"LED","sw_version":"3.0.0"}'
    )


@pytest.mark.unit
def test_light_state_exact(host):
    assert host["state_on"] == _state("ON", 128, 120, 100)
    assert host["state_off"] == _state("OFF", 0, 0, 0)
    assert host["state_max"] == _state("ON", 255, 360, 50)
    assert host["state_mid"] == _state("ON", 50, 181, 0)


@pytest.mark.unit
def test_inbound_colour_conversion(host):
    # lroundf(constrain(x)/range*255): 195.5294 deg -> 139 and 47.2549 % -> 121 as 2.3.0 (a
    # double-precision formula gives 138 / 120)
    assert host["deg_to_hue8"] == "0,85,139,255,255,0,0"
    assert host["pct_to_sat8"] == "0,121,128,255,255,0,0"


def _need_libraries(host: dict, key: str) -> None:
    """Skip when build/libraries is missing (no ./setup.sh yet); fail when it is there but the
    driver did not see it."""
    if host[key] == "absent":
        if (LIBS / "ArduinoJson" / "src").is_dir() and (
            LIBS / "XeWeCore" / "src"
        ).is_dir():
            pytest.fail(
                "build/libraries is present but the host driver did not include it"
            )
        pytest.skip("ArduinoJson / XeWeCore not installed (run ./setup.sh)")


@pytest.mark.unit
def test_builders_equal_2_3_0_arduinojson(host):
    _need_libraries(host, "arduinojson")
    diffs = "\n".join(v for k, v in host.items() if k.startswith("diff"))
    result = host["arduinojson"]
    assert result.startswith("same:"), diffs
    # 2.7k discovery payloads, 8.4M inbound conversion grid points and the light states: 133k
    # (every hue x saturation byte at brightness 128, every brightness at three colours, on and
    # off), or 655k with XEWE_TEST_HA_FULL_GRID=1 (every hue x saturation at five brightnesses)
    full = os.environ.get("XEWE_TEST_HA_FULL_GRID") == "1"
    assert host["grid"] == ("full" if full else "fast"), host["grid"]
    assert int(result[5:]) == (9_098_063 if full else 8_575_311), result


# ---- ha::Bridge on the host (HaBridge.h with a fake MQTT client, led and Os) ---------------
# Each log entry: `pub <R|-> <topic> <payload>` (R = retained), `sub <topic>`, `loop`,
# `print <text>`, `led <call> <args> by=ha` (by=ha: the module passed itself as the origin).


@pytest.fixture(scope="module")
def bridge(host) -> dict:
    _need_libraries(host, "bridge")
    assert host["bridge"] == "ran"
    return {k[7:]: v for k, v in host.items() if k.startswith("bridge_")}


def _log(bridge: dict, scenario: str) -> list[str]:
    return json.loads(bridge[scenario])


def pub(topic: str, payload: str) -> str:
    """A retained publish: every publish of the module is retained (2.3.0)."""
    return f"pub R {topic} {payload}"


def _param(key: str, name: str, lo, hi, step, config: bool, value: int) -> list[str]:
    return [
        pub(
            f"homeassistant/number/{DEV}/{key}/config",
            _number(key, name, lo, hi, step, config),
        ),
        pub(f"{BASE}/param/{key}/state", str(value)),
    ]


def _clear(key: str) -> list[str]:
    return [
        pub(f"homeassistant/number/{DEV}/{key}/config", ""),
        pub(f"{BASE}/param/{key}/state", ""),
    ]


def _light(on: str, b: int, h: int, s: int) -> str:
    return pub(f"{BASE}/light/state", _state(on, b, h, s))


SOLID_PARAMS = [
    *_param("hue", '"Hue"', 0, 255, 1, False, 85),
    *_param("sat", '"Saturation"', 0, 255, 1, False, 255),
]


@pytest.mark.unit
def test_bridge_connect_publishes_2_3_0_sequence(bridge):
    # connect(): avail online, light discovery, mode discovery, light state, mode state, the
    # running mode's number entities, then the three subscriptions; all retained
    assert bridge["device"] == DEVICE
    assert _log(bridge, "announce") == [
        pub(f"{BASE}/avail", "online"),
        pub(f"homeassistant/light/{DEV}/config", LIGHT),
        pub(f"homeassistant/select/{DEV}/config", _mode(MODE_NAMES)),
        _light("ON", 128, 120, 100),
        pub(f"{BASE}/mode/state", "Solid"),
        *SOLID_PARAMS,
        f"sub {BASE}/light/set",
        f"sub {BASE}/mode/set",
        f"sub {BASE}/param/+/set",
    ]


@pytest.mark.unit
def test_bridge_mode_change_swaps_number_entities(bridge):
    # led listener on_mode: mode state, publish the new mode's parameters (value from the modes
    # JSON), clear the previous mode's other keys (sorted), then the light state
    assert _log(bridge, "mode_fade") == [
        pub(f"{BASE}/mode/state", "Color Fade"),
        *_param("hue", '"Hue"', 0, 255, 1, False, 195),
        *_param("sat", '"Min Saturation"', 0, 245, 1, False, 245),
        *_param("speed", '"Speed"', 1, 50, 1, True, 4),
        *_param("fire_step", '"Density"', 1, 255, 1, True, 20),
        *_param("h_gap", '"Color Variance"', 0, 65535, 100, True, 15000),
        *_param("min_bright", '"Depth"', 0, 255, 1, True, 150),
        _light("ON", 128, 120, 100),
    ]
    assert _log(bridge, "mode_rainbow") == [
        pub(f"{BASE}/mode/state", "Rainbow"),
        *_param("speed", '"Speed"', 1, 20, 1, False, 5),
        *_param("density", '"Density"', 1, 30, 1, True, 10),
        *_clear("fire_step"),
        *_clear("h_gap"),
        *_clear("hue"),
        *_clear("min_bright"),
        *_clear("sat"),
        _light("ON", 128, 120, 100),
    ]


@pytest.mark.unit
def test_bridge_light_commands(bridge):
    # order state, brightness, colour; brightness clamped 0-255; "ON" exactly is on; h/s clamped,
    # converted with lroundf, a missing or null one keeps get_hsv(); V = 255; every change echoed
    assert _log(bridge, "light_full") == [
        "led state 1 by=ha",
        _light("ON", 128, 120, 100),
        "led brightness 255 by=ha",
        _light("ON", 255, 120, 100),
        "led color 126,255,126 by=ha",  # hsv_to_rgb(85, 128, 255)
        _light("ON", 255, 120, 100),
    ]
    assert _log(bridge, "light_off_clamp") == [
        "led state 0 by=ha",
        _light("OFF", 255, 120, 100),
        "led brightness 0 by=ha",
        _light("OFF", 0, 120, 100),
    ]
    # get_hsv() = (10, 20, 30) for these three
    assert _log(bridge, "light_keep_hue") == [
        "led color 255,59,0 by=ha",  # hsv_to_rgb(10, 255, 255)
        _light("OFF", 0, 14, 8),
    ]
    assert _log(bridge, "light_keep_both") == [
        "led color 255,239,235 by=ha",  # hsv_to_rgb(10, 20, 255)
        _light("OFF", 0, 14, 8),
    ]
    assert _log(bridge, "light_clamp_colour") == [
        "led color 255,255,255 by=ha",  # h 400 -> 255, s -3 -> 0
        _light("OFF", 0, 14, 8),
    ]
    # wrong types (state not a string, brightness not an int, colour not an object), bad JSON
    assert _log(bridge, "light_ignored") == []


@pytest.mark.unit
def test_bridge_mode_and_param_commands(bridge):
    # a mode NAME, exact and case-sensitive; an unknown one does nothing
    assert _log(bridge, "mode_unknown") == []
    assert _log(bridge, "mode_solid") == [
        "led mode 0 by=ha",
        pub(f"{BASE}/mode/state", "Solid"),
        *SOLID_PARAMS,
        *_clear("density"),
        *_clear("speed"),
        _light("OFF", 0, 120, 100),
    ]
    # strtol base 10, (uint16_t) cast, the led clamps (the fake: to 50), the result published
    assert _log(bridge, "param") == [
        "led param hue 12",
        pub(f"{BASE}/param/hue/state", "12"),
        "led param speed 99",
        pub(f"{BASE}/param/speed/state", "50"),
        "led param speed 65535",  # "-1"
        pub(f"{BASE}/param/speed/state", "50"),
        "led param speed 0",  # "0x10"
        pub(f"{BASE}/param/speed/state", "0"),
        "led param speed 0",  # "abc"
        pub(f"{BASE}/param/speed/state", "0"),
    ]
    # a nested or empty key, another device, a state topic: ignored
    assert _log(bridge, "other_topics") == []


@pytest.mark.unit
def test_bridge_retract_clears_retained_then_offline(bridge):
    # /deprovision and reset while connected: empty retained light, select, every published
    # number entity, then avail offline (retained) and one loop
    tail = [pub(f"{BASE}/avail", "offline"), "loop"]
    entities = [
        pub(f"homeassistant/light/{DEV}/config", ""),
        pub(f"{BASE}/light/state", ""),
        pub(f"homeassistant/select/{DEV}/config", ""),
        pub(f"{BASE}/mode/state", ""),
    ]
    assert _log(bridge, "retract") == [*entities, *_clear("hue"), *_clear("sat"), *tail]
    assert _log(bridge, "retract_again") == [*entities, *tail]


@pytest.mark.unit
def test_bridge_silent_while_disconnected(bridge):
    # every publisher checks connected() first; the led still applies the command
    assert _log(bridge, "disconnected") == ["led brightness 9 by=ha"]


@pytest.mark.unit
def test_bridge_refused_publish_and_bad_modes_json(bridge):
    assert _log(bridge, "publish_refused") == [
        pub(f"homeassistant/light/{DEV}/config", LIGHT),
        "print [HomeAssistant] light discovery FAILED (raise MQTT_BUFFER_SIZE?)",
        pub(f"homeassistant/select/{DEV}/config", _mode(MODE_NAMES)),
    ]
    # unparsable modes JSON: empty options; no parameter change; mode commands ignored
    assert _log(bridge, "bad_modes") == [
        pub(f"homeassistant/select/{DEV}/config", _mode([]))
    ]


@pytest.mark.unit
def test_bridge_copies_modes_json_values(bridge):
    # name/min/max/step copied as compact JSON (missing: null), empty keys skipped, a value over
    # 65535 wraps in the uint16_t cast, a mode without a name gives ""
    assert _log(bridge, "odd_modes") == [
        pub(f"homeassistant/select/{DEV}/config", _mode(["Odd", ""])),
        *_param("k", '{"a":[1,2]}', -1.5, '"x"', "null", True, 4464),
        *_param("n", "null", "null", "null", "null", False, 0),
    ]
    assert bridge["device_renamed"] == DEVICE.replace('"Desk"', '"Kitchen \\"2\\""')


@pytest.mark.unit
def test_every_publish_is_retained():
    # 2.3.0 retained every message (the LWT is a connect argument, also retained); the module
    # publishes only through ha::Bridge
    assert "mqtt.publish(" not in _strip_comments(CPP.read_text())
    calls = re.findall(
        r"mqtt\.publish\(([^;]*)\);", _strip_comments(BRIDGE.read_text())
    )
    assert len(calls) == 14
    for call in calls:
        assert re.sub(r"\s+", " ", call).endswith(", true"), call


# ---- module identity, sketch, constants ----------------------------------------------------


@pytest.mark.unit
def test_identity(cpp):
    head = cpp[
        cpp.index("HomeAssistant::HomeAssistant(") : cpp.index("add_requirement(wifi)")
    ]
    assert '"homeassistant"' in head and '"Home_Assistant"' in head
    assert _c_text(head[head.index("description") :].split("/* requires")[0]) == (
        "Allows to control the LED from Home Assistant over MQTT auto-discovery.\n"
        "REQUIRES an MQTT broker (the Home Assistant Mosquitto add-on works)."
    )
    flags = re.findall(
        r"/\* (requires_init_setup|can_be_disabled|has_cli_cmds)\s*\*/ (\w+)", head
    )
    assert flags == [
        ("requires_init_setup", "true"),
        ("can_be_disabled", "true"),
        ("has_cli_cmds", "true"),
    ]


@pytest.mark.unit
def test_constructor_and_sketch_declaration(project_root):
    hdr = HDR.read_text()
    assert re.search(
        r"HomeAssistant\(xewe::Os& host, Wifi& \w+, WebInterface& \w+, Led& \w+\);", hdr
    )
    assert "class HomeAssistant : public xewe::Module, public LedListener" in hdr
    ino = (project_root / "xewe-led-os.ino").read_text()
    include = re.search(
        r'^#include "src/HomeAssistant/HomeAssistant\.h"$', ino, re.MULTILINE
    )
    decl = re.search(
        r"^HomeAssistant homeassistant\(os, wifi, web_interface, led\);$",
        ino,
        re.MULTILINE,
    )
    assert include and decl, "live sketch include and declaration"
    assert include.start() < decl.start()
    # Home Assistant before HomeKit, so MDNS.end() never runs after HomeSpan started mDNS
    homekit = re.search(r"^HomeKit homekit\(", ino, re.MULTILINE)
    assert homekit and decl.start() < homekit.start()


@pytest.mark.unit
def test_constants(cpp):
    hdr = HDR.read_text()
    assert "MQTT_BUFFER_SIZE = 2048" in hdr
    assert "RECONNECT_INTERVAL_MS = 3000" in hdr
    assert "STATUS_INTERVAL_MS = 3000" in hdr
    assert "DEFAULT_PORT = 1883" in hdr
    assert "MDNS_SERVICE_PORT = 80" in hdr
    assert "ha::Bridge<PubSubClient, Led, xewe::Os> bridge;" in hdr
    # the one approved deviation: the real build version, not 2.3.0's "0.4.0"
    assert "constexpr const char* SW_VERSION = BUILD_VERSION;" in cpp
    assert "#include <XeWeBuildInfo.h>" in cpp
    assert "0.4.0" not in cpp + hdr + BRIDGE.read_text()
    # no settings table: 2.3.0 had only the generic commands and a one-line status
    assert "settings()" not in hdr


# ---- HomeAssistant.cpp, every definition whole ---------------------------------------------

SETUP_TEXT = (
    "\nHome Assistant setup required.\n"
    "\n"
    "Discovery mode is active. Open Home Assistant > Settings > Devices & Services.\n"
    "If the discovered card does not appear, provision manually by POSTing MQTT broker "
    "credentials to:\n"
    "  http://<device-ip>/provision\n"
    "\n"
    "JSON body:\n"
    '  {"host":"<mqtt-host>","port":1883,"user":"<mqtt-user>","pass":"<mqtt-pass>"}\n'
    "\n"
    "Example:\n"
    "  curl -X POST http://<device-ip>/provision \\\n"
    "    -H 'Content-Type: application/json' \\\n"
    '    -d \'{"host":"192.168.1.10","port":1883,"user":"mqtt","pass":"password"}\'\n'
    "\n"
    "Setup continues automatically after MQTT connects and discovery is published.\n"
    "To abort and disable Home Assistant, press (x): "
)

# The 2.3.0 functions as ported (comments dropped; compared without layout, see _norm).
# Identity and requirements; the listener hooks (no own-origin filter; mode: mode state, number
# entities, light state; length: nothing; all: state, brightness, colour, mode); the required /
# init / common routines (listener and routes; the first-boot wait with mDNS only while
# unprovisioned, server.begin() without a port, x aborts, status lines every 3000 ms, three
# MDNS.end() sites; init and common share start_client: buffer, callback, stored broker, and
# connect_now = setServer, timer reset, connect); the 3 s reconnect loop; reset and status (HA
# line first, nothing when disabled); set_broker / clear_broker (clear_broker and reset retract
# through retract_and_disconnect); connect (LWT offline qos 1 retained); /provision (connect
# BEFORE the 200 answer, 400 bad_json, int port wrapped by the cast) and /deprovision (200 first);
# the 2.3.0 NVS keys.
CPP_DEFINITIONS = {
    "HomeAssistant": r"""
HomeAssistant::HomeAssistant(
    xewe::Os& host, Wifi& wifi_ref, WebInterface& web_interface_ref, Led& led_ref)
    : xewe::Module(host,
          "homeassistant",
          "Home_Assistant",
          "Allows to control the LED from Home Assistant over MQTT auto-discovery.\n"
          "REQUIRES an MQTT broker (the Home Assistant Mosquitto add-on works).",
          true,
          true,
          true),
      wifi(wifi_ref), web_interface(web_interface_ref), led(led_ref),
      bridge(mqtt, led_ref, host, this, SW_VERSION) {
    add_requirement(wifi);
    add_requirement(web_interface);
    add_requirement(led);
}
""",
    "on_color": r"""
void HomeAssistant::on_color(std::array<uint8_t, 3> rgb, const void* origin) {
    (void)rgb;
    (void)origin;
    if (is_disabled())
        return;
    bridge.publish_light_state();
}
""",
    "on_brightness": r"""
void HomeAssistant::on_brightness(uint8_t brightness, const void* origin) {
    (void)brightness;
    (void)origin;
    if (is_disabled())
        return;
    bridge.publish_light_state();
}
""",
    "on_state": r"""
void HomeAssistant::on_state(bool on, const void* origin) {
    (void)on;
    (void)origin;
    if (is_disabled())
        return;
    bridge.publish_light_state();
}
""",
    "on_mode": r"""
void HomeAssistant::on_mode(uint8_t mode_id, const void* origin) {
    (void)mode_id;
    (void)origin;
    if (is_disabled())
        return;
    bridge.publish_mode_state();
    bridge.reconcile_params();
    bridge.publish_light_state();
}
""",
    "on_length": r"""
void HomeAssistant::on_length(uint16_t length, const void* origin) {
    (void)length;
    (void)origin;
}
""",
    "on_all": r"""
void HomeAssistant::on_all(std::array<uint8_t, 3> rgb, uint8_t brightness, bool on, uint8_t mode_id,
    uint16_t length, const void* origin) {
    (void)length;
    if (is_disabled())
        return;
    on_state(on, origin);
    on_brightness(brightness, origin);
    on_color(rgb, origin);
    on_mode(mode_id, origin);
}
""",
    "begin_routines_required": r"""
void HomeAssistant::begin_routines_required() {
    build_topics();
    if (!led.listeners.add(this))
        os.serial.print("! Home_Assistant: no free led listener slot; LED changes are not "
                        "published (raise XEWE_MODULE_LED_LISTENERS)");
    WebServer& server = web_interface.get_server();
    server.on("/provision", HTTP_POST, [this] { handle_provision(); });
    server.on("/deprovision", HTTP_POST, [this] { handle_deprovision(); });
}
""",
    "begin_routines_init": r"""
void HomeAssistant::begin_routines_init() {
    if (is_disabled())
        return;
    start_client();
    WebServer& server = web_interface.get_server();
    bool mdns_started = false;
    std::string mdns_host;
    if (!provisioned) {
        mdns_host = ha::mdns_host(mac_hex);
        const std::string device_name = os.system.get_device_name();
        if (MDNS.begin(mdns_host.c_str())) {
            MDNS.addService("_xewe-led-os", "_tcp", MDNS_SERVICE_PORT);
            MDNS.addServiceTxt("_xewe-led-os", "_tcp", "mac", mac_hex.c_str());
            MDNS.addServiceTxt("_xewe-led-os", "_tcp", "fw", SW_VERSION);
            MDNS.addServiceTxt("_xewe-led-os", "_tcp", "name", device_name.c_str());
            mdns_started = true;
        }
    }
    server.begin();
    os.serial.print("\nHome Assistant setup required.\n"
                    "\n"
                    "Discovery mode is active. Open Home Assistant > Settings > Devices & "
                    "Services.\n"
                    "If the discovered card does not appear, provision manually by POSTing MQTT "
                    "broker credentials to:\n"
                    "  http://<device-ip>/provision\n"
                    "\n"
                    "JSON body:\n"
                    "  {\"host\":\"<mqtt-host>\",\"port\":1883,\"user\":\"<mqtt-user>\",\"pass\":"
                    "\"<mqtt-pass>\"}\n"
                    "\n"
                    "Example:\n"
                    "  curl -X POST http://<device-ip>/provision \\\n"
                    "    -H 'Content-Type: application/json' \\\n"
                    "    -d "
                    "'{\"host\":\"192.168.1.10\",\"port\":1883,\"user\":\"mqtt\",\"pass\":"
                    "\"password\"}'\n"
                    "\n"
                    "Setup continues automatically after MQTT connects and discovery is "
                    "published.\n"
                    "To abort and disable Home Assistant, press (x): ");
    if (mdns_started) {
        os.serial.print("\n[HomeAssistant] mDNS discovery active: ");
        os.serial.print(mdns_host.c_str());
        os.serial.print("._xewe-led-os._tcp.local");
    } else if (!provisioned) {
        os.serial.print(
            "\n[HomeAssistant] mDNS discovery FAILED; /provision is still available manually.");
    }
    uint32_t last_status_ms = 0;
    while (!provisioned || !mqtt.connected()) {
        server.handleClient();
        os.serial.loop();
        if (os.serial.has_line()) {
            const std::string input = os.serial.read_line();
            if (!input.empty() && input[0] == 'x') {
                if (mdns_started)
                    MDNS.end();
                disable(false, true);
                return;
            }
            os.serial.print("\n(x)?: ");
        }
        if (provisioned) {
            if (mdns_started) {
                MDNS.end();
                mdns_started = false;
            }
            if (!mqtt.connected()) {
                connect();
            } else {
                mqtt.loop();
            }
        }
        const uint32_t now = millis();
        if (now - last_status_ms >= STATUS_INTERVAL_MS) {
            last_status_ms = now;
            if (!provisioned) {
                os.serial.print("\n[HomeAssistant] waiting for Home Assistant discovery card or "
                                "/provision...");
            } else if (!mqtt.connected()) {
                os.serial.print("\n[HomeAssistant] provisioned; waiting for MQTT connection...");
            }
        }
        delay(5);
    }
    if (mdns_started)
        MDNS.end();
    mqtt.loop();
    os.serial.print("\n[HomeAssistant] paired: MQTT connected, discovery published, state "
                    "retained.\n"
                    "Device should now appear in Home Assistant via MQTT discovery.\n");
}
""",
    "begin_routines_common": r"""
void HomeAssistant::begin_routines_common() {
    if (is_disabled())
        return;
    start_client();
}
""",
    "start_client": r"""
void HomeAssistant::start_client() {
    mqtt.setBufferSize(MQTT_BUFFER_SIZE);
    mqtt.setCallback([this](char* t, uint8_t* p, unsigned int l) { bridge.on_message(t, p, l); });
    load_creds();
    if (provisioned)
        connect_now();
}
""",
    "connect_now": r"""
void HomeAssistant::connect_now() {
    mqtt.setServer(mqtt_host.c_str(), mqtt_port);
    last_reconnect_ms = 0;
    connect();
}
""",
    "retract_and_disconnect": r"""
void HomeAssistant::retract_and_disconnect() {
    if (mqtt.connected()) {
        bridge.retract();
        delay(100);
        mqtt.disconnect();
    }
}
""",
    "loop": r"""
void HomeAssistant::loop() {
    if (is_disabled() || !provisioned)
        return;
    if (wifi.is_disconnected())
        return;
    if (!mqtt.connected())
        connect();
    else
        mqtt.loop();
}
""",
    "reset": r"""
void HomeAssistant::reset(const bool verbose, const bool do_restart, const bool keep_enabled) {
    retract_and_disconnect();
    provisioned = false;
    xewe::Module::reset(verbose, do_restart, keep_enabled);
}
""",
    "status": r"""
std::string HomeAssistant::status(const bool verbose) const {
    if (is_disabled())
        return std::string("Home Assistant module disabled");
    std::string line = "Home Assistant: ";
    if (!provisioned) {
        line += "not provisioned (POST /provision to pair)";
    } else {
        line += "broker ";
        line += mqtt_host;
        line += ':';
        line += std::to_string(mqtt_port);
        line += mqtt.connected() ? " [connected]" : " [disconnected]";
    }
    os.serial.print(line);
    return xewe::Module::status(verbose);
}
""",
    "set_broker": r"""
void HomeAssistant::set_broker(
    const std::string& host, uint16_t port, const std::string& user, const std::string& pass) {
    mqtt_host = host;
    mqtt_port = port;
    mqtt_user = user;
    mqtt_pass = pass;
    provisioned = !mqtt_host.empty();
    save_creds();
    if (provisioned)
        connect_now();
}
""",
    "clear_broker": r"""
void HomeAssistant::clear_broker() {
    retract_and_disconnect();
    os.nvs.remove(id, "host");
    os.nvs.remove(id, "port");
    os.nvs.remove(id, "user");
    os.nvs.remove(id, "pass");
    mqtt_host.clear();
    mqtt_user.clear();
    mqtt_pass.clear();
    mqtt_port = DEFAULT_PORT;
    provisioned = false;
}
""",
    "connect": r"""
void HomeAssistant::connect() {
    if (mqtt.connected())
        return;
    if (!provisioned || mqtt_host.empty())
        return;
    if (last_reconnect_ms != 0 && millis() - last_reconnect_ms < RECONNECT_INTERVAL_MS)
        return;
    last_reconnect_ms = millis();
    const char* user = mqtt_user.empty() ? nullptr : mqtt_user.c_str();
    const char* pass = mqtt_pass.empty() ? nullptr : mqtt_pass.c_str();
    if (!mqtt.connect(bridge.topics.device_id.c_str(), user, pass, bridge.topics.avail.c_str(), 1,
            true, "offline")) {
        os.serial.print("[HomeAssistant] MQTT connect failed, will retry");
        return;
    }
    bridge.announce();
    os.serial.print("[HomeAssistant] MQTT connected; discovery published");
}
""",
    "handle_provision": r"""
void HomeAssistant::handle_provision() {
    WebServer& server = web_interface.get_server();
    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain"))) {
        server.send(400, "application/json", "{\"error\":\"bad_json\"}");
        return;
    }
    set_broker(std::string(doc["host"] | ""), (uint16_t)(doc["port"] | 1883),
        std::string(doc["user"] | ""), std::string(doc["pass"] | ""));
    server.send(200, "application/json", "{\"ok\":true}");
    os.serial.print("[HomeAssistant] provisioned via /provision");
}
""",
    "handle_deprovision": r"""
void HomeAssistant::handle_deprovision() {
    WebServer& server = web_interface.get_server();
    server.send(200, "application/json", "{\"ok\":true}");
    os.serial.print("[HomeAssistant] /deprovision: clearing creds + retained discovery");
    clear_broker();
}
""",
    "build_topics": r"""
void HomeAssistant::build_topics() {
    mac_hex = ha::mac_hex(wifi.get_mac_address());
    bridge.topics = ha::make_topics(mac_hex);
}
""",
    "load_creds": r"""
void HomeAssistant::load_creds() {
    mqtt_host = os.nvs.read<std::string>(id, "host", "");
    mqtt_port = os.nvs.read<uint16_t>(id, "port", DEFAULT_PORT);
    mqtt_user = os.nvs.read<std::string>(id, "user", "");
    mqtt_pass = os.nvs.read<std::string>(id, "pass", "");
    provisioned = !mqtt_host.empty();
}
""",
    "save_creds": r"""
void HomeAssistant::save_creds() const {
    os.nvs.write<std::string>(id, "host", mqtt_host);
    os.nvs.write<uint16_t>(id, "port", mqtt_port);
    os.nvs.write<std::string>(id, "user", mqtt_user);
    os.nvs.write<std::string>(id, "pass", mqtt_pass);
}
""",
}


@pytest.mark.unit
def test_every_cpp_definition_is_pinned(cpp):
    found = _definitions(cpp)
    assert sorted(found) == sorted(CPP_DEFINITIONS)
    for name, want in CPP_DEFINITIONS.items():
        assert _norm(found[name]) == _norm(want), name


@pytest.mark.unit
def test_first_boot_instructions_text(cpp):
    body = _definitions(cpp)["begin_routines_init"]
    start = body.index('os.serial.print("\\nHome Assistant setup required.')
    assert _c_text(body[start : body.index(");", start)]) == SETUP_TEXT
    # mDNS only during the first-boot wait
    assert cpp.count("MDNS.begin(") == 1 and body.count("MDNS.end();") == 3


@pytest.mark.unit
def test_files_have_spdx_headers():
    for path in (CPP, HDR, DISC, BRIDGE):
        head = path.read_text().split("\n", 2)
        assert head[0].startswith("// SPDX-FileCopyrightText: "), path
        assert head[1] == "// SPDX-License-Identifier: GPL-3.0-only", path
        # comments explain behaviour in plain words, no tracking tags
        assert not re.search(r"PORT-(FIX|QUIRK)|DECISIONS", path.read_text()), path
    assert (SRC / "README.md").is_file()
