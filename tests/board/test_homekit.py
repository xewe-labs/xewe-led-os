# SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
# SPDX-License-Identifier: GPL-3.0-only
# xewe-led-os/tests/board/test_homekit.py
"""HomeKit board tests. Run from the project: `build/tools/.venv/bin/python -m xewe test`.

Hardware preconditions: a provisioned board (first boot done, Wi-Fi connected) with HomeKit
enabled and paired with a Home hub. With HomeKit disabled the tests that need it skip.

The last test disables HomeKit (answering the core's confirmation), which erases the HomeKit
pairings, restarts, enables it again and aborts the pairing wait with `x`, then runs
`$homekit reset` while HomeKit is disabled (HomeSpan never started on that boot: only the stored
pairing list is erased, the board must not crash) and aborts the pairing wait that follows. It
leaves HomeKit disabled and unpaired, so it runs only with XEWE_TEST_HOMEKIT_DISABLE=1. With
XEWE_TEST_HOMEKIT_PAIR=1 as well it enables HomeKit once more, waits up to 5 minutes for a person
to pair it in the Home app (setup code 4663-7726) and checks that the Brightness characteristic
stored before the disable survived: the reset erases the pairings only.
"""

import os
import re

import pytest
from xewe.board.serialio import BOOT_READY, wait_for_banner

ID = "homekit"
NAME = "HomeKit"
# the input line of a firmware prompt; answer only after it
INPUT = r"^(?:\(y/n\) )?> $"
QR_URL = (
    "https://github.com/xewe-labs/xewe-led-os/raw/v2.3.0/static/media/resources/"
    "HomeKit_Connect_QR.png"
)


def _status(serial) -> list[str]:
    serial.send(f"${ID} status")
    return serial.collect(silence=1.5, limit=15)


def _enabled(serial) -> bool:
    return any(f"{NAME} module enabled" in line for line in _status(serial))


def _needs_enabled(serial) -> None:
    if not _enabled(serial):
        pytest.skip("HomeKit is disabled on this board")


def _led_brightness(serial) -> int:
    serial.send("$led status")
    text = "\n".join(serial.collect(silence=1.0, limit=10))
    return int(re.search(r"Brightness:\s+(\d+)/255", text)[1])


def _led_live(serial) -> tuple[tuple[int, int, int], bool]:
    """The colour and power state from `$led status`."""
    serial.send("$led status")
    text = "\n".join(serial.collect(silence=1.0, limit=30))
    rgb = re.search(r"Color \(RGB\):\s+\((\d+), (\d+), (\d+)\)", text)
    state = re.search(r"Power State:\s+(ON|OFF)", text)
    return (int(rgb[1]), int(rgb[2]), int(rgb[3])), state[1] == "ON"


def _characteristic(serial, name: str) -> str:
    """A characteristic's value from the HomeSpan info dump in `$homekit status`."""
    text = "\n".join(_status(serial))
    return re.search(rf"Characteristic {name}\(([^)]*)\)", text)[1]


def _homekit_brightness(serial) -> int:
    return int(_characteristic(serial, "Brightness"))


def _wait_reboot(serial) -> None:
    serial.expect(r"Rebooting", timeout=10)
    wait_for_banner(serial, BOOT_READY, 90, reset=False)
    serial.collect(silence=2.0, limit=40)


def test_compiles(compiled):
    # the project firmware with HomeKit declared, for the session chip
    assert compiled.is_file()


def test_status(serial):
    # enabled: the HomeSpan status and info dumps come first, the module line last;
    # disabled: nothing at all
    lines = _status(serial)
    text = "\n".join(lines)
    if f"{NAME} module enabled" not in text:
        assert not re.search(rf"{NAME} module|HomeSpan", text), text
        pytest.skip("HomeKit is disabled on this board")
    status_at = next(i for i, x in enumerate(lines) if "*** HomeSpan Status ***" in x)
    info_at = next(i for i, x in enumerate(lines) if "*** HomeSpan Info ***" in x)
    module_at = next(i for i, x in enumerate(lines) if f"{NAME} module enabled" in x)
    assert status_at < info_at < module_at
    assert re.search(r"Characteristic Brightness\(", text)
    assert re.search(r"Characteristic Hue\(", text)


def test_system_status_prints_the_dump(serial):
    _needs_enabled(serial)
    serial.send("$system status")
    text = "\n".join(serial.collect(silence=1.5, limit=20))
    assert "*** HomeSpan Status ***" in text and "*** HomeSpan Info ***" in text


def test_led_brightness_mirrored(serial):
    # led -> Home app: round((b / 255) * 100), and 0 is passed through below HomeKit's minimum 1
    _needs_enabled(serial)
    before = _led_brightness(serial)
    try:
        for byte, pct in ((128, 50), (0, 0), (255, 100)):
            serial.send(f"$led set_brightness {byte}")
            serial.collect(silence=0.5, limit=5)
            assert _homekit_brightness(serial) == pct, byte
    finally:
        serial.send(f"$led set_brightness {before}")
        serial.command("$led status", expect=rf"Brightness:\s+{before}/255", timeout=5)


