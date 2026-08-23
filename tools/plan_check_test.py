"""Deterministic regression tests for the frozen-plan checker."""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).with_name("plan_check.py")


class PlanCheckTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = Path(self.temp_dir.name)
        (self.root / "src").mkdir()
        (self.root / "plan").mkdir()
        (self.root / "PLAN.md").write_text("# Fixture\n", encoding="utf-8")
        (self.root / "src" / "Valid.cpp").write_text(
            "int valid_value = 0;\n", encoding="utf-8"
        )
        (self.root / "CMakeLists.txt").write_text(
            "add_test(NAME TST-P0-001 COMMAND fixture_test)\n",
            encoding="utf-8",
        )
        self.write_manifest()
        self.write_quality()

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def write_manifest(self, roots: list[str] | None = None) -> None:
        manifest = {
            "plan_version": "test",
            "status": "frozen",
            "roots": roots if roots is not None else ["src"],
            "deferred_roots": [],
            "forbidden_references": ["never-present-token"],
            "phases": [{"id": "P0", "name": "contracts", "depends_on": []}],
        }
        (self.root / "plan" / "manifest.json").write_text(
            json.dumps(manifest), encoding="utf-8"
        )

    def write_quality(self, evidence_policy: str = "registration-only") -> None:
        quality = {
            "evidence_policy": evidence_policy,
            "tests": [
                {
                    "id": "TST-P0-001",
                    "name": "fixture",
                    "command": "fixture_test",
                    "phase": "P0",
                }
            ],
        }
        (self.root / "plan" / "quality.json").write_text(
            json.dumps(quality), encoding="utf-8"
        )

    def run_checker(self) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [sys.executable, str(SCRIPT), "--root", str(self.root)],
            check=False,
            capture_output=True,
            text=True,
        )

    def test_valid_fixture(self) -> None:
        result = self.run_checker()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("runtime evidence is not claimed", result.stdout)

    def test_rejects_empty_path_component(self) -> None:
        self.write_manifest(["src//broken"])
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("unsafe path", result.stderr)

    def test_rejects_missing_materialized_root(self) -> None:
        self.write_manifest(["missing"])
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("materialized root is missing", result.stderr)

    def test_rejects_materialized_deferred_root(self) -> None:
        deferred = self.root / "future"
        deferred.mkdir()
        manifest_path = self.root / "plan" / "manifest.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        manifest["deferred_roots"] = ["future"]
        manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("deferred root is materialized", result.stderr)

    def test_rejects_noncontiguous_phase_ids(self) -> None:
        manifest_path = self.root / "plan" / "manifest.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        manifest["phases"] = [
            {"id": "P1", "name": "wrong-order", "depends_on": []}
        ]
        manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("phase identifiers", result.stderr)

    def test_rejects_runtime_evidence_claim(self) -> None:
        self.write_quality("executed")
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("evidence_policy", result.stderr)

    def test_rejects_duplicate_quality_id(self) -> None:
        quality_path = self.root / "plan" / "quality.json"
        quality = json.loads(quality_path.read_text(encoding="utf-8"))
        quality["tests"].append(quality["tests"][0])
        quality_path.write_text(json.dumps(quality), encoding="utf-8")
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("invalid or duplicate quality test ID", result.stderr)

    def test_rejects_ctest_mismatch(self) -> None:
        (self.root / "CMakeLists.txt").write_text(
            "add_test(NAME TST-P0-002 COMMAND fixture_test)\n",
            encoding="utf-8",
        )
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("CTest manifest mismatch", result.stderr)

    def test_rejects_duplicate_source_filename(self) -> None:
        duplicate_dir = self.root / "src" / "Nested"
        duplicate_dir.mkdir()
        (duplicate_dir / "Valid.cpp").write_text(
            "int duplicate_value = 0;\n", encoding="utf-8"
        )
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("duplicate source filename", result.stderr)

    def test_rejects_forbidden_reference(self) -> None:
        (self.root / "src" / "Valid.cpp").write_text(
            "const char* marker = \"never-present-token\";\n",
            encoding="utf-8",
        )
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("forbidden reference", result.stderr)


if __name__ == "__main__":
    unittest.main()
