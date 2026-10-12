# SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
# SPDX-License-Identifier: GPL-3.0-only
# xewe-led-os/tests/unit/test_schedule_page.py
"""Unit tests of the Schedule Page module (src/SchedulePage): module shape, route table, the
served page bytes, the C++ handlers (g++, tests/unit/test_schedule_page.cpp) and the page script
syntax. No board and no firmware build.

The comparison with xewe-led-os 2.3.0 reads commit 91bd344 with `git show` from XEWE_LED_OS_REPO,
else this project's own repository, else xewe-led-os next to the project; without it only the
SHA-256 values recorded here are checked. The C++ test needs ArduinoJson in build/libraries
(./setup.sh installs it).
"""

from __future__ import annotations

import hashlib
import os
import re
import shutil
import subprocess
import warnings
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src" / "SchedulePage"
PAGES = SRC / "pages"
ARDUINOJSON = "build/libraries/ArduinoJson/src"

ID = "schedule_page"
NAME = "Schedule Page"

V230 = "91bd344"
V230_DIR = "src/Modules/Software/SmartHome/WebInterface"

# file here: (file in 2.3.0, bytes and SHA-256 of the served raw literal)
ASSETS = {
    "schedule_html.h": (
        "templates/schedule_html.h",
        883,
        "425707274de37a887e39215128c2d1378d655d48d55b2b12d6cdb74f859fc86d",
    ),
    "schedule_style_css.h": (
        "static/schedule_style_css.h",
        5094,
        "d49ccfa091f817bbc741b0c21a4ed239d2a3c5058d0c0ddb6980802bb01c9044",
    ),
    "schedule_core_js.h": (
        "static/schedule_core_js.h",
        4463,
        "db2ef4244af1a073b8fee08d9c466fb31d49dedec5b2fa8a863ec98193288816",
    ),
    "schedule_actions_js.h": (
        "static/schedule_actions_js.h",
        8556,
        "9dd59248a31265734cbb2dfdf789e7ccbb49eae3d555ef7b8c859947cd719b1a",
    ),
    "schedule_interactions_js.h": (
        "static/schedule_interactions_js.h",
        4474,
        "e95a23fa0fbc60aab9ebe022948e73e4dbc4cf5cab5ad5b74132c68149b926e3",
    ),
    "schedule_ui_js.h": (
        "static/schedule_ui_js.h",
        8876,
        "a290c9c408d67ff5e4f41df750d8a2d26c777cb3dfee6d5c431aeaa268035628",
    ),
}
# the symbol each asset declares, and the content kind that serves it
SYMBOLS = {
    "schedule_html.h": ("SCHEDULE_HTML", "Page"),
    "schedule_style_css.h": ("SCHEDULE_STYLE_CSS", "Style"),
    "schedule_core_js.h": ("SCHEDULE_CORE_JS", "CoreJs"),
    "schedule_actions_js.h": ("SCHEDULE_ACTIONS_JS", "ActionsJs"),
    "schedule_interactions_js.h": ("SCHEDULE_INTERACTIONS_JS", "InteractionsJs"),
    "schedule_ui_js.h": ("SCHEDULE_UI_JS", "UiJs"),
}
EMPTY_SHA256 = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"

JINJA_CSS = "/%7B%7B%20url_for('static',%20filename='style.css')%20%7D%7D"
# (path, method, content kind) in registration order
EXPECTED_ROUTES = [
    ("/schedule", "GET", "Page"),
    ("/templates/schedule.html", "GET", "Page"),
    ("/schedule/json", "GET", "List"),
    ("/schedule/set", "POST", "Set"),
    ("/schedule/delete", "POST", "Delete"),
    (JINJA_CSS, "GET", "Style"),
    ("/static/schedule-style.css", "GET", "Style"),
    ("/static/schedule-core.js", "GET", "CoreJs"),
    ("/static/schedule-actions.js", "GET", "ActionsJs"),
    ("/static/schedule-interactions.js", "GET", "InteractionsJs"),
    ("/static/schedule-ui.js", "GET", "UiJs"),
    ("/static/schedule-utils.js", "GET", "UtilsJs"),
]
CONTENT_TYPES = {
    "Page": "text/html",
    "Style": "text/css",
    "CoreJs": "application/javascript",
    "ActionsJs": "application/javascript",
    "InteractionsJs": "application/javascript",
    "UiJs": "application/javascript",
    "UtilsJs": "application/javascript",
    "List": "application/json",
    "Set": "application/json",
    "Delete": "application/json",
}
CORS = [
    ("Access-Control-Allow-Origin", "*"),
    ("Access-Control-Allow-Methods", "POST, GET, OPTIONS, DELETE, PUT"),
    ("Access-Control-Allow-Headers", "Content-Type"),
]
BODIES = {
    "BODY_SUCCESS": '{"status": "success"}',
    "BODY_NO_PAYLOAD": '{"status": "error", "message": "No payload"}',
    "BODY_INVALID_JSON": '{"status": "error", "message": "Invalid JSON"}',
    "BODY_REJECTED": '{"status": "error", "message": "Rejected"}',
    "BODY_INVALID_ID": '{"status": "error", "message": "Invalid ID"}',
}
DEFAULT_COMMAND = "'$led set_hsv 85 255 255', '#33ff33'"

