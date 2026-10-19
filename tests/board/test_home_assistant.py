# SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
# SPDX-License-Identifier: GPL-3.0-only
# xewe-led-os/tests/board/test_home_assistant.py
"""Home Assistant module (src/HomeAssistant/) on a board. Run from the project:
`build/tools/.venv/bin/python -m xewe test`.

Hardware preconditions: a provisioned board (first boot done), Wi-Fi connected. `xewe provision`
cannot answer the module's first-boot wait, so the module may be enabled or disabled; the tests
that need it enabled skip otherwise. A test that changes the module skips when a broker is
stored (a working setup is never wiped or replaced) and ends with the module enabled and
unprovisioned.

Opt-in groups (environment); each names what it changes on the board:
- XEWE_TEST_HA_HTTP=1: this machine reaches the board over HTTP. /provision with bad JSON and
  with an empty host, and /deprovision, on an unprovisioned module (nothing is stored).
- XEWE_TEST_HA_MQTT_HOST (+ _PORT, _USER, _PASS), with XEWE_TEST_HA_HTTP=1: a real broker, and
  paho-mqtt for the MQTT side. Provisions it; checks the retained discovery and state, the echo
  of Home Assistant commands and of LED changes made from the CLI and from Alexa (when enabled),
  mode by name and a parameter round trip (both restored); deprovisions and checks that the
  retained entities are gone and avail is offline.
- XEWE_TEST_HA_ROUND_TRIP=1, with the broker: `$homeassistant reset`, and `$homeassistant disable`
  then `enable`, each restart into the first-boot wait (web interface on a regular boot). The
  test completes the wait through /provision and deprovisions. With XEWE_TEST_HA_MDNS=1 and
  python-zeroconf: the `_xewe-led-os._tcp` record during the wait, gone after it, and HomeSpan's
  `_hap._tcp` record answering after it (HomeKit enabled).
- XEWE_TEST_HA_WEB_RESET=1, with XEWE_TEST_HA_HTTP=1: `$web_interface reset` puts the web interface
  settings back to their defaults (port 80, no root redirect). The boot after it is the web
  interface's setup boot and a regular boot of Home Assistant; the web interface starts its
  server there too, so /provision answers whether Alexa is enabled or not. The test checks that
  boot and the one after `$system restart`.
- XEWE_TEST_HA_FACTORY_RESET=1, with the broker and the `xewe provision` settings (XEWE_WIFI_SSID,
  XEWE_WIFI_PASSWORD, optional XEWE_DEVICE_NAME / XEWE_TIMEZONE / XEWE_PROVISION_MODULES):
  `$system reset` erases every setting (Wi-Fi, device name, LED setup, every module); the tools'
  provisioning driver answers the first-boot prompts again. HomeKit is declined (its pairing wait
  needs a person) and ends DISABLED with no pairings; Alexa's discovery wait is ended by fetching
  /description.xml. On that first boot the web interface and Home Assistant are both on their
  setup boot: the web interface starts its server and /provision must answer. Home Assistant ends enabled and deprovisioned.

Checked by hand: a change made in the Home app reaches Home Assistant (needs a paired HomeKit
controller); the last will (cut the power while connected: avail turns `offline`); the blocking
reconnect while the broker is stopped.
"""

import importlib.util
import json
import os
import re
import threading
import time
import urllib.error
import urllib.request
from typing import Self

import pytest
from xewe.board.serialio import BOOT_READY, wait_for_banner

ID = "homeassistant"
NAME = "Home_Assistant"
# the input line of a firmware prompt; answer only after it
INPUT = r"^(?:\(y/n\) )?> $"
# The module is enabled only through its first-boot wait, which ends when a broker is provisioned
# (or with `x`, which disables it), so enabling a disabled module needs a broker
DISABLED = "needs an MQTT broker (Home Assistant is disabled on this board)"
STATUS = (
    r"^Home Assistant: (?:not provisioned \(POST /provision to pair\)"
    r"|broker (\S+):(\d+) \[(connected|disconnected)\])$"
)
WAITING = (
    r"\[HomeAssistant\] waiting for Home Assistant discovery card or /provision\.\.\."
)
PAIRED = r"\[HomeAssistant\] paired: MQTT connected"
DEPROVISIONED = r"\[HomeAssistant\] /deprovision: clearing creds \+ retained discovery"


