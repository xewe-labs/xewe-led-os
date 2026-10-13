# SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
# SPDX-License-Identifier: GPL-3.0-only
# xewe-led-os/tests/unit/test_alexa.py
"""Alexa unit tests: the colour mapping (g++) and source checks of src/Alexa against the
xewe-led-os 2.3.0 behaviour (v2.3.0, src/Modules/Software/SmartHome/Alexa). The 2.3.0 values are
pinned here as literals, so the tests need neither the 2.3.0 sources nor a board.
Run from the project: `build/tools/.venv/bin/python -m xewe test --unit-only`.
"""

import re
import shutil
import subprocess

import pytest

ID = "alexa"
NAME = "Alexa"
DESCRIPTION = r"It allows to control LEDs with Amazon Alexa\nspeaker and app"

# LED colour -> Espalexa (hue16, sat8), computed with the 2.3.0 rgb_to_hsv and Alexa::sync_color
# (hue16 = h << 8 | h, saturation 255 sent as 254).
LED_TO_ALEXA = (
    ((255, 0, 0), (0, 254)),
    ((0, 255, 0), (21845, 254)),
    ((0, 0, 255), (43690, 254)),
    ((255, 255, 255), (0, 0)),
    ((0, 0, 0), (0, 0)),
    ((255, 128, 0), (5397, 254)),
    ((12, 34, 56), (38036, 200)),
    ((200, 100, 50), (3598, 191)),
    ((0, 255, 128), (27242, 254)),
    ((255, 0, 255), (54484, 254)),
    ((128, 128, 0), (10794, 254)),
    ((1, 2, 3), (38036, 169)),
    ((250, 10, 240), (54998, 244)),
    ((30, 255, 30), (21845, 225)),
    ((0, 191, 255), (35466, 254)),
    ((255, 254, 253), (5397, 1)),
)

# The first-boot discovery wait of 2.3.0, as printed.
DISCOVERY_TEXT = (
    "\\nAsk Alexa to discover new devices\\nThe setup process will continue automatically"
    "\\nafter device is pared with Alexa"
)
ABORT_PROMPT = "TO ABORT PRESS (x): "
RETRY_PROMPT = "\\n(x)?: "
SETTING_UP = "Setting up Alexa"
PAIRED = "\\nDevice successfully paired with Alexa"
REMOVE_REMINDER = "You also need to remove the device from the Alexa App manually."

# The not-found handler of 2.3.0 Alexa::begin_routines_required (compared without whitespace).
NOT_FOUND_HANDLER = (
    "server_ref.onNotFound([this,&server_ref](){"
    "if(!espalexa.handleAlexaApiCall(server_ref.uri(),server_ref.arg(0))){"
    'server_ref.send(404,"text/plain","Endpoint not found.");'
    "}"
    "});"
)


@pytest.fixture(scope="module")
def src(project_root):
    return project_root / "src" / "Alexa"


@pytest.fixture(scope="module")
def cpp(src):
    return (src / "Alexa.cpp").read_text()


@pytest.fixture(scope="module")
def code(cpp):
    return _strip_comments(cpp)


def _strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    return re.sub(r"//[^\n]*", "", text)


def _join_literals(text: str) -> str:
    """Adjacent C++ string literals joined into one ("a" "b" -> "ab")."""
    return re.sub(r'"\s*\n\s*"', "", text)


def _body(code: str, signature: str) -> str:
    """The body of `Alexa::<signature>(...)`, braces included."""
    start = code.index(f"Alexa::{signature}(")
    i = code.index("{", code.index(")", start))
    depth = 0
    for j in range(i, len(code)):
        if code[j] == "{":
            depth += 1
        elif code[j] == "}":
            depth -= 1
            if depth == 0:
                return code[i : j + 1]
    raise AssertionError(f"unbalanced body of Alexa::{signature}")


def _squash(text: str) -> str:
    return re.sub(r"\s+", "", text)


LITERAL = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')


