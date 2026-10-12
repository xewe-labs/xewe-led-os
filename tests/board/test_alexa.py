# SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
# SPDX-License-Identifier: GPL-3.0-only
# xewe-led-os/tests/board/test_alexa.py
"""Alexa board tests (src/Alexa). Run from the project: `build/tools/.venv/bin/python -m xewe test`.

Hardware preconditions: provisioned board (first boot done, Alexa enabled and its discovery wait
passed, the led wizard completed), WiFi connected, the web interface on port 80 (Espalexa always
announces :80). The HTTP tests also need this machine to reach the board: they run only with
XEWE_TEST_ALEXA_HTTP=1 and read the board's address from `$led_web status`. They talk to the
Espalexa API the way an Echo does (no Echo needed). Discovery by a real Echo runs only with
XEWE_TEST_ALEXA_ECHO=1 and a person saying "Alexa, discover devices". Each test restores what it
changes. The last two tests restart the board: one aborts the discovery wait with `x`, one
disables the module; both enable it again and complete the discovery wait by fetching
/description.xml themselves (so they need XEWE_TEST_ALEXA_HTTP=1). Alexa is disabled between the
two restarts of each test; a failure in between can leave it disabled.
"""

import json
import os
import re
import time
import urllib.error
import urllib.request

import pytest
from xewe.board.serialio import BOOT_READY, wait_for_banner

ID = "alexa"
NAME = "Alexa"
# the input line of a firmware prompt; answer only after it
INPUT = r"^(?:\(y/n\) )?> $"
# `$led_web status`: the page address (Alexa needs port 80, so no :<port> is expected)
PAGE = r"^Page: http://([\d.]+(?::\d+)?)/$"
# `/state` of the led page: F<rgb>,<brightness>,<state>,<mode>
STATE = r"F([0-9A-F]{6}),(\d+),([01]),(\d+)"
DISCOVERY = r"Ask Alexa to discover new devices"
REMOVE_REMINDER = r"You also need to remove the device from the Alexa App manually\."
# the Web Fallback answer, in force while Alexa is disabled (2.3.0 web-interface fallback)
CORS = {
    "Access-Control-Allow-Origin": "*",
    "Access-Control-Allow-Methods": "POST, GET, OPTIONS, DELETE, PUT",
    "Access-Control-Allow-Headers": "Content-Type",
}
# LED colour -> Espalexa (hue, sat): hue16 = h << 8 | h, saturation 255 sent as 254
HUE_SAT = (
    ((255, 0, 0), (0, 254)),
    ((0, 255, 0), (21845, 254)),
    ((255, 128, 0), (5397, 254)),
    ((12, 34, 56), (38036, 200)),
)


def _needs_http():
    if os.environ.get("XEWE_TEST_ALEXA_HTTP") != "1":
        pytest.skip("needs network access to the board (XEWE_TEST_ALEXA_HTTP=1)")


def _enabled(serial) -> bool:
    m = serial.command(
        f"${ID} status", expect=rf"^{NAME} module (enabled|disabled)$", timeout=5
    )
    return m[1] == "enabled"


def _needs_enabled(serial):
    if not _enabled(serial):
        pytest.skip("Alexa is disabled on this board")


def _host(serial) -> str:
    m = serial.command("$led_web status", expect=PAGE, timeout=5)
    if m[1].split(":")[0] == "0.0.0.0":
        pytest.skip("board has no IP address")
    if ":" in m[1]:
        pytest.skip("web interface is not on port 80 (Espalexa announces :80)")
    return m[1]


def _request_full(
    host: str, path: str, method: str = "GET", body: bytes | None = None
) -> tuple[int, dict, str]:
    # A body goes out as JSON, as an Echo sends it. urllib's default for a body is
    # application/x-www-form-urlencoded, which the ESP32 WebServer parses as form fields, so
    # Espalexa would see no `plain` argument and ignore the command.
    headers = {"Content-Type": "application/json"} if body is not None else {}
    req = urllib.request.Request(
        f"http://{host}{path}", data=body, method=method, headers=headers
    )
    try:
        with urllib.request.urlopen(req, timeout=5) as r:
            return r.status, dict(r.headers.items()), r.read().decode()
    except urllib.error.HTTPError as e:
        return e.code, dict(e.headers.items()), e.read().decode()


def _request(host: str, path: str, method: str = "GET", body: bytes | None = None):
    code, headers, text = _request_full(host, path, method, body)
    return code, headers.get("Content-Type", ""), text