# ---- helpers ---------------------------------------------------------------------------------


def _until(serial, check, timeout: float, what: str):
    """Poll `check()` until it returns something true, reading the console in between (the
    fixture's bounded wait; no blind sleeps). Fails after `timeout` seconds."""
    deadline = time.monotonic() + timeout
    while True:
        value = check()
        if value:
            return value
        assert time.monotonic() < deadline, f"timed out after {timeout} s: {what}"
        serial.collect(silence=0.2, limit=0.5)


def _wait_reboot(serial) -> None:
    serial.expect(r"Rebooting", timeout=15)
    wait_for_banner(serial, BOOT_READY, 120, reset=False)
    serial.collect(silence=2.0, limit=40)


def _table_row(serial, name: str) -> re.Match:
    """The `$system status` row of module `name`: (enabled Yes|No, status text)."""
    m = serial.command(
        "$system status",
        expect=rf"\|\s*{name}\s*\|\s*(Yes|No)\s*\|\s*([^|]*?)\s*\|",
        timeout=10,
    )
    serial.collect(silence=1.0, limit=10)
    return m


def _enabled(serial, name: str = NAME) -> bool:
    # `$homeassistant status` prints nothing while disabled (2.3.0); the system table always
    # has the row
    return _table_row(serial, name)[1] == "Yes"


def _status(serial) -> re.Match:
    m = serial.command(f"${ID} status", expect=STATUS, timeout=10)
    serial.expect(rf"^{NAME} module enabled$", timeout=5)
    return m


def _provisioned(serial) -> bool:
    return _status(serial)[0].startswith("Home Assistant: broker")


def _need_http() -> None:
    if os.environ.get("XEWE_TEST_HA_HTTP") != "1":
        pytest.skip("needs HTTP access to the board (XEWE_TEST_HA_HTTP=1)")


def _need_gate(name: str, what: str) -> None:
    if os.environ.get(name) != "1":
        pytest.skip(f"{what} ({name}=1)")


def _broker() -> dict:
    host = os.environ.get("XEWE_TEST_HA_MQTT_HOST")
    if not host:
        pytest.skip("needs an MQTT broker (XEWE_TEST_HA_MQTT_HOST)")
    return {
        "host": host,
        "port": int(os.environ.get("XEWE_TEST_HA_MQTT_PORT", "1883")),
        "user": os.environ.get("XEWE_TEST_HA_MQTT_USER", ""),
        "pass": os.environ.get("XEWE_TEST_HA_MQTT_PASS", ""),
    }


def _need_unprovisioned(serial) -> None:
    if not _enabled(serial):
        pytest.skip(DISABLED)
    if _provisioned(serial):
        pytest.skip("a broker is stored; not wiping or replacing it")


def _address(serial) -> tuple[str, str]:
    """(`<ip>[:<port>]` of the web interface server, MAC as 12 lower-case hex)."""
    serial.send("$wifi status")
    ip = serial.expect(r"^Local ip: ([\d.]+)$", timeout=5)[1]
    mac = serial.expect(r"^Mac: ([0-9A-F:]{17})$", timeout=5)[1]
    port = serial.command("$web_interface get port", expect=r"^port=(\d+)$", timeout=5)[
        1
    ]
    if ip == "0.0.0.0":
        pytest.skip("board has no IP address")
    return (ip if port == "80" else f"{ip}:{port}"), mac.replace(":", "").lower()


