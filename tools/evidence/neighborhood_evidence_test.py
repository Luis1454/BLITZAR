import copy
import pathlib
import unittest

from tools.evidence.neighborhood_contract import (
    load_contract,
    parse_record,
    validate_contract,
    validate_records,
)
from tools.evidence.neighborhood_evidence import external_output


class NeighborhoodEvidenceTests(unittest.TestCase):
    def test_repository_contract_is_valid(self) -> None:
        root = pathlib.Path(__file__).resolve().parents[2]
        self.assertEqual(validate_contract(load_contract(root)), [])

    def test_parser_converts_neighbor_record(self) -> None:
        line = (
            "BLITZAR NEIGHBOR schema=1 seed=424242 scenario=dense candidate=cell-linked-v1 "
            "particles=96 steps=6 radius=0.75 skin=0.4 build_ns=10 query_ns=20 total_ns=30 "
            "rebuild_count=1 neighbor_count=5 reference_count=5 memory_bytes=100 "
            "candidate_hash=1 reference_hash=1 ordering_hash=2 octree_build_ns=10 "
            "octree_cells=3 octree_memory_bytes=1000 octree_hash=4 finite=1 correct=1 "
            "repeatable=1 selected=1"
        )
        record = parse_record(line)
        self.assertIsNotNone(record)
        self.assertEqual(record["particles"], 96)
        self.assertEqual(record["candidate"], "cell-linked-v1")
        self.assertTrue(record["correct"])

    def test_matrix_requires_all_records(self) -> None:
        root = pathlib.Path(__file__).resolve().parents[2]
        self.assertTrue(validate_records([], load_contract(root)))

    def test_contract_rejects_an_unselected_candidate(self) -> None:
        root = pathlib.Path(__file__).resolve().parents[2]
        contract = copy.deepcopy(load_contract(root))
        contract["selection"]["dense"] = "verlet-list-v1"
        self.assertTrue(validate_contract(contract))

    def test_evidence_output_must_be_external(self) -> None:
        root = pathlib.Path("C:/repo").resolve()
        self.assertTrue(external_output(root, root.parent / "evidence"))
        self.assertFalse(external_output(root, root / "evidence"))


if __name__ == "__main__":
    unittest.main()