def _assert_fallback(host: str) -> None:
    """The Web Fallback answers: CORS headers, OPTIONS 204, else 404 text/plain."""
    code, headers, body = _request_full(host, "/no-such-page")
    assert (code, body) == (404, "404: Not Found")
    assert headers.get("Content-Type", "").startswith("text/plain")
    for name, value in CORS.items():
        assert headers.get(name) == value, name
    code, headers, body = _request_full(host, "/no-such-page", method="OPTIONS")
    assert (code, body) == (204, "")
    assert headers.get("Content-Type", "").startswith("text/html")
    for name, value in CORS.items():
        assert headers.get(name) == value, name


def _complete_discovery(serial, host: str) -> None:
    """After a restart into the discovery wait: check the re-prompt, then end the wait the way an
    Echo does (by fetching /description.xml) and wait for the boot to finish."""
    serial.expect(DISCOVERY, timeout=90)
    serial.expect(r"TO ABORT PRESS \(x\):", timeout=5)
    serial.send("q")
    serial.expect(r"\(x\)\?:", timeout=5)
    deadline = time.monotonic() + 30
    while True:
        try:
            if _request(host, "/description.xml")[0] == 200:
                break
        except OSError:
            pass
        assert time.monotonic() < deadline, "description.xml not served"
        time.sleep(1)
    serial.expect(r"Setting up Alexa", timeout=10)
    serial.expect(r"Device successfully paired with Alexa", timeout=10)
    wait_for_banner(serial, BOOT_READY, 90, reset=False)
    serial.collect(silence=2.0, limit=40)


def _enable_and_complete(serial, host: str) -> None:
    """`$alexa enable` restarts into the discovery wait; complete it."""
    serial.command(f"${ID} enable", expect=rf"{NAME} module enabled", timeout=10)
    serial.expect(r"Rebooting", timeout=10)
    _complete_discovery(serial, host)


def _light(host: str) -> tuple[str, dict]:
    """The key and state of the single Espalexa light."""
    code, ctype, body = _request(host, "/api/test/lights")
    assert code == 200 and ctype.startswith("application/json"), (code, ctype)
    lights = json.loads(body)
    assert len(lights) == 1, lights
    key, light = next(iter(lights.items()))
    return key, light["state"]


def _led_state(host: str) -> re.Match:
    code, _, body = _request(host, "/state")
    assert code == 200
    m = re.fullmatch(STATE, body)
    assert m, body
    return m


def _wait_reboot(serial) -> None:
    serial.expect(r"Rebooting", timeout=10)
    wait_for_banner(serial, BOOT_READY, 90, reset=False)
    serial.collect(silence=2.0, limit=40)


def test_compiles(compiled):
    assert compiled.is_file()


def test_status(serial):
    # 2.3.0: the core line only, also when disabled
    serial.command(
        f"${ID} status", expect=rf"^{NAME} module (enabled|disabled)$", timeout=5
    )


def test_http_not_found_is_alexa_handler(serial):
    _needs_http()
    _needs_enabled(serial)
    host = _host(serial)
    assert _request(host, "/no-such-page") == (404, "text/plain", "Endpoint not found.")
    # OPTIONS is not special-cased while Alexa owns the handler (2.3.0)
    assert _request(host, "/no-such-page", method="OPTIONS")[0] == 404
    # any path containing "api" is an Espalexa call
    assert _request(host, "/api")[:2] == (200, "application/json")
    code, ctype, body = _request(host, "/description.xml")
    assert code == 200 and "xml" in ctype and ":80/" in body
    assert _request(host, "/espalexa")[0] == 200


def test_led_changes_reach_alexa(serial):
    _needs_http()
    _needs_enabled(serial)
    host = _host(serial)
    led = _led_state(host)
    brightness, on = int(led[2]), led[3] == "1"
    other = 77 if brightness != 77 else 78
    try:
        serial.send("$led turn_on")
        serial.send(f"$led set_brightness {other}")
        time.sleep(1)
        _, state = _light(host)
        assert state["on"] is True
        serial.send("$led turn_off")
        time.sleep(1)
        _, state = _light(host)
        assert state["on"] is False
        # 2.3.0: a brightness change while off is still sent, and Alexa then reports on
        serial.send(f"$led set_brightness {brightness}")
        time.sleep(1)
        _, state = _light(host)
        assert state["on"] is True
        assert _led_state(host)[3] == "0"  # the strip itself stays off
    finally:
        serial.send(f"$led set_brightness {brightness}")
        serial.send("$led turn_on" if on else "$led turn_off")
        time.sleep(1)