def _post(host: str, path: str, body: bytes | None) -> tuple[int, str, str]:
    req = urllib.request.Request(
        f"http://{host}{path}",
        data=body if body is not None else b"",
        method="POST",
        headers={"Content-Type": "application/json"},
    )
    try:
        with urllib.request.urlopen(req, timeout=20) as r:
            return r.status, r.headers.get("Content-Type", ""), r.read().decode()
    except urllib.error.HTTPError as e:
        return e.code, e.headers.get("Content-Type", ""), e.read().decode()


def _probe(host: str) -> int | None:
    """HTTP status of a harmless `POST /provision` (bad JSON: 400, nothing stored), or None when
    nothing answers (connection refused or timed out)."""
    try:
        return _post(host, "/provision", b"{not json")[0]
    except (urllib.error.URLError, OSError):
        return None


def _deprovision(serial, host: str) -> None:
    assert _post(host, "/deprovision", None) == (200, "application/json", '{"ok":true}')
    serial.expect(DEPROVISIONED, timeout=10)


class _Mqtt:
    """A paho-mqtt client on the device's topics: every payload per topic, in arrival order."""

    def __init__(self, broker: dict, mac: str) -> None:
        mqtt = pytest.importorskip("paho.mqtt.client")
        self.dev = f"xewe_led_os_{mac}"
        self.base = f"xewe_led_os/{self.dev}"
        self.got: dict[str, list[str]] = {}
        self.lock = threading.Lock()
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        if broker["user"]:
            self.client.username_pw_set(broker["user"], broker["pass"])
        self.client.on_message = self._on_message
        self.client.connect(broker["host"], broker["port"])
        self.client.subscribe(
            [(f"{self.base}/#", 0), (f"homeassistant/+/{self.dev}/#", 0)]
        )
        self.client.loop_start()

    def _on_message(self, client, userdata, msg) -> None:
        with self.lock:
            self.got.setdefault(msg.topic, []).append(msg.payload.decode())

    def last(self, topic: str) -> str | None:
        with self.lock:
            return (self.got.get(topic) or [None])[-1]

    def count(self, topic: str) -> int:
        with self.lock:
            return len(self.got.get(topic, []))

    def topics(self) -> set[str]:
        with self.lock:
            return set(self.got)

    def publish(self, topic: str, payload: str) -> None:
        self.client.publish(topic, payload)

    def __enter__(self) -> Self:
        return self

    def __exit__(self, *exc) -> None:
        self.client.loop_stop()
        self.client.disconnect()


def _light_state(m: _Mqtt) -> dict | None:
    text = m.last(f"{m.base}/light/state")
    return json.loads(text) if text else None


def _led_brightness(serial) -> int:
    return int(
        serial.command("$led status", expect=r"Brightness:\s+(\d+)/255", timeout=5)[1]
    )


# ---- status and commands -------------------------------------------------------------------


def test_compiles(compiled):
    assert compiled.is_file()


def test_status(serial):
    if _enabled(serial):
        _status(serial)
    else:
        # 2.3.0: a disabled module prints nothing for `status`
        start = len(serial.lines)
        serial.send(f"${ID} status")
        serial.collect(silence=1.5, limit=5)
        assert not any("Home" in line for line in serial.lines[start:])


def test_system_status_prints_the_ha_line_before_the_table(serial):
    # 2.3.0 order: status() prints its own line while `$system status` collects the rows, so the
    # line comes before the table; the row shows the core line (or "disabled")
    serial.send("$system status")
    lines = serial.collect(silence=1.5, limit=15)
    title = next(i for i, line in enumerate(lines) if "System Status" in line)
    ha = [i for i, line in enumerate(lines) if re.search(STATUS, line)]
    rows = [line for line in lines if re.search(rf"\|\s*{NAME}\s*\|", line)]
    assert len(rows) == 1, rows
    if re.search(r"\|\s*Yes\s*\|", rows[0]):
        assert len(ha) == 1 and ha[0] < title, lines
        assert re.search(rf"\|\s*{NAME} module enabled\s*\|", rows[0]), rows[0]
    else:
        assert ha == [], lines
        # core wraps a long cell onto continuation lines, so join the Status cell back together
        at = lines.index(rows[0])
        cell = [rows[0].split("|")[-2]]
        for line in lines[at + 1 :]:
            if not line.lstrip().startswith("|"):
                break
            cell.append(line.split("|")[-2])
        assert " ".join(" ".join(cell).split()) == "Home Assistant module disabled", lines[at:]


