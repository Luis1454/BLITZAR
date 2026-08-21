"""Validate the immutable clean-room plan used by CI."""

from __future__ import annotations

import json
import pathlib
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "plan" / "manifest.json"
QUALITY = ROOT / "plan" / "quality.json"


def fail(message: str) -> None:
    print(f"plan-check: {message}", file=sys.stderr)
    raise SystemExit(1)


def main() -> None:
    if not (ROOT / "PLAN.md").is_file():
        fail("PLAN.md is missing")
    if not MANIFEST.is_file():
        fail("plan/manifest.json is missing")
    if not QUALITY.is_file():
        fail("plan/quality.json is missing")

    try:
        data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        fail(f"manifest is not valid JSON: {error}")

    if data.get("status") != "frozen":
        fail("manifest status must remain 'frozen'")
    if not data.get("plan_version"):
        fail("plan_version is required")

    phases = data.get("phases")
    if not isinstance(phases, list) or not phases:
        fail("at least one phase is required")
    phase_ids = [phase.get("id") for phase in phases]
    expected_ids = [f"P{index}" for index in range(len(phases))]
    if phase_ids != expected_ids:
        fail("phase identifiers must be contiguous and ordered")
    if any(not phase.get("name") for phase in phases):
        fail("every phase needs a name")

    roots = data.get("roots")
    if not isinstance(roots, list) or len(roots) != len(set(roots)):
        fail("roots must be a non-empty list of unique paths")

    forbidden = data.get("forbidden_references")
    if not isinstance(forbidden, list) or not forbidden:
        fail("forbidden_references must be declared")
    code_roots = [
        ROOT / "src",
        ROOT / "include",
        ROOT / "apps",
        ROOT / "tests",
        ROOT / "examples",
    ]
    code_files = [
        path
        for directory in code_roots
        if directory.is_dir()
        for path in directory.rglob("*")
        if path.is_file()
    ]
    for path in code_files:
        text = path.read_text(encoding="utf-8", errors="ignore").lower()
        for reference in forbidden:
            if str(reference).lower() in text:
                fail(f"{path.relative_to(ROOT)} contains forbidden reference: {reference}")

    try:
        quality = json.loads(QUALITY.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        fail(f"quality manifest is not valid JSON: {error}")
    tests = quality.get("tests")
    test_ids = [test.get("id") for test in tests] if isinstance(tests, list) else []
    if not tests or any(not test_id for test_id in test_ids):
        fail("quality manifest needs named tests with IDs")
    if len(test_ids) != len(set(test_ids)):
        fail("quality test IDs must be unique")

    print(f"plan-check: frozen plan {data['plan_version']} is valid")


if __name__ == "__main__":
    main()
