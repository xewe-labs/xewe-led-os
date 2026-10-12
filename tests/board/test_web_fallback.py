# SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
# SPDX-License-Identifier: GPL-3.0-only
# xewe-led-os/tests/board/test_web_fallback.py
"""Web Fallback board tests. Run from the project: `build/tools/.venv/bin/python -m xewe test`.

Hardware preconditions: provisioned board (first boot done), WiFi connected. The module has no
commands of its own; its state is read from `$system status`. The HTTP test needs this machine to
reach the board: it runs only with XEWE_TEST_WEB_FALLBACK_HTTP=1 and reads the address from
`$wifi status` and `$web_interface get port`. It skips when the board has no IP address and when
Alexa is enabled (an enabled Alexa replaces this handler with its own until the next restart; the
Alexa board tests leave it enabled). The last test restarts the board.
"""

import os
import re
import urllib.error
import urllib.request

import pytest
from xewe.board.serialio import BOOT_READY, wait_for_banner

NAME = "Web Fallback"
# `$system status` table row of a module: name, enabled
ROW = r"^\|\s*{}\s*\|\s*(Yes|No)\s*\|"
CORS = {
    "Access-Control-Allow-Origin": "*",
    "Access-Control-Allow-Methods": "POST, GET, OPTIONS, DELETE, PUT",
    "Access-Control-Allow-Headers": "Content-Type",
}


def _system_rows(serial) -> dict[str, str]:
    """`$system status` as {module name: "Yes" | "No"} for this module and Alexa."""
    serial.send("$system status")
    text = "\n".join(serial.collect(limit=10))
    rows = {}
    for name in (NAME, "Alexa"):
        m = re.search(ROW.format(re.escape(name)), text, re.MULTILINE)
        if m:
            rows[name] = m[1]
    return rows


def _status_row(serial) -> str:
    rows = _system_rows(serial)
    assert NAME in rows, f"{NAME} missing from $system status"
    return rows[NAME]


def _host(serial) -> str:
    """`<ip>[:<port>]` of the web interface server; skips without an IP address."""
    serial.send("$wifi status")
    text = "\n".join(serial.collect(limit=10))
    m = re.search(r"^Local ip: ([\d.]+)$", text, re.MULTILINE)
    if m is None or m[1] == "0.0.0.0":
        pytest.skip("board has no IP address (WiFi not connected)")
    port = serial.command("$web_interface get port", expect=r"^port=(\d+)$", timeout=5)[
        1
    ]
    return m[1] if port == "80" else f"{m[1]}:{port}"


def _request(host: str, path: str, method: str = "GET"):
    req = urllib.request.Request(f"http://{host}{path}", method=method)
    try:
        with urllib.request.urlopen(req, timeout=5) as r:
            return r.status, dict(r.headers.items()), r.read()
    except urllib.error.HTTPError as e:
        return e.code, dict(e.headers.items()), e.read()


def test_compiles(compiled):
    assert compiled.is_file()


def test_status(serial):
    assert _status_row(serial) == "Yes"  # mandatory


def test_http_not_found_and_preflight(serial):
    if os.environ.get("XEWE_TEST_WEB_FALLBACK_HTTP") != "1":
        pytest.skip("needs network access to the board (XEWE_TEST_WEB_FALLBACK_HTTP=1)")
    if _system_rows(serial).get("Alexa") == "Yes":
        pytest.skip("Alexa is enabled: its not-found handler replaces this one")
    host = _host(serial)
    code, headers, body = _request(host, "/no-such-path")
    assert (code, body) == (404, b"404: Not Found")
    assert headers.get("Content-Type", "").startswith("text/plain")
    for name, value in CORS.items():
        assert headers.get(name) == value, name
    # a preflight to any path, routed or not, ends here: routes answer GET or POST only
    for path in ("/no-such-path", "/schedule/set", "/set"):
        code, headers, body = _request(host, path, method="OPTIONS")
        assert (code, body) == (204, b""), path
        assert headers.get("Content-Type", "").startswith("text/html"), path
        for name, value in CORS.items():
            assert headers.get(name) == value, (path, name)


def test_restart_keeps_it_enabled(serial):
    serial.send("$system restart")
    serial.expect(r"Rebooting", timeout=10)
    wait_for_banner(serial, BOOT_READY, 90, reset=False)
    serial.collect(silence=2.0, limit=40)
    assert _status_row(serial) == "Yes"