def test_only_generic_commands(serial):
    for cmd in ("set host 192.0.2.1", "get host", "schema"):
        serial.command(
            f"${ID} {cmd}",
            expect=rf"Error: Unknown command '{cmd.split()[0]}' in command group '{ID}'",
            timeout=5,
        )


# ---- HTTP ----------------------------------------------------------------------------------


def test_provision_rejects_bad_json(serial):
    _need_http()
    if not _enabled(serial):
        pytest.skip(DISABLED)
    host, _ = _address(serial)
    assert _post(host, "/provision", b"{not json") == (
        400,
        "application/json",
        '{"error":"bad_json"}',
    )
    # an empty body does not parse either
    assert _post(host, "/provision", None)[0] == 400


def test_provision_empty_host_and_deprovision(serial):
    _need_http()
    _need_unprovisioned(serial)
    host, _ = _address(serial)
    # an empty host stores "unprovisioned" and still answers ok
    assert _post(host, "/provision", b'{"host":""}') == (
        200,
        "application/json",
        '{"ok":true}',
    )
    serial.expect(r"\[HomeAssistant\] provisioned via /provision", timeout=5)
    assert not _provisioned(serial)
    _deprovision(serial, host)
    assert not _provisioned(serial)


# ---- a real broker -------------------------------------------------------------------------


def _check_discovery(serial, m: _Mqtt) -> None:
    _until(serial, lambda: m.last(f"{m.base}/light/state"), 10, "retained light state")
    assert m.last(f"{m.base}/avail") == "online"
    light = m.last(f"homeassistant/light/{m.dev}/config")
    assert light.startswith(
        '{"~":"'
        + m.base
        + '","schema":"json","name":null,"unique_id":"'
        + m.dev
        + '_light",'
    )
    assert json.loads(light)["device"]["manufacturer"] == "XeWe"
    assert '"options":["Solid","Color Fade",' in m.last(
        f"homeassistant/select/{m.dev}/config"
    )
    assert m.last(f"{m.base}/mode/state")
    numbers = [t for t in m.topics() if t.startswith(f"homeassistant/number/{m.dev}/")]
    assert numbers, "number entities of the running mode"


def _check_echoes(serial, m: _Mqtt, host: str) -> None:
    light = f"{m.base}/light/state"
    before = _light_state(m)
    try:
        # a Home Assistant command comes back as the confirmed state (no own-origin filter)
        m.publish(f"{m.base}/light/set", '{"brightness":77}')
        _until(serial, lambda: _light_state(m)["brightness"] == 77, 10, "HA echo")
        # an LED change from the CLI is published at once
        serial.send("$led set_brightness 66")
        _until(serial, lambda: _light_state(m)["brightness"] == 66, 10, "CLI echo")
        # an LED change from Alexa (Espalexa API, as an Echo sends it) is published too
        if _enabled(serial, "Alexa") and ":" not in host:
            _, _, body = _post_json(host, "/api/test/lights", None, "GET")
            key = next(iter(json.loads(body)))
            seen = m.count(light)
            assert (
                _post_json(
                    host,
                    f"/api/test/lights/{key}/state",
                    b'{"on":true,"bri":99}',
                    "PUT",
                )[0]
                == 200
            )
            _until(serial, lambda: m.count(light) > seen, 10, "Alexa echo")
            _until(
                serial,
                lambda: _light_state(m)["brightness"] == _led_brightness(serial),
                10,
                "HA state equals the LED after the Alexa change",
            )
    finally:
        m.publish(
            f"{m.base}/light/set",
            json.dumps({"state": before["state"], "brightness": before["brightness"]}),
        )
        _until(serial, lambda: _light_state(m) == before, 10, "light state restored")


