"""Test the HDF5 evidence contract and record validation."""

from __future__ import annotations

import pathlib
import unittest

from tools.evidence.hdf5_evidence import load_json, validate_contract, validate_record


class Hdf5EvidenceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.root = pathlib.Path(__file__).resolve().parents[2]
        self.contract = load_json(self.root / "plan" / "hdf5.json")

    def test_repository_contract_is_valid(self) -> None:
        self.assertEqual(validate_contract(self.root), [])

    def test_enabled_record_is_valid(self) -> None:
        record = {
            "backend": "enabled",
            "schema": "1",
            "soa_fields": "8",
            "repeat_writes": "32",
            "transactional_read": "1",
            "file_bytes": "7400",
            "staging_bytes": "128",
            "elapsed_us": "100",
        }

        self.assertEqual(validate_record(record, self.contract), [])

    def test_unavailable_record_has_no_file(self) -> None:
        record = {
            "backend": "unavailable",
            "schema": "1",
            "soa_fields": "8",
            "repeat_writes": "32",
            "transactional_read": "1",
            "file_bytes": "0",
            "staging_bytes": "128",
            "elapsed_us": "0",
        }

        self.assertEqual(validate_record(record, self.contract), [])

    def test_enabled_record_rejects_empty_file(self) -> None:
        record = {
            "backend": "enabled",
            "schema": "1",
            "soa_fields": "8",
            "repeat_writes": "32",
            "transactional_read": "1",
            "file_bytes": "0",
            "staging_bytes": "128",
            "elapsed_us": "100",
        }

        self.assertIn(
            "enabled HDF5 evidence must publish a non-empty file",
            validate_record(record, self.contract),
        )


if __name__ == "__main__":
    unittest.main()