LITERAL_RE = re.compile(rb'R"rawliteral\((.*)\)rawliteral"', re.DOTALL)


# ---------------------------------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------------------------------


def _has_v230(repo: Path) -> bool:
    if not (repo / ".git").exists():
        return False
    return (
        subprocess.run(
            ["git", "-C", str(repo), "cat-file", "-e", f"{V230}^{{commit}}"],
            capture_output=True,
            check=False,
        ).returncode
        == 0
    )


def _v230_repo() -> Path | None:
    if shutil.which("git") is None:
        return None
    env = os.environ.get("XEWE_LED_OS_REPO")
    candidates = [Path(env)] if env else []
    candidates += [ROOT, ROOT.parent / "xewe-led-os"]
    for repo in candidates:
        if _has_v230(repo):
            return repo
    return None


def _v230_repo_or_skip() -> Path:
    """The 2.3.0 repository; without it a warning (shown in the summary, also in CI) and a skip:
    the values pinned in this file are then checked against the port only."""
    repo = _v230_repo()
    if repo is None:
        msg = (
            f"xewe-led-os with commit {V230} not found (XEWE_LED_OS_REPO): "
            "the cross-check of the pinned values against 2.3.0 did not run"
        )
        warnings.warn(msg, stacklevel=2)
        pytest.skip(msg)
    return repo


def _git_show(repo: Path, path: str) -> bytes:
    return subprocess.run(
        ["git", "-C", str(repo), "show", f"{V230}:{path}"],
        capture_output=True,
        check=True,
    ).stdout


def _literal(data: bytes) -> bytes:
    m = LITERAL_RE.search(data)
    assert m, 'no R"rawliteral(...)rawliteral" in the asset'
    return m.group(1)


def _api() -> str:
    return (SRC / "SchedulePageApi.h").read_text()


def _routes() -> list[tuple[str, str, str]]:
    text = _api()
    start = text.index("ROUTES[] = {")
    table = text[start : text.index("};", start)]
    return re.findall(r'\{"([^"]*)", "([A-Z]+)", Content::(\w+)\}', table)


def _body(text: str, signature: str) -> str:
    start = text.index(signature)
    return text[start : text.index("\n}\n", start)]


# ---------------------------------------------------------------------------------------------
# module shape
# ---------------------------------------------------------------------------------------------


@pytest.mark.unit
def test_module_identity_and_flags():
    cpp = (SRC / "SchedulePage.cpp").read_text()
    assert re.search(
        r"xewe::Module\(host,\s*/\* id\s*\*/ \"schedule_page\",\s*"
        r"/\* name\s*\*/ \"Schedule Page\",",
        cpp,
    )
    assert re.search(r"/\* requires_init_setup \*/ false", cpp)
    assert re.search(r"/\* can_be_disabled\s*\*/ false", cpp)
    assert re.search(r"/\* has_cli_cmds\s*\*/ false", cpp)
    header = (SRC / "SchedulePage.h").read_text()
    assert re.search(
        r"SchedulePage\(xewe::Os& host, WebInterface& web_interface_ref, "
        r"Scheduler& scheduler_ref\);",
        header,
    )


@pytest.mark.unit
def test_requires_led_web_not_scheduler():
    cpp = (SRC / "SchedulePage.cpp").read_text()
    ctor = _body(cpp, "SchedulePage::SchedulePage(")
    assert 'host.get_module("led_web")' in ctor
    assert re.findall(r"add_requirement\(([^)]*)\)", cpp) == ["*led_web"]