def _post_json(host: str, path: str, body: bytes | None, method: str):
    req = urllib.request.Request(
        f"http://{host}{path}",
        data=body,
        method=method,
        headers={"Content-Type": "application/json"} if body is not None else {},
    )
    with urllib.request.urlopen(req, timeout=10) as r:
        return r.status, r.headers.get("Content-Type", ""), r.read().decode()


def _check_mode_and_param(serial, m: _Mqtt) -> None:
    mode_state = f"{m.base}/mode/state"
    original = m.last(mode_state)
    speed = f"{m.base}/param/speed/state"
    try:
        # the select carries the mode NAME; the number entities follow the mode
        m.publish(f"{m.base}/mode/set", "Rainbow")
        _until(serial, lambda: m.last(mode_state) == "Rainbow", 10, "mode by name")
        _until(
            serial,
            lambda: m.last(f"homeassistant/number/{m.dev}/density/config"),
            10,
            "Rainbow's number entities",
        )
        speed_before = _until(serial, lambda: m.last(speed), 10, "speed state")
        try:
            m.publish(f"{m.base}/param/speed/set", "3")
            _until(serial, lambda: m.last(speed) == "3", 10, "parameter echo")
            # out of range: the led clamps (Rainbow speed 1..20), the result is published
            m.publish(f"{m.base}/param/speed/set", "99")
            _until(serial, lambda: m.last(speed) == "20", 10, "clamped parameter echo")
        finally:
            m.publish(f"{m.base}/param/speed/set", speed_before)
            _until(serial, lambda: m.last(speed) == speed_before, 10, "speed restored")
        # an unknown name does nothing
        seen = m.count(mode_state)
        m.publish(f"{m.base}/mode/set", "rainbow")
        serial.collect(silence=2.0, limit=3)
        assert m.count(mode_state) == seen
    finally:
        m.publish(f"{m.base}/mode/set", original)
        _until(serial, lambda: m.last(mode_state) == original, 10, "mode restored")


def _check_cleared(serial, broker: dict, mac: str) -> None:
    # a new subscriber gets only the retained messages: after /deprovision that is avail=offline
    # (the discovery and state topics were emptied, which deletes them)
    with _Mqtt(broker, mac) as m:
        _until(serial, lambda: m.last(f"{m.base}/avail"), 10, "retained avail")
        serial.collect(silence=2.0, limit=3)
        assert m.topics() == {f"{m.base}/avail"}, m.topics()
        assert m.last(f"{m.base}/avail") == "offline"


def test_broker_provision_and_deprovision(serial):
    _need_http()
    broker = _broker()
    _need_unprovisioned(serial)
    host, mac = _address(serial)
    try:
        # set_broker connects before the answer is sent
        assert _post(host, "/provision", json.dumps(broker).encode()) == (
            200,
            "application/json",
            '{"ok":true}',
        )
        serial.expect(
            r"\[HomeAssistant\] MQTT connected; discovery published", timeout=30
        )
        m = _status(serial)
        assert (m[1], m[2], m[3]) == (broker["host"], str(broker["port"]), "connected")
        if importlib.util.find_spec("paho") is not None:
            with _Mqtt(broker, mac) as client:
                _check_discovery(serial, client)
                _check_echoes(serial, client, host)
                _check_mode_and_param(serial, client)
    finally:
        _deprovision(serial, host)
    assert not _provisioned(serial)
    if importlib.util.find_spec("paho") is not None:
        _check_cleared(serial, broker, mac)


# ---- first-boot wait -----------------------------------------------------------------------


def _mdns(serial, service: str, seconds: float) -> list:
    """The `service` records answered within `seconds` (python-zeroconf ServiceInfo)."""
    zeroconf = pytest.importorskip("zeroconf")
    zc = zeroconf.Zeroconf()
    found = []

    class Listener:
        def add_service(self, z, type_, name):
            info = z.get_service_info(type_, name, 3000)
            if info:
                found.append(info)

        def update_service(self, z, type_, name):
            pass

        def remove_service(self, z, type_, name):
            pass

    browser = zeroconf.ServiceBrowser(zc, service, Listener())
    try:
        serial.collect(silence=seconds, limit=seconds)
    finally:
        browser.cancel()
        zc.close()
    return found


