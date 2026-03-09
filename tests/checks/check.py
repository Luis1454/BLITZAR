#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import sys
from collections.abc import Callable
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.checks.clang_tidy import ClangTidyCheck
from python_tools.checks.ini_check import IniCheck
from python_tools.checks.ivv_gate import IvvGateCheck
from python_tools.checks.launcher_check import LauncherContractCheck
from python_tools.checks.main_delivery_policy import MainDeliveryGateCheck
from python_tools.checks.mirror_check import MirrorCheck
from python_tools.checks.no_legacy_check import NoLegacyCheck
from python_tools.checks.pr_policy import PrPolicyCheck, PrPolicySelfTestCheck
from python_tools.checks.python_quality_gate import PythonQualityGateCheck
from python_tools.checks.quality_baseline import QualityBaselineCheck
from python_tools.checks.repo_policy import RepoPolicyCheck
from python_tools.checks.test_catalog import TestCatalogCheck
from python_tools.checks.traceability_gate import TraceabilityGateCheck
from python_tools.core.base_check import BaseCheck
from python_tools.core.models import CheckContext
from python_tools.core.reporting import ResultReporter
from python_tools.core.runner import CheckRunner

CHECKS: dict[str, Callable[[], BaseCheck]] = {
    "ini": IniCheck,
    "mirror": MirrorCheck,
    "no_legacy": NoLegacyCheck,
    "launcher": LauncherContractCheck,
    "quality": QualityBaselineCheck,
    "test_catalog": TestCatalogCheck,
    "pr_policy": PrPolicySelfTestCheck,
    "repo": RepoPolicyCheck,
    "python_quality": PythonQualityGateCheck,
    "clang_tidy": ClangTidyCheck,
    "pr_policy_gate": PrPolicyCheck,
    "traceability_gate": TraceabilityGateCheck,
    "ivv_gate": IvvGateCheck,
    "main_delivery_gate": MainDeliveryGateCheck,
}


def _add_common_repo_args(parser: argparse.ArgumentParser, *, config: bool = False, build: bool = False, with_launcher: bool = False) -> None:
    parser.add_argument("--root", default=".", help="Repository root directory")
    if config:
        parser.add_argument("--config", default="simulation.ini", help="Path to simulation.ini")
    if build:
        parser.add_argument("--build-dir", default="", help="Build directory")
        parser.add_argument("--check-build-targets", action="store_true", help="Validate generated targets")
    if with_launcher:
        parser.add_argument("--with-launcher", action="store_true", help="Include launcher contract in all checks")
    parser.add_argument("--target-lines", type=int, default=200, help="Target file size limit")
    parser.add_argument("--hard-lines", type=int, default=300, help="Hard file size limit")
    parser.add_argument(
        "--allowlist",
        default="tests/checks/policy_allowlist.txt",
        help="Path to policy allowlist file (entries must be mirrored in docs/quality/manifest/deviations.json)",
    )