@pytest.mark.unit
def test_every_source_file_is_gpl():
    files = [*SRC.rglob("*.h"), *SRC.rglob("*.cpp")]
    files += [
        ROOT / "tests" / "unit" / "test_schedule_page.py",
        ROOT / "tests" / "unit" / "test_schedule_page.cpp",
        ROOT / "tests" / "board" / "test_schedule_page.py",
    ]
    for p in files:
        if p.is_file():
            head = "\n".join(p.read_text().splitlines()[:3])
            assert "SPDX-License-Identifier: GPL-3.0-only" in head, p


@pytest.mark.unit
def test_comments_describe_code_only():
    for p in [*SRC.glob("*.h"), *SRC.glob("*.cpp")]:
        text = p.read_text()
        for pattern in (
            r"DECISION",
            r"PORT-(FIX|QUIRK)",
            r"CONTRACT",
            r"\bTODO\b",
            r"\boriginal\b",
            r"\bFlask\b",
            r"\b2\.3\.\d\b",
        ):
            assert not re.search(pattern, text, re.IGNORECASE), f"{p.name}: {pattern}"


# ---------------------------------------------------------------------------------------------
# routes and handlers
# ---------------------------------------------------------------------------------------------


@pytest.mark.unit
def test_route_table():
    assert _routes() == EXPECTED_ROUTES
    api = _api()
    for name, value in CORS:
        assert f'{{"{name}", "{value}"}}' in api
    for const, body in BODIES.items():
        literal = body.replace('"', '\\"')
        assert re.search(rf'{const} =\s*"{re.escape(literal)}";', api), const


@pytest.mark.unit
def test_routes_are_registered_on_the_web_interface_server():
    cpp = (SRC / "SchedulePage.cpp").read_text()
    reg = _body(cpp, "void SchedulePage::begin_routines_regular(")
    assert "WebServer& server = web_interface.get_server();" in reg
    assert re.search(
        r"for \(const schedule_page_api::Route& route : schedule_page_api::ROUTES\)",
        reg,
    )
    assert re.search(
        r'std::strcmp\(route\.method, "POST"\) == 0 \? HTTP_POST : HTTP_GET', reg
    )
    assert "server.on(route.path, method," in reg
    # the catch-all and `/` belong to other modules
    assert "onNotFound" not in cpp and "set_root" not in cpp
    # no caching headers, only the CORS ones
    assert "Cache-Control" not in cpp and "ETag" not in cpp
    serve = _body(cpp, "void SchedulePage::serve(")
    assert re.search(r"if \(is_disabled\(\)\)\s+return;", serve)
    assert serve.index("is_disabled()") < serve.index("apply_cors();")
    assert serve.index("apply_cors();") < serve.index("switch (route.content)")
    # each asset is served from its own symbol; utils is an empty body
    for symbol, kind in SYMBOLS.values():
        assert re.search(
            rf"case schedule_page_api::Content::{kind}:\s+"
            rf"server\.send_P\(200, type, {symbol}\);",
            serve,
        ), kind
    assert re.search(
        r'case schedule_page_api::Content::UtilsJs:\s+server\.send\(200, type, ""\);',
        serve,
    )
    assert "schedule_page_api::list_schedules(scheduler)" in serve
    for fn in ("set_schedule", "delete_schedule"):
        assert re.search(
            rf'schedule_page_api::{fn}\(\s*server\.hasArg\("plain"\), '
            r"std::string\(plain\.c_str\(\), plain\.length\(\)\), scheduler\)",
            serve,
        ), fn


@pytest.mark.unit
def test_handlers_follow_v230_steps():
    api = _api()
    put = _body(api, "Response set_schedule(")
    steps = [
        "if (!has_payload)",
        "return {400, TYPE_JSON, BODY_NO_PAYLOAD};",
        "deserializeJson(doc, payload);",
        "return {400, TYPE_JSON, BODY_INVALID_JSON};",
        'doc["displayed_color"].as<std::string>()',
        'static_cast<uint8_t>(doc["day"].as<int>())',
        'static_cast<uint16_t>(doc["start_time"].as<int>())',
        'static_cast<uint16_t>(doc["end_time"].as<int>())',
        'for (JsonVariant c : doc["commands"].as<JsonArray>())',
        "if (!cmd.empty())",
        'if (!doc["id"].isNull())',
        "if (old_id > 0)",
        "scheduler.remove(static_cast<uint8_t>(old_id));",
        "scheduler.add(start, end, day, std::move(color), std::move(commands));",
        "return {400, TYPE_JSON, BODY_REJECTED};",
        "return {200, TYPE_JSON, BODY_SUCCESS};",
    ]
    at = 0
    for step in steps:
        at = put.index(step, at)
    delete = _body(api, "Response delete_schedule(")
    assert 'if (err || doc["id"].isNull())' in delete
    assert re.search(
        r'scheduler\.remove\(static_cast<uint8_t>\(doc\["id"\]\.as<int>\(\)\)\);\s+'
        r"return \{200, TYPE_JSON, BODY_SUCCESS\};",
        delete,
    )


