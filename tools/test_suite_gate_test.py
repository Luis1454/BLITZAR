"""Tests for the test-suite responsibility gate."""

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from test_suite_gate import run


class TestSuiteGateTests(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.root = Path(self.directory.name)
        (self.root / "plan").mkdir()
        (self.root / "tests").mkdir()
        (self.root / "tests" / "Test.cpp").write_text("int main() { return 0; }\n", encoding="utf-8")
        quality = {
            "tests": [{"id": "TST-P0-001", "command": "test_target", "phase": "P0"}],
            "architecture": {
                "thresholds": {
                    "max_parameters": 4,
                    "max_function_lines": 80,
                    "max_functions_per_file": 12,
                    "max_branch_points": 12,
                }
            },
        }
        (self.root / "plan" / "quality.json").write_text(
            json.dumps(quality), encoding="utf-8"
        )
        (self.root / "plan" / "test_map.json").write_text(
            json.dumps(
                {
                    "cases": [
                        {
                            "id": "TST-P0-001",
                            "entrypoint": "tests/Test.cpp",
                            "responsibility": "tests/Test.cpp",
                            "mode": "",
                            "ranks": 1,
                            "contract": "test contract",
                        }
                    ]
                }
            ),
            encoding="utf-8",
        )
        (self.root / "plan" / "architecture_reviews.json").write_text(
            json.dumps({"reviews": []}), encoding="utf-8"
        )
        (self.root / "CMakeLists.txt").write_text(
            "add_test(NAME TST-P0-001 COMMAND test_target)\n", encoding="utf-8"
        )

    def tearDown(self) -> None:
        self.directory.cleanup()

    def test_valid_mapping(self) -> None:
        self.assertEqual(run(self.root), [])

    def test_missing_ctest_registration_is_rejected(self) -> None:
        (self.root / "CMakeLists.txt").write_text("", encoding="utf-8")

        self.assertTrue(any("CTest" in error for error in run(self.root)))

    def test_assert_is_rejected(self) -> None:
        (self.root / "tests" / "Test.cpp").write_text(
            "#include <cassert>\nint main() { assert(true); }\n", encoding="utf-8"
        )

        self.assertTrue(any("assert" in error for error in run(self.root)))


if __name__ == "__main__":
    unittest.main()
