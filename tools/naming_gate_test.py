"""Fixtures for the repository naming gate."""

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from naming_gate import build_report, validate


class NamingGateTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = Path(self.temp_dir.name)
        (self.root / "src").mkdir()
        (self.root / "plan").mkdir()

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def write_source(self, relative: str, content: str = "int Value = 0;\n") -> None:
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")

    def enable_type_mapping(self) -> None:
        (self.root / "plan" / "quality.json").write_text(
            json.dumps({"naming": {}}), encoding="utf-8"
        )

    def test_accepts_cpp_header_pair_and_reports_allowed_stem(self) -> None:
        self.write_source("src/Pair.cpp")
        self.write_source("src/Pair.hpp", "struct Pair final {};\n")

        report = build_report(self.root)

        self.assertEqual(validate(self.root), [])
        self.assertEqual(report["name_report"]["stem_collisions"][0]["status"], "allowed_pair")

    def test_rejects_duplicate_full_filename(self) -> None:
        self.write_source("src/Valid.cpp")
        self.write_source("src/Nested/Valid.cpp")

        failures = validate(self.root)

        self.assertTrue(any("duplicate source filename" in item for item in failures))

    def test_rejects_non_pascal_case_filename(self) -> None:
        self.write_source("src/notPascal.cpp")

        failures = validate(self.root)

        self.assertTrue(any("non-PascalCase" in item for item in failures))

    def test_rejects_disallowed_stem_pair(self) -> None:
        self.write_source("src/Pair.cpp")
        self.write_source("src/Pair.c")

        failures = validate(self.root)

        self.assertTrue(any("duplicate source stem" in item for item in failures))

    def test_enforces_primary_type_and_owner_mapping_when_enabled(self) -> None:
        self.enable_type_mapping()
        self.write_source("src/Only.hpp", "struct Other final {};\n")
        self.write_source("src/Only.cpp")

        failures = validate(self.root)

        self.assertTrue(any("primary type/file mismatch" in item for item in failures))
        self.assertTrue(any("no owner header" in item for item in failures))

    def test_rejects_long_name_forbidden_path_and_redundant_prefix(self) -> None:
        (self.root / "plan" / "quality.json").write_text(
            json.dumps({"naming": {"max_filename_length": {"production": 8}}}),
            encoding="utf-8",
        )
        self.write_source("src/utils/OldLong.hpp", "struct OldLong final {};\n")
        self.write_source(
            "src/utils/OldLong.cpp",
            '#include "utils/OldLong.hpp"\n',
        )

        failures = validate(self.root)

        self.assertTrue(any("category limit" in item for item in failures))
        self.assertTrue(any("forbidden repository path" in item for item in failures))
        self.assertTrue(any("redundant filename prefix" in item for item in failures))


if __name__ == "__main__":
    unittest.main()