def _mdns_wanted() -> bool:
    return os.environ.get("XEWE_TEST_HA_MDNS") == "1"


def _complete_first_boot_wait(serial, broker: dict, host: str, mac: str) -> None:
    """From the restart into the wait to the end of that boot: instructions, mDNS, the server
    the module started itself, /provision, pairing. Then deprovision."""
    serial.expect(r"Home Assistant setup required\.", timeout=120)
    serial.expect(r"To abort and disable Home Assistant, press \(x\):", timeout=10)
    serial.expect(
        rf"\[HomeAssistant\] mDNS discovery active: xewe-led-os-{mac[-6:]}"
        r"\._xewe-led-os\._tcp\.local",
        timeout=10,
    )
    serial.expect(WAITING, timeout=10)
    ip = host.split(":")[0]
    if _mdns_wanted():
        records = [
            r
            for r in _mdns(serial, "_xewe-led-os._tcp.local.", 6)
            if r.properties.get(b"mac") == mac.encode()
        ]
        assert len(records) == 1, records
        assert records[0].port == 80
        assert records[0].server == f"xewe-led-os-{mac[-6:]}.local."
        assert records[0].properties.get(b"fw")
    # the module started the web interface server itself for the wait
    assert _post(host, "/provision", json.dumps(broker).encode())[0] == 200
    serial.expect(PAIRED, timeout=60)
    wait_for_banner(serial, BOOT_READY, 120, reset=False)
    serial.collect(silence=2.0, limit=40)
    if _mdns_wanted():
        # mDNS ended with the wait; HomeSpan's responder (started after it) still answers
        assert not [
            r
            for r in _mdns(serial, "_xewe-led-os._tcp.local.", 6)
            if r.properties.get(b"mac") == mac.encode()
        ]
        if _enabled(serial, "HomeKit"):
            hap = [
                r
                for r in _mdns(serial, "_hap._tcp.local.", 6)
                if ip in r.parsed_addresses()
            ]
            assert len(hap) == 1, hap
    _deprovision(serial, host)
    assert _enabled(serial) and not _provisioned(serial)


def _need_round_trip(serial) -> tuple[dict, str, str]:
    _need_gate("XEWE_TEST_HA_ROUND_TRIP", "restarts into the first-boot wait")
    _need_http()
    broker = (
        _broker()
    )  # without a broker the wait could only end with `x` (module disabled)
    _need_unprovisioned(serial)
    host, mac = _address(serial)
    return broker, host, mac


def test_reset_first_boot_wait(serial):
    # `$homeassistant reset`: the module's setup boot over a web interface regular boot
    broker, host, mac = _need_round_trip(serial)
    serial.command(f"${ID} reset", expect=rf"{NAME} module reset", timeout=10)
    serial.expect(r"Rebooting", timeout=15)
    _complete_first_boot_wait(serial, broker, host, mac)


def test_disable_enable_first_boot_wait(serial):
    broker, host, mac = _need_round_trip(serial)
    serial.command(f"${ID} disable", expect=r"OK\?", timeout=5)
    serial.expect(INPUT, timeout=5)
    serial.send("y")
    serial.expect(rf"{NAME} module disabled", timeout=10)
    _wait_reboot(serial)
    try:
        assert not _enabled(serial)
    finally:
        serial.command(f"${ID} enable", expect=rf"{NAME} module enabled", timeout=10)
        serial.expect(r"Rebooting", timeout=15)
        _complete_first_boot_wait(serial, broker, host, mac)


# ---- the web interface setup boot ----------------------------------------------------------


