#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.ci.python_quality_gate import PythonQualityGateCheck
from python_tools.core.models import CheckContext
from python_tools.core.runner import CheckRunner
from python_tools.policies.ini_check import IniCheck
from python_tools.policies.launcher_check import LauncherContractCheck
from python_tools.policies.mirror_check import MirrorCheck
from python_tools.policies.no_legacy_check import NoLegacyCheck
from python_tools.policies.pr_policy import PrPolicySelfTestCheck
from python_tools.policies.quality_baseline import QualityBaselineCheck
from python_tools.policies.repo_policy import RepoPolicyCheck
from python_tools.policies.test_catalog import TestCatalogCheck


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Unified repository quality checks.")
    parser.add_argument("check", choices=["ini", "mirror", "no_legacy", "launcher", "quality", "test_catalog", "pr_policy", "repo", "python_quality", "all"])
    parser.add_argument("--root", default=".", help="Repository root directory")
    parser.add_argument("--config", default="simulation.ini", help="Path to simulation.ini")
    parser.add_argument("--build-dir", default="", help="Build directory")
    parser.add_argument("--check-build-targets", action="store_true", help="Validate generated targets")
    parser.add_argument("--with-launcher", action="store_true", help="Include launcher contract in all checks")
    parser.add_argument("--target-lines", type=int, default=200, help="Target file size limit")
    parser.add_argument("--hard-lines", type=int, default=300, help="Hard file size limit")
    parser.add_argument(
        "--allowlist",
        default="tests/checks/policy_allowlist.txt",
        help="Path to policy allowlist file (entries must be mirrored in docs/quality/manifest/deviations.json)",
    )
    return parser.parse_args()


def build_context(args: argparse.Namespace) -> CheckContext:
    root = Path(args.root).resolve()
    allowlist = Path(args.allowlist)
    if not allowlist.is_absolute():
        allowlist = (root / allowlist).resolve()
    config = Path(args.config)
    if not config.is_absolute():
        config = (root / config).resolve()
    build_dir = Path(args.build_dir).resolve() if args.build_dir else None
    return CheckContext(
        root=root,
        config=config,
        build_dir=build_dir,
        allowlist=allowlist,
        check_build_targets=args.check_build_targets,
        with_launcher=args.with_launcher,
        target_lines=args.target_lines,
        hard_lines=args.hard_lines,
    )


def build_runner() -> CheckRunner:
    return CheckRunner(
        registry={
            "ini": IniCheck,
            "mirror": MirrorCheck,
            "no_legacy": NoLegacyCheck,
            "launcher": LauncherContractCheck,
            "quality": QualityBaselineCheck,
            "test_catalog": TestCatalogCheck,
            "pr_policy": PrPolicySelfTestCheck,
            "repo": RepoPolicyCheck,
            "python_quality": PythonQualityGateCheck,
        }
    )


def main() -> int:
    args = parse_args()
    context = build_context(args)
    runner = build_runner()
    return 0 if runner.run(args.check, context) else 1


if __name__ == "__main__":
    raise SystemExit(main())