def test_led_colour_and_state_mirrored(serial):
    # led -> Home app: rgb -> hue/saturation (V ignored), power state -> On
    _needs_enabled(serial)
    rgb_before, on_before = _led_live(serial)
    try:
        serial.send("$led set_rgb 255 0 0")
        serial.collect(silence=0.5, limit=5)
        assert _characteristic(serial, "Hue") == "0"
        assert _characteristic(serial, "Saturation") == "100"
        serial.send("$led set_rgb 0 0 255")
        serial.collect(silence=0.5, limit=5)
        assert _characteristic(serial, "Hue") == "240"
        assert _characteristic(serial, "Saturation") == "100"
        serial.send("$led turn_on")
        serial.collect(silence=0.5, limit=5)
        assert _characteristic(serial, "On") == "1"
        serial.send("$led turn_off")
        serial.collect(silence=0.5, limit=5)
        assert _characteristic(serial, "On") == "0"
    finally:
        serial.send("$led set_rgb {} {} {}".format(*rgb_before))
        serial.collect(silence=0.5, limit=5)
        serial.send("$led turn_on" if on_before else "$led turn_off")
        serial.collect(silence=0.5, limit=5)
        assert _led_live(serial) == (rgb_before, on_before)


@pytest.mark.skipif(
    os.environ.get("XEWE_TEST_HOMEKIT_DISABLE") != "1",
    reason="erases the HomeKit pairings: set XEWE_TEST_HOMEKIT_DISABLE=1",
)
def test_disable_enable_pairing_wait(serial):
    # core Module::disable asks "[WARNING] Disabling <name> / Will reset it / OK?" (2 x 15 s,
    # default no): answer y. The reset erases the pairings and the board restarts.
    _needs_enabled(serial)
    brightness_before = _led_brightness(serial)
    # a stored characteristic value other than the default 100, checked after the re-pairing
    serial.send("$led set_brightness 128")
    serial.collect(silence=0.5, limit=5)
    assert _homekit_brightness(serial) == 50
    serial.command(f"${ID} disable", expect=r"OK\?", timeout=5)
    serial.expect(INPUT, timeout=5)
    serial.send("y")
    serial.expect(rf"{NAME} module disabled", timeout=10)
    _wait_reboot(serial)
    assert not _enabled(serial)

    # enable: the namespace was reset, so the next boot runs the pairing wait again
    serial.command(f"${ID} enable", expect=rf"{NAME} module enabled", timeout=10)
    serial.expect(r"Rebooting", timeout=10)
    serial.expect(r"^Open the link with the Setup QR below and scan it$", timeout=90)
    serial.expect(r"^with your iPhone/iPad$", timeout=5)
    serial.expect(rf"^{re.escape(QR_URL)}$", timeout=5)
    serial.expect(r"^using code 4663-7726$", timeout=5)
    serial.expect(r"^after device is pared with HomeKit$", timeout=5)
    serial.expect(r"TO ABORT PRESS \(x\): ", timeout=5)
    serial.send("?")
    serial.expect(r"\(x\)\?: ", timeout=5)
    # x: disable without a question, then restart
    serial.send("x")
    _wait_reboot(serial)
    assert not _enabled(serial)

    # reset while disabled: HomeSpan never started on this boot, so only the stored pairing list
    # is erased; the reset re-enables the module and the next boot runs the pairing wait
    start = len(serial.lines)
    serial.command(
        f"${ID} reset",
        expect=r"^You also need to remove the device from the Home App manually$",
        timeout=5,
    )
    serial.expect(rf"^{NAME} module reset$", timeout=10)
    serial.expect(r"Rebooting", timeout=10)
    serial.expect(r"TO ABORT PRESS \(x\): ", timeout=90)
    text = "\n".join(serial.lines[start:])
    assert not re.search(r"Guru Meditation|panic|abort\(\)|Backtrace", text), text
    serial.send("x")
    _wait_reboot(serial)
    assert not _enabled(serial)

    if os.environ.get("XEWE_TEST_HOMEKIT_PAIR") == "1":
        serial.command(f"${ID} enable", expect=rf"{NAME} module enabled", timeout=10)
        serial.expect(r"TO ABORT PRESS \(x\): ", timeout=90)
        serial.expect(r"^Setting up HomeKit$", timeout=300)
        serial.expect(r"^Device successfully paired with HomeKit\.$", timeout=10)
        wait_for_banner(serial, BOOT_READY, 90, reset=False)
        assert _enabled(serial)
        # 'U' erased the pairings only; 'F' would also have erased the stored values (100)
        assert _homekit_brightness(serial) == 50

    serial.send(f"$led set_brightness {brightness_before}")
    serial.command(
        "$led status", expect=rf"Brightness:\s+{brightness_before}/255", timeout=5
    )
