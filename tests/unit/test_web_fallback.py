# SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
# SPDX-License-Identifier: GPL-3.0-only
# xewe-led-os/tests/unit/test_web_fallback.py
"""Unit tests of the Web Fallback module (src/WebFallback): module shape, the hook that installs
the not-found handler, the handler's answers (g++, tests/unit/test_web_fallback.cpp) and their
equality with the xewe-led-os 2.3.0 web interface handler, and the declaration order in the
sketch. No board and no firmware build.

The comparison with 2.3.0 reads commit 91bd344 with `git show` from XEWE_LED_OS_REPO, else this
project's own repository, else xewe-led-os next to the project.
"""

from __future__ import annotations

import os
import re
import shutil
import subprocess
import warnings
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src" / "WebFallback"
V230 = "91bd344"
V230_WI = "src/Modules/Software/SmartHome/WebInterface/WebInterface.cpp"

CORS = [
    ("Access-Control-Allow-Origin", "*"),
    ("Access-Control-Allow-Methods", "POST, GET, OPTIONS, DELETE, PUT"),
    ("Access-Control-Allow-Headers", "Content-Type"),
]


def _v230_repo() -> Path | None:
    if shutil.which("git") is None:
        return None
    env = os.environ.get("XEWE_LED_OS_REPO")
    for repo in [*([Path(env)] if env else []), ROOT, ROOT.parent / "xewe-led-os"]:
        if (repo / ".git").exists() and (
            subprocess.run(
                ["git", "-C", str(repo), "cat-file", "-e", f"{V230}^{{commit}}"],
                capture_output=True,
                check=False,
            ).returncode
            == 0
        ):
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


def _body(text: str, signature: str) -> str:
    start = text.index(signature)
    return text[start : text.index("\n}\n", start)]


def _declarations(ino: str) -> list[str]:
    """Variable names of the module declarations that are not commented out, in order."""
    return re.findall(r"^\s*[A-Z]\w+ (\w+)\(os[,)]", ino, re.MULTILINE)


@pytest.mark.unit
def test_module_identity_and_flags():
    cpp = (SRC / "WebFallback.cpp").read_text()
    assert re.search(
        r"xewe::Module\(host,\s*/\* id\s*\*/ \"web_fallback\",\s*"
        r"/\* name\s*\*/ \"Web Fallback\",",
        cpp,
    )
    assert re.search(r"/\* requires_init_setup \*/ false", cpp)
    assert re.search(r"/\* can_be_disabled\s*\*/ false", cpp)  # mandatory
    assert re.search(r"/\* has_cli_cmds\s*\*/ false", cpp)  # no $web_fallback group
    assert "add_requirement" not in cpp
    header = (SRC / "WebFallback.h").read_text()
    assert "WebFallback(xewe::Os& host, WebInterface& web_interface_ref);" in header
    assert "Alexa" not in re.sub(r"//.*", "", header + cpp)  # no dependency on Alexa


@pytest.mark.unit
def test_handler_installed_in_the_first_routine():
    cpp = (SRC / "WebFallback.cpp").read_text()
    required = _body(cpp, "void WebFallback::begin_routines_required(")
    assert "web_interface.get_server().onNotFound(" in required
    assert cpp.count("onNotFound(") == 1
    for hook in (
        "begin_routines_init",
        "begin_routines_regular",
        "begin_routines_common",
    ):
        assert hook not in cpp, hook


@pytest.mark.unit
def test_handler_sends_cors_then_the_answer():
    cpp = (SRC / "WebFallback.cpp").read_text()
    handler = _body(cpp, "void WebFallback::handle_not_found(")
    assert "is_disabled" not in handler
    assert re.search(
        r"for \(const auto& header : web_fallback_api::CORS_HEADERS\)\s+"
        r"server\.sendHeader\(header\[0\], header\[1\]\);",
        handler,
    )
    assert "web_fallback_api::answer(server.method() == HTTP_OPTIONS)" in handler
    assert re.search(
        r"if \(a\.type == nullptr\)\s+server\.send\(a\.code\);\s+else\s+"
        r"server\.send\(a\.code, a\.type, a\.body\);",
        handler,
    )
    assert handler.index("sendHeader") < handler.index("server.send(")


@pytest.mark.unit
def test_api_values():
    api = (SRC / "WebFallbackApi.h").read_text()
    for name, value in CORS:
        assert f'{{"{name}", "{value}"}}' in api
    assert 'PREFLIGHT = {204, nullptr, ""};' in api
    assert 'NOT_FOUND = {404, "text/plain", "404: Not Found"};' in api
    assert not re.findall(r"#include", api)


@pytest.mark.unit
def test_api_unit_gpp(run_host):
    out = run_host("test_web_fallback", includes=("src/WebFallback",))
    assert int(re.search(r"PASSED: (\d+) check", out)[1]) >= 12


@pytest.mark.unit
def test_handler_equals_v230():
    repo = _v230_repo_or_skip()
    cpp = subprocess.run(
        ["git", "-C", str(repo), "show", f"{V230}:{V230_WI}"],
        capture_output=True,
        check=True,
        text=True,
    ).stdout
    at = cpp.index("httpServer.onNotFound(")
    handler = cpp[at : cpp.index("});", at)]
    assert re.search(
        r"applyCORS\(\);\s+if \(httpServer\.method\(\) == HTTP_OPTIONS\) \{\s+"
        r"httpServer\.send\(204\);\s+\} else \{\s+"
        r'httpServer\.send\(404, "text/plain", "404: Not Found"\);',
        handler,
    )
    assert "is_disabled" not in handler
    apply = _body(cpp, "void WebInterface::applyCORS()")
    assert re.findall(r'sendHeader\("([^"]+)", "([^"]+)"\)', apply) == CORS
    # installed in begin_routines_required, once
    required = _body(cpp, "void WebInterface::begin_routines_required(")
    assert "httpServer.onNotFound(" in required
    assert cpp.count("onNotFound(") == 1


@pytest.mark.unit
def test_declared_before_alexa():
    # the server keeps the last not-found handler: Alexa (when enabled) must begin after this
    names = _declarations((ROOT / "xewe-led-os.ino").read_text())
    assert "web_fallback" in names, "web_fallback is not declared in the sketch"
    if "alexa" in names:
        assert names.index("web_fallback") < names.index("alexa")


@pytest.mark.unit
def test_every_source_file_is_gpl():
    files = [*SRC.glob("*.h"), *SRC.glob("*.cpp")]
    files += [
        ROOT / "tests" / "unit" / "test_web_fallback.py",
        ROOT / "tests" / "unit" / "test_web_fallback.cpp",
        ROOT / "tests" / "board" / "test_web_fallback.py",
    ]
    for p in files:
        if p.is_file():
            head = "\n".join(p.read_text().splitlines()[:3])
            assert "SPDX-License-Identifier: GPL-3.0-only" in head, p
