# SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
# SPDX-License-Identifier: GPL-3.0-only
# xewe-led-os/tests/board/test_schedule_page.py
"""Schedule Page board tests. Run from the project: `build/tools/.venv/bin/python -m xewe test`.

Hardware preconditions: provisioned board (first boot done), Led Web enabled, WiFi connected.
The module has no commands of its own; its state is read from `$system status`. The HTTP tests
also need this machine to reach the board: they run only with XEWE_TEST_SCHEDULE_PAGE_HTTP=1 and
read the board's address from `$led_web status`. The block the HTTP test adds is deleted again.
The last test restarts the board.
"""

import hashlib
import json
import os
import re
import urllib.error
import urllib.request

import pytest
from xewe.board.serialio import BOOT_READY, wait_for_banner

NAME = "Schedule Page"
# `$system status` table row of this module
ROW = rf"^\|\s*{NAME}\s*\|\s*(Yes|No)\s*\|"
# `$led_web status`: the page address, with :<port> when the web interface's port is not 80
PAGE = r"^Page: http://([\d.]+(?::\d+)?)/$"
CORS = {
    "Access-Control-Allow-Origin": "*",
    "Access-Control-Allow-Methods": "POST, GET, OPTIONS, DELETE, PUT",
    "Access-Control-Allow-Headers": "Content-Type",
}
# SHA-256 of the served bytes (src/SchedulePage/pages/*.h raw literals)
SERVED = {
    "/schedule": (
        "text/html",
        "425707274de37a887e39215128c2d1378d655d48d55b2b12d6cdb74f859fc86d",
    ),
    "/templates/schedule.html": (
        "text/html",
        "425707274de37a887e39215128c2d1378d655d48d55b2b12d6cdb74f859fc86d",
    ),
    "/%7B%7B%20url_for('static',%20filename='style.css')%20%7D%7D": (
        "text/css",
        "d49ccfa091f817bbc741b0c21a4ed239d2a3c5058d0c0ddb6980802bb01c9044",
    ),
    "/static/schedule-style.css": (
        "text/css",
        "d49ccfa091f817bbc741b0c21a4ed239d2a3c5058d0c0ddb6980802bb01c9044",
    ),
    "/static/schedule-core.js": (
        "application/javascript",
        "db2ef4244af1a073b8fee08d9c466fb31d49dedec5b2fa8a863ec98193288816",
    ),
    "/static/schedule-actions.js": (
        "application/javascript",
        "9dd59248a31265734cbb2dfdf789e7ccbb49eae3d555ef7b8c859947cd719b1a",
    ),
    "/static/schedule-interactions.js": (
        "application/javascript",
        "e95a23fa0fbc60aab9ebe022948e73e4dbc4cf5cab5ad5b74132c68149b926e3",
    ),
    "/static/schedule-ui.js": (
        "application/javascript",
        "a290c9c408d67ff5e4f41df750d8a2d26c777cb3dfee6d5c431aeaa268035628",
    ),
    "/static/schedule-utils.js": (
        "application/javascript",
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
    ),
}


def _needs_network():
    if os.environ.get("XEWE_TEST_SCHEDULE_PAGE_HTTP") != "1":
        pytest.skip(
            "needs network access to the board (XEWE_TEST_SCHEDULE_PAGE_HTTP=1)"
        )


def _host(serial) -> str:
    m = serial.command("$led_web status", expect=PAGE, timeout=5)
    if m[1].split(":")[0] == "0.0.0.0":
        pytest.skip("board has no IP address")
    return m[1]


def _request(host: str, path: str, body: bytes | None = None):
    """Status, headers and raw body of GET (or POST with a JSON body) http://<host><path>."""
    req = urllib.request.Request(f"http://{host}{path}", data=body)
    if body is not None:
        req.add_header("Content-Type", "application/json")
    try:
        with urllib.request.urlopen(req, timeout=5) as r:
            return r.status, dict(r.headers.items()), r.read()
    except urllib.error.HTTPError as e:
        return e.code, dict(e.headers.items()), e.read()


def _status_row(serial) -> str:
    serial.send("$system status")
    text = "\n".join(serial.collect(limit=10))
    m = re.search(ROW, text, re.MULTILINE)
    assert m, f"{NAME} missing from $system status"
    return m[1]


def test_compiles(compiled):
    assert compiled.is_file()


def test_status(serial):
    # cannot be disabled: enabled whenever Led Web is
    assert _status_row(serial) == "Yes"


def test_http_served_bytes_and_headers(serial):
    _needs_network()
    host = _host(serial)
    for path, (ctype, sha) in SERVED.items():
        code, headers, body = _request(host, path)
        assert code == 200, path
        assert headers.get("Content-Type", "").startswith(ctype), (path, headers)
        assert hashlib.sha256(body).hexdigest() == sha, path
        for name, value in CORS.items():
            assert headers.get(name) == value, (path, name)
        assert "Cache-Control" not in headers, path


def test_http_set_and_delete(serial):
    _needs_network()
    host = _host(serial)
    code, headers, body = _request(host, "/schedule/json")
    assert code == 200 and headers["Content-Type"].startswith("application/json")
    before = json.loads(body)
    block = {
        "commands": ["$led set_hsv 85 255 255", ""],
        "displayed_color": "33ff33",
        "day": 6,
        "start_time": 1425,
        "end_time": 1440,
    }
    code, _, body = _request(host, "/schedule/set", json.dumps(block).encode())
    if code == 400:
        assert body == b'{"status": "error", "message": "Rejected"}'
        pytest.skip("scheduler disabled: saves are rejected")
    assert (code, body) == (200, b'{"status": "success"}')
    after = json.loads(_request(host, "/schedule/json")[2])
    added = [b for b in after if b not in before]
    assert len(added) == 1
    new = added[0]
    try:
        assert new["commands"] == ["$led set_hsv 85 255 255"]  # empty commands dropped
        assert (new["day"], new["start_time"], new["end_time"]) == (6, 1425, 1440)
        assert new["displayed_color"] == "33ff33"
    finally:
        code, _, body = _request(
            host, "/schedule/delete", json.dumps({"id": new["id"]}).encode()
        )
        assert (code, body) == (200, b'{"status": "success"}')
    assert json.loads(_request(host, "/schedule/json")[2]) == before
    # an unknown id is a success too; a missing id is not
    assert _request(host, "/schedule/delete", b'{"id": 250}')[0] == 200
    assert _request(host, "/schedule/delete", b"{}")[:3:2] == (
        400,
        b'{"status": "error", "message": "Invalid ID"}',
    )


def test_restart_keeps_it_enabled(serial):
    serial.send("$system restart")
    serial.expect(r"Rebooting", timeout=10)
    wait_for_banner(serial, BOOT_READY, 90, reset=False)
    serial.collect(silence=2.0, limit=40)
    assert _status_row(serial) == "Yes"