def _add_event_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--root", default=".", help="Repository root directory")
    parser.add_argument("--event-name", default=os.getenv("GITHUB_EVENT_NAME", ""), help="GitHub event name")
    parser.add_argument("--event-path", default=os.getenv("GITHUB_EVENT_PATH", ""), help="Path to GitHub event payload JSON")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Unified repository quality checks.")
    subparsers = parser.add_subparsers(dest="check", required=True)

    for name in ("ini", "mirror", "launcher", "quality", "test_catalog", "pr_policy", "repo", "python_quality", "all"):
        subparser = subparsers.add_parser(name)
        _add_common_repo_args(subparser, config=name in {"ini", "all"}, build=name in {"launcher", "all"}, with_launcher=name == "all")

    no_legacy = subparsers.add_parser("no_legacy")
    _add_common_repo_args(no_legacy, build=True)

    clang_tidy = subparsers.add_parser("clang_tidy")
    clang_tidy.add_argument("--root", default=".", help="Repository root")
    clang_tidy.add_argument("--build-dir", required=True, help="Build directory containing compile_commands.json")
    clang_tidy.add_argument("--clang-tidy", default="clang-tidy", help="clang-tidy executable")
    clang_tidy.add_argument("--checks", default="-*,clang-analyzer-*,bugprone-unused-return-value", help="clang-tidy checks expression")
    clang_tidy.add_argument("--path", dest="paths", action="append", default=[], help="Restrict analysis to this repo-relative path")

    pr_policy_gate = subparsers.add_parser("pr_policy_gate")
    _add_event_args(pr_policy_gate)
    pr_policy_gate.add_argument("--branch", default="", help="Override PR branch name")
    pr_policy_gate.add_argument("--title", default="", help="Override PR title")
    pr_policy_gate.add_argument("--body", default="", help="Override PR body")

    traceability_gate = subparsers.add_parser("traceability_gate")
    _add_event_args(traceability_gate)
    traceability_gate.add_argument("--repo", default=os.getenv("GITHUB_REPOSITORY", ""), help="GitHub owner/repo slug")
    traceability_gate.add_argument("--token", default=os.getenv("GITHUB_TOKEN", ""), help="GitHub token")

    ivv_gate = subparsers.add_parser("ivv_gate")
    _add_event_args(ivv_gate)
    ivv_gate.add_argument("--repo", default=os.getenv("GITHUB_REPOSITORY", ""), help="GitHub owner/repo slug")
    ivv_gate.add_argument("--token", default=os.getenv("GITHUB_TOKEN", ""), help="GitHub token")

    main_delivery = subparsers.add_parser("main_delivery_gate")
    _add_event_args(main_delivery)
    main_delivery.add_argument("--repo", default=os.getenv("GITHUB_REPOSITORY", ""), help="GitHub owner/repo slug")
    main_delivery.add_argument("--token", default=os.getenv("GITHUB_TOKEN", ""), help="GitHub token")
    main_delivery.add_argument("--sha", default=os.getenv("GITHUB_SHA", ""), help="Commit SHA to validate")
    return parser


def build_context(args: argparse.Namespace) -> CheckContext:
    root = Path(args.root).resolve()
    allowlist = Path(args.allowlist).resolve() if hasattr(args, "allowlist") and Path(args.allowlist).is_absolute() else (root / getattr(args, "allowlist", "tests/checks/policy_allowlist.txt")).resolve()
    config_path = getattr(args, "config", "")
    config = None if not config_path else (Path(config_path).resolve() if Path(config_path).is_absolute() else (root / config_path).resolve())
    build_dir_value = getattr(args, "build_dir", "")
    build_dir = Path(build_dir_value).resolve() if build_dir_value else None
    options = {}
    for key in ("repo", "token", "sha"):
        value = getattr(args, key, "")
        if value:
            options[key] = value
    return CheckContext(
        root=root,
        config=config,
        build_dir=build_dir,
        allowlist=allowlist if hasattr(args, "allowlist") else None,
        check_build_targets=getattr(args, "check_build_targets", False),
        with_launcher=getattr(args, "with_launcher", False),
        target_lines=getattr(args, "target_lines", 200),
        hard_lines=getattr(args, "hard_lines", 300),
        event_name=getattr(args, "event_name", ""),
        event_path=getattr(args, "event_path", ""),
        branch=getattr(args, "branch", ""),
        title=getattr(args, "title", ""),
        body=getattr(args, "body", ""),
        clang_tidy_binary=getattr(args, "clang_tidy", "clang-tidy"),
        clang_tidy_checks=getattr(args, "checks", "-*,clang-analyzer-*,bugprone-unused-return-value"),
        paths=tuple(getattr(args, "paths", [])),
        options=options,
    )


def build_runner() -> CheckRunner:
    return CheckRunner(registry=CHECKS)


def main() -> int:
    args = build_parser().parse_args()
    context = build_context(args)
    if args.check == "all":
        return 0 if build_runner().run(args.check, context) else 1
    check = CHECKS[args.check]()
    return 0 if ResultReporter().emit(check.run(context)) else 1


if __name__ == "__main__":
    raise SystemExit(main())
