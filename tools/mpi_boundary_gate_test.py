"""Tests for the native MPI boundary gate."""

from __future__ import annotations

import json
import pathlib
import sys
import tempfile
import unittest


TOOLS = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))

from mpi_boundary_gate import build_report  # noqa: E402


class MpiBoundaryGateTests(unittest.TestCase):
    def write_repository(self, source: str, native: str = "#include <mpi.h>\nMPI_Comm value;\n") -> pathlib.Path:
        directory = pathlib.Path(tempfile.mkdtemp())
        (directory / "plan").mkdir()
        (directory / "src" / "parallel").mkdir(parents=True)
        (directory / "tests").mkdir()
        quality = {
            "mpi_boundary": {
                "native_units": ["src/parallel/MpiNative.cpp", "src/parallel/MpiNativeState.hpp"],
                "test_native_units": ["tests/Mpi.cpp"],
                "scan_roots": ["src", "tests"],
                "suffixes": [".cpp", ".hpp"],
            }
        }
        (directory / "plan" / "quality.json").write_text(
            json.dumps(quality), encoding="utf-8"
        )
        (directory / "src" / "parallel" / "MpiNative.cpp").write_text(native, encoding="utf-8")
        (directory / "src" / "parallel" / "MpiNativeState.hpp").write_text(
            "#include <mpi.h>\n", encoding="utf-8"
        )
        (directory / "tests" / "Mpi.cpp").write_text(
            "#include <mpi.h>\nMPI_Init(nullptr, nullptr);\n", encoding="utf-8"
        )
        (directory / "src" / "parallel" / "Contract.cpp").write_text(source, encoding="utf-8")
        return directory

    def test_accepts_registered_native_units(self) -> None:
        report = build_report(self.write_repository("int Contract() { return 0; }\n"))

        self.assertEqual(report["violations"], [])

    def test_rejects_mpi_symbol_outside_native_units(self) -> None:
        report = build_report(self.write_repository("#include <mpi.h>\nMPI_Request request;\n"))

        self.assertEqual(len(report["violations"]), 2)
        self.assertIn("escaped the native boundary", report["violations"][0]["message"])

    def test_rejects_missing_native_unit(self) -> None:
        root = self.write_repository("int Contract() { return 0; }\n")
        (root / "src" / "parallel" / "MpiNativeState.hpp").unlink()

        report = build_report(root)

        self.assertEqual(report["violations"][0]["message"], "registered native unit is missing")


if __name__ == "__main__":
    unittest.main()
