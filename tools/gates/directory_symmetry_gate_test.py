"""Fixtures for the source/test responsibility-directory symmetry gate."""

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from tools.gates.directory_symmetry_gate import validate


class DirectorySymmetryGateTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = Path(self.temp_dir.name)
        (self.root / "plan").mkdir()
        self.write_policy()

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def write_policy(self) -> None:
        (self.root / "plan" / "quality.json").write_text(
            json.dumps(
                {
                    "directory_symmetry": {
                        "pairs": [
                            {
                                "source": "src/solvers/fmm",
                                "tests": "tests/fmm",
                            }
                        ],
                        "allowed_missing": [
                            {
                                "source": "src/solvers/fmm/kifmm",
                                "tests": "tests/fmm/kifmm",
                                "reason": "variant tests are introduced with the layout migration",
                            }
                        ],
                    }
                }
            ),
            encoding="utf-8",
        )

    def make_roots(self) -> tuple[Path, Path]:
        source = self.root / "src" / "solvers" / "fmm"
        tests = self.root / "tests" / "fmm"
        source.mkdir(parents=True)
        tests.mkdir(parents=True)
        return source, tests

    def test_accepts_explicitly_allowed_missing_variant_directory(self) -> None:
        source, tests = self.make_roots()
        (source / "kifmm").mkdir()

        self.assertEqual(validate(self.root), [])

    def test_rejects_missing_test_variant_directory(self) -> None:
        source, _ = self.make_roots()
        (source / "kifmm").mkdir()
        quality_path = self.root / "plan" / "quality.json"
        quality = json.loads(quality_path.read_text(encoding="utf-8"))
        quality["directory_symmetry"]["allowed_missing"] = []
        quality_path.write_text(json.dumps(quality), encoding="utf-8")

        violations = validate(self.root)

        self.assertTrue(any("missing mirrored test directory" in item for item in violations))

    def test_rejects_extra_test_variant_directory(self) -> None:
        _, tests = self.make_roots()
        (tests / "kifmm").mkdir()

        violations = validate(self.root)

        self.assertTrue(any("extra mirrored test directory" in item for item in violations))

    def test_rejects_stale_missing_directory_allowance(self) -> None:
        source, tests = self.make_roots()
        (source / "kifmm").mkdir()
        (tests / "kifmm").mkdir()

        violations = validate(self.root)

        self.assertTrue(any("stale allowed missing directory" in item for item in violations))

    def test_rejects_missing_allowance_without_reason(self) -> None:
        quality_path = self.root / "plan" / "quality.json"
        quality = json.loads(quality_path.read_text(encoding="utf-8"))
        quality["directory_symmetry"]["allowed_missing"][0]["reason"] = ""
        quality_path.write_text(json.dumps(quality), encoding="utf-8")

        with self.assertRaisesRegex(ValueError, "needs a reason"):
            validate(self.root)


if __name__ == "__main__":
    unittest.main()
