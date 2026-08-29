"""Test the block-time qualification contract and record parser."""

from __future__ import annotations

import copy
import pathlib
import unittest

from tools.evidence.block_time_contract import (
    load_contract,
    parse_record,
    validate_contract,
    validate_records,
)
from tools.evidence.block_time_evidence import external_output


def record_line(workload: str) -> str:
    return (
        "BLITZAR BLOCK schema=1 seed=424242 workload="
        f"{workload} particles=32 horizon_ticks=64 sync_interval=8 migration_tick=32 "
        "fixed_events=2048 block_events=960 fixed_elapsed_ns=100 block_elapsed_ns=50 "
        "modeled_speedup=2.1333333333333333 fixed_event_hash=11 block_event_hash=22 "
        "restart_event_hash=22 rollback_event_hash=22 fixed_ownership_hash=33 "
        "block_ownership_hash=33 initial_input_hash=44 final_input_hash=44 "
        "reference=fixed-kdk-v1 candidate=block-kdk-schedule-v1 "
        "speedup_scope=schedule-work-proxy decision=not-selected active_ordered=1 "
        "deterministic=1 ledger_conserved=1 migration=1 restart_compatible=1 "
        "rollback_transactional=1 state_unchanged=1 candidate_selected=0"
    )


class BlockTimeEvidenceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.root = pathlib.Path(__file__).resolve().parents[2]
        self.contract = load_contract(self.root)

    def test_repository_contract_is_valid(self) -> None:
        self.assertEqual(validate_contract(self.contract), [])

    def test_parser_converts_a_qualified_record(self) -> None:
        record = parse_record(record_line("heterogeneous-v1"))
        self.assertIsNotNone(record)
        self.assertEqual(record["particles"], 32)
        self.assertAlmostEqual(record["modeled_speedup"], 2048 / 960)
        self.assertTrue(record["deterministic"])

    def test_matrix_requires_all_workloads(self) -> None:
        records = [parse_record(record_line("heterogeneous-v1"))]
        self.assertTrue(validate_records(records, self.contract))

    def test_candidate_selection_is_rejected(self) -> None:
        record = parse_record(record_line("heterogeneous-v1"))
        record["candidate_selected"] = True
        self.assertTrue(validate_records([record] * 3, self.contract))

    def test_contract_rejects_a_production_block_integrator(self) -> None:
        contract = copy.deepcopy(self.contract)
        contract["decision"]["status"] = "selected"
        self.assertTrue(validate_contract(contract))

    def test_evidence_output_must_be_external(self) -> None:
        root = pathlib.Path("C:/repo").resolve()
        self.assertTrue(external_output(root, root.parent / "evidence"))
        self.assertFalse(external_output(root, root / "evidence"))


if __name__ == "__main__":
    unittest.main()
