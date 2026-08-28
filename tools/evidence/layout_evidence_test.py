import copy
import pathlib
import unittest

from tools.evidence.layout_contract import (
    load_contract,
    parse_layout_record,
    validate_contract,
    validate_records,
)
from tools.evidence.layout_evidence import external_output


class LayoutEvidenceTests(unittest.TestCase):
    def test_repository_contract_is_valid(self) -> None:
        root = pathlib.Path(__file__).resolve().parents[2]
        contract = load_contract(root)

        self.assertEqual(validate_contract(contract), [])

    def test_layout_record_parser_converts_measurements(self) -> None:
        line = (
            "BLITZAR LAYOUT schema=1 seed=424242 particles=65 "
            "ordering=stable-comparison-v1 layout=soa tile_width=0 sort_ns=12 "
            "materialize_ns=13 tree_build_ns=14 scan_ns=15 "
            "scan_particles_per_second=1.5 locality_mean_squared_distance=0.25 "
            "cache_line_visits_proxy=7 candidate_bytes=3584 materialized_bytes=0 "
            "order_hash=1 state_hash=2 byte_hash=3 tree_hash=4 scan_checksum=5.0 "
            "stable=1 repeatable=1 ordering_equivalent=1 representation_equivalent=1 tree_valid=1"
        )

        record = parse_layout_record(line)

        self.assertIsNotNone(record)
        self.assertEqual(record["particles"], 65)
        self.assertEqual(record["tile_width"], 0)
        self.assertEqual(record["scan_ns"], 15)
        self.assertTrue(record["repeatable"])

    def test_record_matrix_requires_every_candidate(self) -> None:
        root = pathlib.Path(__file__).resolve().parents[2]
        contract = load_contract(root)

        self.assertTrue(validate_records([], contract))

    def test_contract_rejects_unbounded_tile_widths(self) -> None:
        root = pathlib.Path(__file__).resolve().parents[2]
        contract = copy.deepcopy(load_contract(root))
        contract["bounded_tile_widths"] = [4, 8, 16, 64]

        self.assertTrue(validate_contract(contract))

    def test_evidence_output_must_be_external(self) -> None:
        root = pathlib.Path("C:/repo").resolve()

        self.assertTrue(external_output(root, root.parent / "evidence"))
        self.assertFalse(external_output(root, root / "evidence"))


if __name__ == "__main__":
    unittest.main()