def _norm(text: str) -> str:
    """Whitespace removed outside string and character literals; literals kept byte for byte."""
    text = _join_literals(text)
    out, at = [], 0
    for m in LITERAL.finditer(text):
        out.append(_squash(text[at : m.start()]))
        out.append(m.group(0))
        at = m.end()
    out.append(_squash(text[at:]))
    return "".join(out)


def _assert_body(code: str, signature: str, expected: str) -> None:
    """The whole body of `Alexa::<signature>` equals `expected` (every statement, in order)."""
    assert _norm(_body(code, signature)) == _norm(expected), signature


@pytest.mark.unit
def test_colour_mapping_gpp(run_host):
    out = run_host("host_alexa_map")
    assert re.search(r"^519 checks, 0 failures$", out, re.MULTILINE), out


@pytest.mark.unit
def test_map_header_kept(src):
    text = (src / "AlexaMap.h").read_text()
    assert set(re.findall(r"#include <([^>]+)>", text)) == {"cstdint"}
    code = _squash(_strip_comments(text))
    assert (
        "inlineuint16_thue8_to_hue16(uint8_thue8){"
        "returnstatic_cast<uint16_t>((hue8<<8)|hue8);}"
    ) in code
    assert "inlineuint8_tsat8_to_alexa(uint8_tsat8){returnsat8==255?254:sat8;}" in code


@pytest.mark.unit
def test_led_to_alexa_literals_gpp(project_root, src, tmp_path):
    """The core rgb_to_hsv plus AlexaMap.h give the 2.3.0 hue16/sat8 for the pinned colours."""
    gpp = shutil.which("g++")
    if gpp is None:
        pytest.skip("g++ not installed")
    core = project_root / "build" / "libraries" / "XeWeCore" / "src"
    if not (core / "XeWeCore" / "Utils" / "Color.h").is_file():
        pytest.skip("XeWeCore not installed (run ./setup.sh)")
    rows = "\n".join(f"    show({r}, {g}, {b});" for (r, g, b), _ in LED_TO_ALEXA)
    driver = tmp_path / "alexa_colour.cpp"
    driver.write_text(
        "#include <cstdio>\n"
        '#include "XeWeCore/Utils/Color.h"\n'
        '#include "AlexaMap.h"\n'
        "static void show(uint8_t r, uint8_t g, uint8_t b) {\n"
        "    const auto hsv = xewe::color::rgb_to_hsv({r, g, b});\n"
        '    std::printf("%u %u\\n", led_map::hue8_to_hue16(hsv[0]),\n'
        "        unsigned(led_map::sat8_to_alexa(hsv[1])));\n"
        "}\n"
        f"int main() {{\n{rows}\n    return 0;\n}}\n"
    )
    exe = tmp_path / "alexa_colour"
    build = subprocess.run(
        [gpp, "-std=c++17", "-Wall", "-Wextra", "-Werror", f"-I{core}", f"-I{src}"]
        + ["-o", str(exe), str(driver)],
        capture_output=True,
        text=True,
        check=False,
    )
    assert build.returncode == 0, build.stderr
    out = subprocess.run([str(exe)], capture_output=True, text=True, check=True).stdout
    got = [tuple(int(v) for v in line.split()) for line in out.splitlines()]
    assert got == [expected for _, expected in LED_TO_ALEXA]


@pytest.mark.unit
def test_identity(cpp, src):
    assert f'/* id                  */ "{ID}"' in cpp
    assert f'/* name                */ "{NAME}"' in cpp
    assert f'/* description         */ "{DESCRIPTION}"' in cpp
    for flag in ("requires_init_setup", "can_be_disabled", "has_cli_commands"):
        assert re.search(rf"/\* {flag}\s+\*/ true", cpp), flag
    header = _strip_comments((src / "Alexa.h").read_text())
    assert "class Alexa : public xewe::Module, public LedListener" in header
    # the dependencies by reference after host (CONTRACT section 8, module contract)
    assert (
        "Alexa(xewe::Os& host, Wifi& wifi_ref, WebInterface& web_interface_ref, Led& led_ref);"
        in header
    )
    assert "#include <Wifi/Wifi.h>" in header
    # 2.3.0 Alexa has no status override: `$alexa status` is the core line
    assert "status(" not in header


