"""Tests for the raw-pointer and ownership boundary gate."""

from __future__ import annotations

import json
import pathlib
import tempfile
import unittest

from pointer_ownership_gate import build_report


class PointerOwnershipGateTests(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = pathlib.Path(tempfile.mkdtemp())
        (self.directory / "plan").mkdir()
        (self.directory / "include" / "blitzar").mkdir(parents=True)
        (self.directory / "src" / "sdk").mkdir(parents=True)
        (self.directory / "src" / "solvers" / "gpu").mkdir(parents=True)

    def write_contract(self, allowlist: list[dict[str, str]], allocations: list[str] | None = None) -> None:
        contract = {
            "schema_version": 1,
            "scan_roots": ["include", "src"],
            "suffixes": [".cpp", ".h", ".hpp", ".inl"],
            "allowlist": allowlist,
            "allocation_allowlist": allocations or [],
        }
        (self.directory / "plan" / "pointer_ownership.json").write_text(
            json.dumps(contract), encoding="utf-8"
        )

    @staticmethod
    def boundary(path: str, category: str = "device_view") -> dict[str, str]:
        return {
            "path": path,
            "category": category,
            "ownership": "borrowed",
        }

    def test_accepts_registered_abi_and_device_views(self) -> None:
        self.write_contract([
            self.boundary("include/blitzar/blitzar.h", "public_c_abi"),
            self.boundary("src/solvers/gpu/DirectDevice.inl"),
        ])
        (self.directory / "include" / "blitzar" / "blitzar.h").write_text(
            "const double* values;\n", encoding="utf-8"
        )
        (self.directory / "src" / "solvers" / "gpu" / "DirectDevice.inl").write_text(
            "const double* values;\n", encoding="utf-8"
        )

        report = build_report(self.directory)

        self.assertEqual(report["violations"], [])
        self.assertEqual(len(report["findings"]), 2)

    def test_rejects_unregistered_pointer_and_owner_like_device_field(self) -> None:
        self.write_contract([self.boundary("src/solvers/gpu/DirectDevice.inl")])
        (self.directory / "src" / "sdk" / "Internal.cpp").write_text(
            "int* owner;\n", encoding="utf-8"
        )
        (self.directory / "src" / "solvers" / "gpu" / "DirectDevice.inl").write_text(
            "int* owner;\n", encoding="utf-8"
        )

        report = build_report(self.directory)
        messages = {item["message"] for item in report["violations"]}

        self.assertIn(
            "raw pointer is not registered at an ABI or execution boundary", messages
        )
        self.assertIn("device view uses an owner-like raw pointer name", messages)

    def test_rejects_new_outside_handle_boundary(self) -> None:
        self.write_contract([])
        (self.directory / "src" / "sdk" / "Internal.cpp").write_text(
            "void Create() { auto value = new int; delete value; }\n", encoding="utf-8"
        )

        report = build_report(self.directory)

        self.assertEqual(len(report["violations"]), 2)
        self.assertTrue(all("outside the C ABI" in item["message"] for item in report["violations"]))

    def test_accepts_new_in_handle_boundary(self) -> None:
        self.write_contract([], ["src/sdk/Api.cpp"])
        (self.directory / "src" / "sdk" / "Api.cpp").write_text(
            "void Create() { auto value = new int; delete value; }\n", encoding="utf-8"
        )

        report = build_report(self.directory)

        self.assertEqual(report["violations"], [])
        self.assertEqual(len(report["findings"]), 2)

    def test_accepts_span_without_boundary_registration(self) -> None:
        self.write_contract([])
        (self.directory / "src" / "sdk" / "Internal.cpp").write_text(
            "std::span<double> values;\n", encoding="utf-8"
        )

        report = build_report(self.directory)

        self.assertEqual(report["violations"], [])
        self.assertEqual(report["findings"], [])


if __name__ == "__main__":
    unittest.main()
