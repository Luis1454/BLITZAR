#!/usr/bin/env python3
from __future__ import annotations

import os
from pathlib import Path

from python_tools.core.base_check import BaseCheck
from python_tools.core.models import CheckContext, CheckResult

FORBIDDEN_TOKENS = (
    "GRAVITY_BUILD_LEGACY_CLIENTS",
    "GRAVITY_BUILD_QT_CLIENT",
    "GRAVITY_BUILD_CLI_CLIENT",
    "QT_CLIENT_NAME",
    "CLI_CLIENT_NAME",
)
FORBIDDEN_TARGETS = ("blitzar-qt",)
EXPECTED_VARIANTS = (
    ("blitzar.exe", "blitzar"),
    ("blitzar-server.exe", "blitzar-server"),
    ("blitzar-headless.exe", "blitzar-headless"),
    ("blitzar-client.exe", "blitzar-client"),
)


class NoLegacyCheck(BaseCheck):
    name = "no_legacy"
    success_message = "Legacy standalone client guard check passed."
    failure_title = "Legacy standalone client guard check failed:"

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        root_cmake = context.root / "CMakeLists.txt"
        if not root_cmake.exists():
            result.add_error(f"Top-level CMakeLists.txt not found at {root_cmake}")
            return
        root_content = root_cmake.read_text(encoding="utf-8")
        for token in FORBIDDEN_TOKENS:
            if token in root_content:
                result.add_error(f"Forbidden legacy token found in top-level CMakeLists.txt: {token}")
        if context.check_build_targets:
            self._check_build_targets(context.build_dir, result)

    def _check_build_targets(self, build_dir: Path | None, result: CheckResult) -> None:
        if build_dir is None:
            result.add_error("build-dir is required when --check-build-targets is enabled")
            return
        build_ninja = build_dir.resolve() / "build.ninja"
        if not build_ninja.exists():
            result.add_error(f"build.ninja not found at {build_ninja}")
            return
        ninja_content = build_ninja.read_text(encoding="utf-8")
        for target in FORBIDDEN_TARGETS:
            if target in ninja_content:
                result.add_error(f"Forbidden legacy target found in build.ninja: {target}")
        for variants in EXPECTED_VARIANTS:
            if not any(variant in ninja_content for variant in variants):
                name = variants[0] if os.name == "nt" else variants[1]
                result.add_error(f"Expected runtime target missing from build.ninja: {name}")