@pytest.mark.unit
def test_constructor_requires_wifi(code):
    """wifi is the one requirement, taken by reference: no lookup by id, no web_interface/led."""
    head = code.index("Alexa::Alexa(")
    init = _norm(code[head : code.index("{", head)])
    assert init.endswith(",web_interface(web_interface_ref),led(led_ref)"), init
    _assert_body(code, "Alexa", "{ add_requirement(wifi_ref); }")
    assert "get_module" not in code


@pytest.mark.unit
def test_not_found_handler_is_2_3_0(code):
    body = _body(code, "begin_routines_required")
    assert "WebServer& server_ref = web_interface.get_server();" in body
    assert _norm(NOT_FOUND_HANDLER) in _norm(body)
    assert 'server_ref.send(404, "text/plain", "Endpoint not found.");' in body
    # no CORS headers, no OPTIONS case: that is the Web Fallback handler, used when Alexa is off
    assert "Access-Control" not in code and "HTTP_OPTIONS" not in code


@pytest.mark.unit
def test_begin_routines_required(code):
    _assert_body(
        code,
        "begin_routines_required",
        "{"
        "    WebServer& server_ref = web_interface.get_server();"
        f"   {NOT_FOUND_HANDLER}"
        "    espalexa.begin(&server_ref);"
        "    device = new EspalexaDevice("
        "        os.system.get_device_name().c_str(),"
        "        [this](EspalexaDevice* d) { this->update_event(d); },"
        "        EspalexaDeviceType::color);"
        "    espalexa.addDevice(device);"
        "    if (!led.listeners.add(this)) {"
        '        os.serial.print("! Alexa: LED listener list full; LED changes will not reach Alexa");'
        "    }"
        "}",
    )
    # Espalexa uses the web interface's server; it neither creates one nor limits devices
    assert "new WebServer" not in code
    assert "ESPALEXA_MAXDEVICES" not in code


@pytest.mark.unit
def test_discovery_wait_is_2_3_0(code):
    _assert_body(
        code,
        "begin_routines_init",
        "{"
        f'    os.serial.print("{DISCOVERY_TEXT}");'
        f'    os.serial.print("{ABORT_PROMPT}");'
        "    while (!espalexa.get_responded_to_search()) {"
        "        espalexa.loop();"
        "        os.serial.loop();"
        "        if (os.serial.has_line()) {"
        "            std::string input = os.serial.read_line();"
        "            if (input[0] == 'x') {"
        "                disable(false, true);"
        "                return;"
        "            } else"
        f'               os.serial.print("{RETRY_PROMPT}");'
        "        }"
        "    }"
        f'    os.serial.print("{SETTING_UP}");'
        "    run_with_dots([this] { espalexa.loop(); }, 3000);"
        f'    os.serial.print("{PAIRED}");'
        "}",
    )


@pytest.mark.unit
def test_alexa_to_led(code):
    """State, brightness only while on (turning off keeps the brightness), colour; origin this."""
    _assert_body(
        code,
        "update_event",
        "{"
        "    if (is_disabled()) return;"
        "    if (!device_ptr) return;"
        "    bool state_from_alexa = device_ptr->getState();"
        "    uint8_t brightness_from_alexa = device_ptr->getValue();"
        "    uint8_t r_val = device_ptr->getR();"
        "    uint8_t g_val = device_ptr->getG();"
        "    uint8_t b_val = device_ptr->getB();"
        "    led.sync_state(state_from_alexa, this);"
        "    if (state_from_alexa) led.sync_brightness(brightness_from_alexa, this);"
        "    led.sync_color({r_val, g_val, b_val}, this);"
        "}",
    )


# every LED -> Alexa hook starts with these guards (own changes, disabled, no device yet)
GUARDS = "if (origin == this) return; if (is_disabled()) return; if (!device) return;"