def test_web_interface_setup_boot_serves_provision(serial):
    _need_gate(
        "XEWE_TEST_HA_WEB_RESET", "resets the web interface settings to their defaults"
    )
    _need_http()
    if not _enabled(serial):
        pytest.skip(DISABLED)
    host = _address(serial)[0]
    assert _probe(host) == 400
    ip = host.split(":")[0]  # the reset puts the port back to 80
    start = len(serial.lines)
    serial.command(
        "$web_interface reset", expect=r"Web Interface module reset", timeout=10
    )
    _wait_reboot(serial)
    # the web interface starts its server on its setup boot too (and says so), so /provision
    # answers on this boot whether Alexa is enabled or not
    assert any("Web Interface now available" in line for line in serial.lines[start:])
    assert _probe(ip) == 400
    serial.command("$system restart", expect=r"Rebooting", timeout=10)
    wait_for_banner(serial, BOOT_READY, 120, reset=False)
    serial.collect(silence=2.0, limit=40)
    assert _probe(ip) == 400


# ---- a fresh device ------------------------------------------------------------------------


def test_fresh_device_provision_reachable(serial):
    # every module on its setup boot: the web interface starts its server before Home
    # Assistant's wait, which then answers /provision
    _need_gate(
        "XEWE_TEST_HA_FACTORY_RESET",
        "erases every setting with $system reset, re-provisions, leaves HomeKit disabled",
    )
    _need_http()
    broker = _broker()
    _need_unprovisioned(serial)
    from xewe.board import provision
    from xewe.report import XeWeError

    try:
        settings = provision.resolve("xewe-led-os", env=os.environ)
    except XeWeError as exc:
        pytest.skip(f"xewe provision settings incomplete: {exc}")
    if not settings.enables("home-assistant"):
        pytest.skip("XEWE_PROVISION_MODULES leaves Home Assistant out")

    class NoHomeKit(provision.Settings):
        def enables(self, slug: str) -> bool:
            return slug != "homekit" and super().enables(slug)

    settings = NoHomeKit(
        name=settings.name,
        modules=settings.modules,
        ssid=settings.ssid,
        password=settings.password,
        timezone=settings.timezone,
    )
    ip = _address(serial)[0].split(":")[0]  # the reset puts the port back to 80
    answered: dict[str, float] = {}
    done = threading.Event()

    def http_side() -> None:
        # /provision as soon as anything listens; then end Alexa's discovery wait (if asked)
        while not done.is_set() and "provision" not in answered:
            try:
                if _post(ip, "/provision", json.dumps(broker).encode())[0] == 200:
                    answered["provision"] = time.monotonic()
            except (urllib.error.URLError, OSError):
                done.wait(1.0)
        while not done.is_set():
            try:
                with urllib.request.urlopen(f"http://{ip}/description.xml", timeout=5):
                    answered["alexa"] = time.monotonic()
                    return
            except (urllib.error.URLError, OSError):
                done.wait(1.0)

    serial.secrets.append(settings.password)
    worker = threading.Thread(target=http_side, daemon=True)
    try:
        serial.command("$system reset", expect=r"OK\?", timeout=5)
        serial.expect(INPUT, timeout=5)
        start = len(serial.lines)
        serial.send("y")
        serial.expect(r"Rebooting", timeout=30)
        worker.start()
        m = wait_for_banner(serial, provision.START_EVENT, 120, reset=False)
        driver = provision.Driver(serial, settings)
        driver.run(*driver.classify(m.string))
        first_boot = serial.lines[start:]
        serial.expect(r"Rebooting", timeout=30)
        wait_for_banner(serial, BOOT_READY, 120, reset=False)
        serial.collect(silence=2.0, limit=40)
    finally:
        done.set()
        worker.join(timeout=30)
        serial.secrets.remove(settings.password)
    assert "provision" in answered, "/provision never answered on the first boot"
    text = "\n".join(first_boot)
    # the web interface served on its setup boot and the wait ended paired
    assert "Web Interface now available" in text
    assert re.search(r"\[HomeAssistant\] provisioned via /provision", text)
    assert re.search(PAIRED, text)
    _deprovision(serial, ip)
    assert _enabled(serial) and not _provisioned(serial)