def test_led_colour_reaches_alexa_as_hue_sat(serial):
    _needs_http()
    _needs_enabled(serial)
    host = _host(serial)
    color = _led_state(host)[1]
    try:
        for (r, g, b), (hue, sat) in HUE_SAT:
            serial.send(f"$led set_rgb {r} {g} {b}")
            time.sleep(1)
            _, state = _light(host)
            assert (state["hue"], state["sat"]) == (hue, sat), (r, g, b)
    finally:
        serial.send(
            f"$led set_rgb {int(color[0:2], 16)} {int(color[2:4], 16)} {int(color[4:6], 16)}"
        )
        time.sleep(1)


def test_alexa_commands_reach_led(serial):
    _needs_http()
    _needs_enabled(serial)
    host = _host(serial)
    led = _led_state(host)
    color, brightness, on = led[1], int(led[2]), led[3] == "1"
    key, _ = _light(host)
    path = f"/api/test/lights/{key}/state"
    try:
        # brightness while on: Espalexa stores bri + 1
        assert _request(host, path, "PUT", b'{"on":true,"bri":99}')[0] == 200
        time.sleep(1)
        led = _led_state(host)
        assert (led[2], led[3]) == ("100", "1")
        # off keeps the brightness (it is not sent to the LED while off)
        assert _request(host, path, "PUT", b'{"on":false}')[0] == 200
        time.sleep(1)
        led = _led_state(host)
        assert (led[2], led[3]) == ("100", "0")
    finally:
        serial.send(f"$led set_brightness {brightness}")
        serial.send(
            f"$led set_rgb {int(color[0:2], 16)} {int(color[2:4], 16)} {int(color[4:6], 16)}"
        )
        serial.send("$led turn_on" if on else "$led turn_off")
        time.sleep(1)


def test_discovery_by_echo(serial):
    if os.environ.get("XEWE_TEST_ALEXA_ECHO") != "1":
        pytest.skip("needs an Echo on the same network (XEWE_TEST_ALEXA_ECHO=1)")
    _needs_enabled(serial)
    # `$alexa reset` restarts into the discovery wait; a person asks the Echo to discover devices
    serial.send(f"${ID} reset")
    serial.expect(REMOVE_REMINDER, timeout=5)
    serial.expect(DISCOVERY, timeout=90)
    serial.expect(r"TO ABORT PRESS \(x\):", timeout=5)
    serial.expect(r"Setting up Alexa", timeout=300)
    serial.expect(r"Device successfully paired with Alexa", timeout=10)
    wait_for_banner(serial, BOOT_READY, 90, reset=False)
    serial.collect(silence=2.0, limit=40)
    assert _enabled(serial)


def test_discovery_abort_with_x(serial):
    _needs_http()
    _needs_enabled(serial)
    host = _host(serial)
    # `$alexa reset` keeps the module enabled and restarts into the discovery wait
    serial.send(f"${ID} reset")
    serial.expect(REMOVE_REMINDER, timeout=5)
    serial.expect(r"Rebooting", timeout=10)
    serial.expect(DISCOVERY, timeout=90)
    serial.expect(r"TO ABORT PRESS \(x\):", timeout=5)
    # `x` disables without a confirmation: the reminder, then a restart
    serial.send("x")
    serial.expect(REMOVE_REMINDER, timeout=5)
    _wait_reboot(serial)
    try:
        serial.command(f"${ID} status", expect=rf"^{NAME} module disabled$", timeout=5)
        _assert_fallback(host)
    finally:
        _enable_and_complete(serial, host)
    assert _enabled(serial)
    assert _request(host, "/no-such-page") == (404, "text/plain", "Endpoint not found.")


def test_disable_enable_across_restart(serial):
    _needs_http()
    _needs_enabled(serial)
    host = _host(serial)
    # core Module::disable asks "[WARNING] Disabling <name> / Will reset it / OK?" (get_yn, 2 x 15 s,
    # default no): answer y. Alexa prints its reminder, the namespace is reset, the board restarts.
    serial.command(f"${ID} disable", expect=r"OK\?", timeout=5)
    serial.expect(INPUT, timeout=5)
    serial.send("y")
    serial.expect(rf"{NAME} module disabled", timeout=10)
    serial.expect(REMOVE_REMINDER, timeout=5)
    _wait_reboot(serial)
    try:
        serial.command(f"${ID} status", expect=rf"^{NAME} module disabled$", timeout=5)
        # the Web Fallback answers unknown paths while Alexa is disabled
        _assert_fallback(host)
    finally:
        _enable_and_complete(serial, host)
    assert _enabled(serial)
    assert _request(host, "/no-such-page") == (404, "text/plain", "Endpoint not found.")