@pytest.mark.unit
def test_led_to_alexa_hooks(code):
    _assert_body(
        code,
        "on_color",
        "{"
        f"   {GUARDS}"
        "    const std::array<uint8_t, 3> hsv = xewe::color::rgb_to_hsv({rgb[0], rgb[1], rgb[2]});"
        "    const uint16_t hue16 = led_map::hue8_to_hue16(hsv[0]);"
        "    const uint8_t sat8 = led_map::sat8_to_alexa(hsv[1]);"
        "    device->setColor(hue16, sat8);"
        "}",
    )
    # the brightness goes to Alexa also while the LED is off, and also when it is 0 (2.3.0: a
    # non-zero value then makes Alexa report on)
    _assert_body(code, "on_brightness", f"{{ {GUARDS} device->setValue(brightness); }}")
    _assert_body(code, "on_state", f"{{ {GUARDS} device->setState(on); }}")
    _assert_body(code, "on_mode", "{ (void)mode_id; (void)origin; }")
    _assert_body(code, "on_length", "{ (void)length; (void)origin; }")
    _assert_body(
        code,
        "on_all",
        "{"
        "    (void)mode_id;"
        "    (void)length;"
        "    if (is_disabled()) return;"
        "    on_state(on, origin);"
        "    on_brightness(brightness, origin);"
        "    on_color(rgb, origin);"
        "}",
    )


@pytest.mark.unit
def test_loop_and_reset(code):
    _assert_body(code, "loop", "{ if (is_disabled()) return; espalexa.loop(); }")
    # the reminder is printed also when not verbose (2.3.0)
    _assert_body(
        code,
        "reset",
        "{"
        f'    os.serial.print("{REMOVE_REMINDER}");'
        "    xewe::Module::reset(verbose, do_restart, keep_enabled);"
        "}",
    )


@pytest.mark.unit
def test_every_function_is_checked(code):
    """No member function beyond those whose whole body is pinned above."""
    defined = sorted(set(re.findall(r"\bAlexa::(\w+)\(", code)))
    assert defined == sorted(
        [
            "Alexa",
            "begin_routines_init",
            "begin_routines_required",
            "loop",
            "on_all",
            "on_brightness",
            "on_color",
            "on_length",
            "on_mode",
            "on_state",
            "reset",
            "update_event",
        ]
    )


@pytest.mark.unit
def test_sketch_declaration(project_root):
    """Declared (not commented out) after the generated modules and after web_fallback: the server
    keeps the last not-found handler, so an enabled Alexa must replace the fallback."""
    ino = _strip_comments((project_root / "xewe-led-os.ino").read_text())
    declared = re.search(
        r"^Alexa alexa\(os, wifi, web_interface, led\);$", ino, re.MULTILINE
    )
    assert declared, "Alexa declaration missing from the sketch (or commented out)"
    assert len(re.findall(r"\bAlexa\s+\w+\s*\(", ino)) == 1
    included = re.search(r'^#include "src/Alexa/Alexa.h"$', ino, re.MULTILINE)
    assert included and included.start() < declared.start()
    modules_at = ino.index('#include "src/Modules.h"')
    assert declared.start() > modules_at
    fallback = re.search(r"^WebFallback web_fallback\(", ino, re.MULTILINE)
    assert fallback, "web_fallback declaration missing from the sketch"
    assert fallback.start() < declared.start()


@pytest.mark.unit
def test_headers_and_comments(project_root, src):
    files = [src / "Alexa.h", src / "Alexa.cpp", src / "AlexaMap.h"]
    files.append(project_root / "tests" / "unit" / "host_alexa_map.cpp")
    for path in files:
        head = path.read_text().splitlines()[:2]
        assert head[1] == "// SPDX-License-Identifier: GPL-3.0-only", path
    for path in (src / "Alexa.h", src / "Alexa.cpp"):
        text = path.read_text()
        for word in ("PORT-FIX", "PORT-QUIRK", "DECISIONS", "(was ", "2.3.", "2026-"):
            assert word not in text, (path, word)
