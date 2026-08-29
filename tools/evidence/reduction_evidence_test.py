import copy
import pathlib
import unittest

from tools.evidence.reduction_contract import (
    load_contract,
    parse_record,
    validate_contract,
    validate_records,
)
from tools.evidence.reduction_evidence import external_output


class ReductionEvidenceTests(unittest.TestCase):
    def test_repository_contract_is_valid(self) -> None:
        root = pathlib.Path(__file__).resolve().parents[2]
        self.assertEqual(validate_contract(load_contract(root)), [])

    def test_parser_converts_reduction_and_long_run_records(self) -> None:
        reduction = parse_record(
            "BLITZAR REDUCTION schema=1 seed=424242 workload=force terms=65536 "
            "policy=plain-v1 elapsed_ns=12 terms_per_second=1.5 expected=1 value=0 "
            "absolute_error=1 relative_error=1 input_hash=1 value_hash=2 "
            "repeatable=1 finite=1 vectorization_eligible=1 selected=1"
        )
        long_run = parse_record(
            "BLITZAR REDUCTION_LONG schema=1 seed=424242 steps=4096 policy=neumaier-v1 "
            "max_relative_energy_error=0.0 final_energy=1 final_momentum_norm=0 "
            "state_hash=3 finite=1 default_policy_match=1 selected=1"
        )
        self.assertEqual(reduction[0], "reduction")
        self.assertEqual(reduction[1]["terms"], 65536)
        self.assertEqual(long_run[0], "long")
        self.assertTrue(long_run[1]["default_policy_match"])

    def test_matrix_requires_all_records(self) -> None:
        root = pathlib.Path(__file__).resolve().parents[2]
        contract = load_contract(root)
        self.assertTrue(validate_records([], [], contract))

    def test_contract_rejects_a_changed_selection(self) -> None:
        root = pathlib.Path(__file__).resolve().parents[2]
        contract = copy.deepcopy(load_contract(root))
        contract["selection"]["force"] = "neumaier-v1"
        self.assertTrue(validate_contract(contract))

    def test_evidence_output_must_be_external(self) -> None:
        root = pathlib.Path("C:/repo").resolve()
        self.assertTrue(external_output(root, root.parent / "evidence"))
        self.assertFalse(external_output(root, root / "evidence"))


if __name__ == "__main__":
    unittest.main()
