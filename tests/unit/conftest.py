# SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
# SPDX-License-Identifier: GPL-3.0-only
# xewe-led-os/tests/unit/conftest.py
"""Shared helpers of the project unit tests (tests/unit)."""

import shutil
import subprocess
from pathlib import Path

import pytest

UNIT = Path(__file__).resolve().parent
ROOT = UNIT.parents[1]  # tests/unit/ -> tests/ -> project root


@pytest.fixture(scope="session")
def project_root() -> Path:
    """The project root (the folder that holds xewe.toml)."""
    return ROOT


@pytest.fixture(scope="session")
def run_host(tmp_path_factory):
    """Compile tests/unit/<name>.cpp with g++ -Wall -Wextra -Werror, run it, return stdout (skip
    without g++). Extra include directories are relative to the project root."""

    def run(name: str, includes: tuple[str, ...] = ()) -> str:
        gpp = shutil.which("g++")
        if gpp is None:
            pytest.skip("g++ not installed")
        exe = tmp_path_factory.mktemp(name) / name
        flags = [f"-I{ROOT / inc}" for inc in includes]
        build = subprocess.run(
            [
                gpp,
                "-std=c++17",
                "-Wall",
                "-Wextra",
                "-Werror",
                *flags,
                "-o",
                str(exe),
                str(UNIT / f"{name}.cpp"),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        assert build.returncode == 0, build.stderr
        result = subprocess.run([str(exe)], capture_output=True, text=True, check=False)
        assert result.returncode == 0, result.stdout + result.stderr
        return result.stdout

    return run