@pytest.mark.unit
def test_routes_equal_v230_registrations():
    repo = _v230_repo_or_skip()
    cpp = _git_show(repo, f"{V230_DIR}/WebInterface.cpp").decode()
    registered = re.findall(r'httpServer\.on\("([^"]+)", HTTP_(\w+)', cpp)
    schedule = [(p, m) for p, m in registered if "schedule" in p or "style.css" in p]
    assert schedule == [(p, m) for p, m, _ in EXPECTED_ROUTES]

    def handler_body(path: str, method: str) -> str:
        at = cpp.index(f'httpServer.on("{path}", HTTP_{method}, ')
        rest = cpp[at:]
        bound = re.match(
            r'httpServer\.on\("[^"]+", HTTP_\w+, std::bind\(&WebInterface::(\w+)', rest
        )
        if bound is None:  # a lambda: its body up to the closing "});"
            return rest[: rest.index("});")]
        return _body(cpp, f"void WebInterface::{bound[1]}()")

    for path, method, kind in EXPECTED_ROUTES:
        body = handler_body(path, method)
        assert re.search(r"if \(is_disabled\(\)\) return;\s+applyCORS\(\);", body), path
        sent = re.findall(r'httpServer\.send(?:_P)?\((\d+), "([^"]+)"', body)
        assert sent and {t for _, t in sent} == {CONTENT_TYPES[kind]}, (path, sent)
        assets = {k: s for s, k in SYMBOLS.values()}
        if kind in assets:
            assert f'send_P(200, "{CONTENT_TYPES[kind]}", {assets[kind]});' in body
    utils = handler_body("/static/schedule-utils.js", "GET")
    assert 'httpServer.send(200, "application/javascript", "");' in utils
    apply = _body(cpp, "void WebInterface::applyCORS()")
    assert re.findall(r'sendHeader\("([^"]+)", "([^"]+)"\)', apply) == CORS
    assert "Cache-Control" not in cpp


@pytest.mark.unit
def test_answers_equal_v230_handlers():
    repo = _v230_repo_or_skip()
    cpp = _git_show(repo, f"{V230_DIR}/WebInterface.cpp").decode()

    def answers(fn: str) -> list[tuple[str, str]]:
        body = _body(cpp, f"void WebInterface::{fn}()")
        found = re.findall(
            r'httpServer\.send\((\d+), "application/json", "((?:[^"\\]|\\.)*)"\)', body
        )
        return [(code, text.encode().decode("unicode_escape")) for code, text in found]

    assert answers("handleScheduleSet") == [
        ("400", BODIES["BODY_NO_PAYLOAD"]),
        ("400", BODIES["BODY_INVALID_JSON"]),
        ("400", BODIES["BODY_REJECTED"]),
        ("200", BODIES["BODY_SUCCESS"]),
    ]
    assert answers("handleScheduleDelete") == [
        ("400", BODIES["BODY_NO_PAYLOAD"]),
        ("400", BODIES["BODY_INVALID_ID"]),
        ("200", BODIES["BODY_SUCCESS"]),
    ]
    put = _body(cpp, "void WebInterface::handleScheduleSet()")
    assert "if (old_id > 0) controller.scheduler.remove" in put
    delete = _body(cpp, "void WebInterface::handleScheduleDelete()")
    assert 'if (err || doc["id"].isNull())' in delete
    assert re.search(
        r"controller\.scheduler\.remove\([^;]+\);\s+"
        r'httpServer\.send\(200, "application/json", "\{\\"status\\": \\"success\\"\}"\);',
        delete,
    )


# firmware headers SchedulePage.h and the pages include; the host test declares what they provide
FIRMWARE_HEADERS = (
    "XeWeCore.h",
    "WebServer.h",
    "pgmspace.h",
    "Scheduler/Scheduler.h",
    "WebInterface/WebInterface.h",
)


