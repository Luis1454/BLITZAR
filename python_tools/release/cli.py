#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
from pathlib import Path

from python_tools.release.coverage_dashboard import CoverageDashboardBuilder, CoverageMetrics
from python_tools.release.fmea_status import FmeaStatusSnapshot
from python_tools.release.numerical_validation import NumericalValidationCampaign
from python_tools.release.release_bundle import ReleaseBundlePackager
from python_tools.release.release_evidence_defaults import (
    build_release_lane_activities,
    build_release_lane_analyzers,
    default_ci_context,
)
from python_tools.release.release_evidence_pack import ReleaseEvidencePackager
from python_tools.release.release_quality_index import ReleaseQualityIndexBuilder
from python_tools.release.tool_manifest import ToolManifestCollector


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Unified release/nightly packaging entrypoint.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    tool_manifest = subparsers.add_parser("tool_manifest")
    tool_manifest.add_argument("--output", default="dist/tool-qualification/tool_manifest.json", help="Output manifest path")
    tool_manifest.add_argument("--lane", default="manual", help="CI or review lane name")
    tool_manifest.add_argument("--profile", default="prod", help="Profile recorded in the manifest")

    bundle = subparsers.add_parser("package_bundle")
    bundle.add_argument("--build-dir", default="build", help="Directory containing built binaries")
    bundle.add_argument("--dist-dir", default="dist/release-bundle", help="Output distribution directory")
    bundle.add_argument("--tag", default="", help="Archive tag (defaults to GitHub ref/run)")
    bundle.add_argument("--tool-manifest", default="", help="Optional generated tool manifest to embed in the bundle")

    evidence = subparsers.add_parser("package_evidence")
    evidence.add_argument("--root", default=".", help="Repository root")
    evidence.add_argument("--dist-dir", default="dist/evidence-pack", help="Output directory for evidence pack files")
    evidence.add_argument("--tag", default="", help="Archive tag (defaults to GitHub ref/run)")
    evidence.add_argument("--profile", default="prod", help="Qualification profile recorded in the pack")
    evidence.add_argument("--requirement", action="append", default=[], help="Requirement id to include")
    evidence.add_argument("--evidence-ref", action="append", default=[], help="Additional EVD_* reference to bundle")

    index = subparsers.add_parser("package_quality_index")
    index.add_argument("--root", default=".", help="Repository root")
    index.add_argument("--dist-dir", default="dist/release-quality-index", help="Output directory for index files")
    index.add_argument("--tag", default="", help="Archive tag (defaults to GitHub ref/run)")
    index.add_argument("--profile", default="prod", help="Qualification profile recorded in the index")
    index.add_argument("--requirement", action="append", default=[], help="Requirement id to include")

    fmea = subparsers.add_parser("fmea_status")
    fmea.add_argument("--root", default=".", help="Repository root")
    fmea.add_argument("--dist-dir", default="dist/fmea-status", help="Output directory for FMEA status files")

    numerical = subparsers.add_parser("numerical_validation")
    numerical.add_argument("--root", default=".", help="Repository root")
    numerical.add_argument("--dist-dir", default="dist/numerical-validation", help="Output directory for the report bundle")
    numerical.add_argument("--profile", default="gpu-prod", help="Campaign profile from docs/quality/manifest/numerical_campaign.json")
    numerical.add_argument("--tool", required=True, help="Path to gravityNumericalValidationTool executable")

    dashboard = subparsers.add_parser("coverage_dashboard")
    dashboard.add_argument("--root", default="coverage-dashboard", help="Coverage dashboard output directory")
    return parser


def _read_metric(name: str) -> float:
    return float(os.environ[name])


def main() -> int:
    args = build_parser().parse_args()
    if args.command == "tool_manifest":
        collector = ToolManifestCollector()
        output = collector.write(collector.collect(lane=args.lane.strip() or "manual", profile=args.profile.strip() or "prod"), Path(args.output))
        print(output.as_posix())
        return 0
    if args.command == "package_bundle":
        bundle_packager = ReleaseBundlePackager()
        tool_manifest = Path(args.tool_manifest) if args.tool_manifest.strip() else None
        archive = bundle_packager.package(Path(args.build_dir), Path(args.dist_dir), bundle_packager.resolve_tag(args.tag), tool_manifest)
        print(archive.as_posix())
        return 0
    if args.command == "package_evidence":
        evidence_packager = ReleaseEvidencePackager()
        ci_context = default_ci_context()
        ci_context["source"] = "release-lane"
        archive = evidence_packager.package(
            root=Path(args.root),
            dist_dir=Path(args.dist_dir),
            tag=evidence_packager.resolve_tag(args.tag),
            profile=args.profile.strip() or "prod",
            requirements=args.requirement or None,
            verification_activities=build_release_lane_activities(args.profile.strip() or "prod"),
            analyzer_status=build_release_lane_analyzers(),
            ci_context=ci_context,
            extra_evidence_refs=args.evidence_ref,
        )
        print(archive.as_posix())
        return 0
    if args.command == "package_quality_index":
        builder = ReleaseQualityIndexBuilder()
        ci_context = default_ci_context()
        ci_context["source"] = "release-lane"
        archive = builder.package(
            root=Path(args.root),
            dist_dir=Path(args.dist_dir),
            tag=builder.resolve_tag(args.tag),
            profile=args.profile.strip() or "prod",
            requirements=args.requirement or None,
            verification_activities=build_release_lane_activities(args.profile.strip() or "prod"),
            analyzer_status=build_release_lane_analyzers(),
            ci_context=ci_context,
        )
        print(archive.as_posix())
        return 0
    if args.command == "fmea_status":
        archive = FmeaStatusSnapshot().package(root=Path(args.root), dist_dir=Path(args.dist_dir))
        print(archive.as_posix())
        return 0
    if args.command == "numerical_validation":
        archive, report = NumericalValidationCampaign().run(
            root=Path(args.root),
            dist_dir=Path(args.dist_dir),
            profile=args.profile.strip() or "gpu-prod",
            tool_path=Path(args.tool),
        )
        print(archive.as_posix())
        return 0 if report.get("status") == "passed" else 1
    CoverageDashboardBuilder().build(
        Path(args.root),
        CoverageMetrics(lines=_read_metric("LINES_PCT"), functions=_read_metric("FUNCS_PCT"), branches=_read_metric("BRANCH_PCT")),
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
