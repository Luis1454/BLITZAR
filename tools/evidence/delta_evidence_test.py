"""Test the snapshot delta evidence contract and record validation."""

from __future__ import annotations

import pathlib
import unittest

from tools.evidence.delta_evidence import load_json, validate_contract, validate_record


class DeltaEvidenceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.root = pathlib.Path(__file__).resolve().parents[2]
        self.contract = load_json(self.root / "plan" / "delta.json")

    def record(self) -> dict[str, str]:
        return {
            "backend": "cpu",
            "codec": "xor-rle-v1",
            "scope": "snapshot-transport-payload",
            "raw_bytes": "2048",
            "encoded_bytes": "640",
            "ratio_ppm": "312500",
            "reference_write_ns": "100",
            "reference_read_ns": "100",
            "delta_write_ns": "200",
            "delta_read_ns": "200",
            "workspace_bytes": "6144",
            "keyframe_interval": "8",
            "random_access": "index-replay",
            "checksum": "123",
            "deterministic": "1",
            "corruption_rejected": "1",
            "transactional": "1",
            "transport": "1",
            "fallback": "binary-snapshot-codec",
        }

    def test_repository_contract_is_valid(self) -> None:
        self.assertEqual(validate_contract(self.root), [])

    def test_qualified_record_is_valid(self) -> None:
        self.assertEqual(validate_record(self.record(), self.contract), [])

    def test_record_rejects_inconsistent_ratio(self) -> None:
        record = self.record()
        record["ratio_ppm"] = "1"
        self.assertIn("delta evidence ratio is inconsistent", validate_record(record, self.contract))

    def test_record_rejects_non_transactional_decode(self) -> None:
        record = self.record()
        record["transactional"] = "0"
        self.assertIn(
            "delta evidence does not prove transactional transport reuse",
            validate_record(record, self.contract),
        )


if __name__ == "__main__":
    unittest.main()