@pytest.mark.unit
def test_api_and_glue_unit_gpp(run_host, tmp_path):
    """SchedulePageApi.h and SchedulePage.cpp on the host (tests/unit/test_schedule_page.cpp)."""
    if not (ROOT / ARDUINOJSON / "ArduinoJson.h").is_file():
        pytest.skip("ArduinoJson not installed (run ./setup.sh)")
    cpp = (SRC / "SchedulePage.cpp").read_text() + (SRC / "SchedulePage.h").read_text()
    for page in PAGES.glob("*.h"):
        cpp += page.read_text()
    included = set(re.findall(r"#include <([^>]+)>", cpp))
    # every firmware header the glue includes is one the host test stands in for
    assert included - {"cstring"} == set(FIRMWARE_HEADERS), included
    for name in FIRMWARE_HEADERS:
        (tmp_path / name).parent.mkdir(parents=True, exist_ok=True)
        (tmp_path / name).write_text("#pragma once\n")
    out = run_host(
        "test_schedule_page", includes=(str(tmp_path), "src/SchedulePage", ARDUINOJSON)
    )
    assert int(re.search(r"PASSED: (\d+) check", out)[1]) >= 177


@pytest.mark.unit
def test_api_header_stays_pure():
    allowed = {"cstddef", "cstdint", "string", "utility", "vector", "ArduinoJson.h"}
    assert set(re.findall(r'#include [<"]([^>"]+)[>"]', _api())) <= allowed


# ---------------------------------------------------------------------------------------------
# page assets
# ---------------------------------------------------------------------------------------------


@pytest.mark.unit
def test_assets_serve_the_v230_bytes():
    assert {p.name for p in PAGES.glob("*.h")} == set(ASSETS)
    for name, (_, size, sha) in ASSETS.items():
        served = _literal((PAGES / name).read_bytes())
        assert len(served) == size, name
        assert hashlib.sha256(served).hexdigest() == sha, name
        symbol = SYMBOLS[name][0]
        assert (
            f'static const char {symbol}[] PROGMEM = R"rawliteral('.encode()
            in (PAGES / name).read_bytes()
        )
    assert hashlib.sha256(b"").hexdigest() == EMPTY_SHA256  # /static/schedule-utils.js


@pytest.mark.unit
def test_assets_equal_v230_files():
    repo = _v230_repo_or_skip()
    for name, (old, _, sha) in ASSETS.items():
        before = _git_show(repo, f"{V230_DIR}/{old}")
        after = (PAGES / name).read_bytes()
        assert hashlib.sha256(_literal(before)).hexdigest() == sha, name
        assert _literal(before) == _literal(after), name
        # outside the literal only the path line differs, and the double blank line before the
        # declaration is one (the formatter's), and a final newline may be added
        b, a = before.split(b"\n"), after.split(b"\n")
        path = f"// xewe-led-os/src/SchedulePage/pages/{name}".encode()
        assert b[6:8] == [b"", b""], name
        expected = b[:2] + [path] + b[3:7] + b[8:]
        assert a == expected or a == [*expected, b""], name


@pytest.mark.unit
def test_page_default_command_and_links():
    js = _literal((PAGES / "schedule_interactions_js.h").read_bytes()).decode()
    assert f"app.ui.openModal('Enter CLI Commands', {DEFAULT_COMMAND}," in js
    html = _literal((PAGES / "schedule_html.h").read_bytes()).decode()
    paths = {p for p, _, _ in EXPECTED_ROUTES}
    assert 'href="/static/schedule-style.css"' in html
    scripts = re.findall(r'<script src="([^"]+)"></script>', html)
    assert scripts == [
        "static/schedule-utils.js",
        "static/schedule-core.js",
        "static/schedule-ui.js",
        "static/schedule-actions.js",
        "static/schedule-interactions.js",
    ]
    for s in scripts:  # relative to /schedule
        assert "/" + s in paths, s
    assert '<a href="/"' in html  # Home: the LED page (Led Web)
    actions = _literal((PAGES / "schedule_actions_js.h").read_bytes()).decode()
    fetched = re.findall(r"fetch\('(/[^']+)'", actions)
    assert sorted(fetched) == ["/schedule/delete", "/schedule/json", "/schedule/set"]
    methods = {p: m for p, m, _ in EXPECTED_ROUTES}
    assert methods["/schedule/set"] == methods["/schedule/delete"] == "POST"


@pytest.mark.unit
def test_page_scripts_parse(tmp_path):
    node = shutil.which("node")
    if node is None:
        pytest.skip("node not installed")
    for name in ASSETS:
        if not name.endswith("_js.h"):
            continue
        js = tmp_path / name.replace("_js.h", ".js")
        js.write_bytes(_literal((PAGES / name).read_bytes()))
        run = subprocess.run(
            [node, "--check", str(js)],
            capture_output=True,
            text=True,
            timeout=60,
            check=False,
        )
        assert run.returncode == 0, (name, run.stderr)
