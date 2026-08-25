"""Report bounded architecture signals for one or every repository profile."""

from __future__ import annotations

import argparse
import json
import pathlib
import sys

from argument_gate import load_exceptions
from architecture_metrics import (
    FileMetric,
    FunctionMetric,
    dependency_names,
    scan_file_functions,
    internal_dependency_count,
)
from architecture_scope import (
    SOURCE_ROOTS,
    classify_path,
    discover_files,
    discover_repository_paths,
    load_profiles,
    load_thresholds,
)
from architecture_sources import source_completeness_report


def analyze_file(
    root: pathlib.Path,
    path: pathlib.Path,
    thresholds: dict[str, int],
    exceptions: set[tuple[str, str]],
    profile: str = "production",
) -> FileMetric:
    source = path.read_text(encoding="utf-8", errors="replace")
    relative = path.relative_to(root).as_posix()
    functions = scan_file_functions(source, relative)
    includes = dependency_names(source, path)
    internal_includes = internal_dependency_count(root, path, includes)
    signals: set[str] = set()

    if len(functions) > thresholds["max_functions_per_file"]:
        signals.add("function_count")
    if sum(item.allocation_sites for item in functions) > thresholds["max_allocation_sites"]:
        signals.add("allocation")
    if internal_includes > thresholds["max_internal_includes"]:
        signals.add("include_dependencies")
    if any(item.body_lines > thresholds["max_function_lines"] for item in functions):
        signals.add("function_length")
    if any(item.branch_points > thresholds["max_branch_points"] for item in functions):
        signals.add("branching")
    if any(
        item.parameters > thresholds["max_parameters"]
        and (relative, item.name) not in exceptions
        for item in functions
    ):
        signals.add("parameter_count")

    return FileMetric(
        path=relative,
        line_count=len(source.splitlines()),
        include_count=len(includes),
        internal_include_count=internal_includes,
        allocation_sites=sum(item.allocation_sites for item in functions),
        functions=functions,
        signals=tuple(sorted(signals)),
        profile=profile,
        language=path.suffix.lower().lstrip(".") or "metadata",
    )


def load_reviews(root: pathlib.Path) -> dict[str, set[str]]:
    path = root / "plan" / "architecture_reviews.json"
    data = json.loads(path.read_text(encoding="utf-8"))
    reviews = data.get("reviews")
    if not isinstance(reviews, list):
        raise ValueError("architecture review registry needs a reviews list")
    result: dict[str, set[str]] = {}
    for review in reviews:
        if not isinstance(review, dict):
            raise ValueError("architecture review entries must be objects")
        review_path = review.get("path")
        signals = review.get("signals")
        if (
            not isinstance(review_path, str)
            or not isinstance(signals, list)
            or not isinstance(review.get("issue"), int)
            or review.get("status") != "accepted"
            or not isinstance(review.get("decision"), str)
            or not review["decision"]
        ):
            raise ValueError(
                "architecture review entries need path, signals, issue, accepted status, and decision"
            )
        if review_path in result:
            raise ValueError(f"duplicate architecture review: {review_path}")
        result[review_path] = {str(signal) for signal in signals}
    return result


def validate_reviews(root: pathlib.Path, files: list[FileMetric]) -> None:
    reviews = load_reviews(root)
    required = {file.path: set(file.signals) for file in files if file.signals}
    if set(reviews) != set(required):
        missing = sorted(set(required) - set(reviews))
        stale = sorted(set(reviews) - set(required))
        raise ValueError(f"architecture review mismatch; missing={missing}, stale={stale}")
    for path, signals in required.items():
        if reviews[path] != signals:
            raise ValueError(
                f"architecture review signals mismatch for {path}: "
                f"report={sorted(signals)}, review={sorted(reviews[path])}"
            )


def build_repository_report(root: pathlib.Path) -> dict[str, object]:
    profiles = load_profiles(root)
    profile_by_name = {profile.name: profile for profile in profiles}
    exceptions = load_exceptions(root)
    repository_paths = discover_repository_paths(root)
    files: list[FileMetric] = []
    unclassified: list[str] = []
    for path in repository_paths:
        relative = path.relative_to(root).as_posix()
        profile_name = classify_path(relative, profiles)
        if profile_name is None:
            unclassified.append(relative)
            continue
        profile = profile_by_name[profile_name]
        files.append(analyze_file(root, path, profile.thresholds, exceptions, profile_name))

    profile_summary = {
        profile.name: {
            "roots": list(profile.roots),
            "paths": list(profile.paths),
            "suffixes": list(profile.suffixes),
            "thresholds": profile.thresholds,
            "file_count": sum(item.profile == profile.name for item in files),
        }
        for profile in profiles
    }
    return {
        "schema_version": 2,
        "scope": [profile.name for profile in profiles],
        "line_count_policy": "informational",
        "profiles": profile_summary,
        "repository_file_count": len(repository_paths),
        "unclassified_files": sorted(unclassified),
        "source_completeness": source_completeness_report(root),
        "review_required_paths": [file.path for file in files if file.signals],
        "files": [file.as_dict() for file in files],
    }


def build_report(root: pathlib.Path) -> dict[str, object]:
    thresholds = load_thresholds(root)
    exceptions = load_exceptions(root)
    files = [
        analyze_file(root, path, thresholds, exceptions, "production")
        for path in discover_files(root)
    ]
    return {
        "schema_version": 1,
        "scope": list(SOURCE_ROOTS),
        "line_count_policy": "informational",
        "thresholds": thresholds,
        "review_required_paths": [file.path for file in files if file.signals],
        "files": [file.as_dict() for file in files],
    }


def write_report(report: dict[str, object], output: pathlib.Path | None) -> None:
    rendered = json.dumps(report, indent=2) + "\n"
    if output is None:
        print(rendered, end="")
        return
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(rendered, encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=pathlib.Path,
        default=pathlib.Path(__file__).resolve().parents[1],
    )
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--quiet", action="store_true")
    parser.add_argument("--all", action="store_true", help="qualify every repository profile")
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()
    try:
        report = build_repository_report(root) if arguments.all else build_report(root)
        if arguments.check:
            files = [
                FileMetric(
                    path=str(item["path"]),
                    line_count=int(item["line_count"]),
                    include_count=int(item["include_count"]),
                    internal_include_count=int(item["internal_include_count"]),
                    allocation_sites=int(item["allocation_sites"]),
                    functions=tuple(
                        FunctionMetric(**function) for function in item["functions"]
                    ),
                    signals=tuple(str(signal) for signal in item["signals"]),
                    profile=str(item.get("profile", "production")),
                    language=str(item.get("language", "cpp")),
                )
                for item in report["files"]
            ]
            validate_reviews(root, files)
            if arguments.all:
                unclassified = report["unclassified_files"]
                completeness = report["source_completeness"]
                if unclassified:
                    raise ValueError(
                        "repository files are not covered by an architecture profile: "
                        + ", ".join(str(item) for item in unclassified)
                    )
                if completeness["status"] != "ok":
                    raise ValueError(
                        "CMake source completeness failed: "
                        f"missing={completeness['missing']}, stale={completeness['stale']}"
                    )
    except (OSError, ValueError, json.JSONDecodeError, KeyError, TypeError) as error:
        print(f"architecture-report: {error}", file=sys.stderr)
        return 1
    if not arguments.quiet:
        write_report(report, arguments.output)
    print(
        f"architecture-report: {len(report['files'])} files, "
        f"{len(report['review_required_paths'])} responsibility reviews required",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
