"""Fixtures for encoding and clang-format file discovery policy."""

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from tools.format.format_clang_gate import encoding_violations, source_files


class ClangFormatGateTests(unittest.TestCase):
    def test_rejects_bom_and_non_utf8(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "Bad.cpp"
            path.write_bytes(b"\xef\xbb\xbfint value = \xff;\n")

            violations = encoding_violations(path)

            self.assertTrue(any("BOM" in item for item in violations))
            self.assertTrue(any("invalid UTF-8" in item for item in violations))

    def test_rejects_crlf_and_missing_final_newline(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "Bad.cpp"
            path.write_bytes(b"int first = 0;\r\nint second = 1;")

            violations = encoding_violations(path)

            self.assertTrue(any("non-LF" in item for item in violations))
            self.assertTrue(any("final newline" in item for item in violations))

    def test_source_discovery_excludes_build_trees(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "src").mkdir()
            (root / "build-temp").mkdir()
            (root / "src" / "Good.cpp").write_text("int value = 0;\n", encoding="utf-8")
            (root / "build-temp" / "Generated.cpp").write_text(
                "int value = 0;\n", encoding="utf-8"
            )

            files = source_files(root)

            self.assertEqual([path.relative_to(root).as_posix() for path in files], ["src/Good.cpp"])


if __name__ == "__main__":
    unittest.main()
