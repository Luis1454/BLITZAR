"""Test the execution-policy and restart-state contract validator."""

from __future__ import annotations

import copy
import pathlib
import unittest

from tools.evidence.reproducibility_contract import (
    load_contract,
    validate_contract,
    validate_plan_version,
)


class ReproducibilityContractTests(unittest.TestCase):
    def setUp(self) -> None:
        self.root = pathlib.Path(__file__).resolve().parents[2]
        self.contract = load_contract(self.root)

    def test_repository_contract_is_valid(self) -> None:
        self.assertEqual(validate_contract(self.contract), [])
        self.assertEqual(validate_plan_version(self.root, self.contract), [])

    def test_fast_mode_is_not_bitwise_reproducible(self) -> None:
        self.assertFalse(self.contract["modes"]["fast"]["bitwise_reproducible"])

    def test_contract_rejects_a_missing_restart_field(self) -> None:
        contract = copy.deepcopy(self.contract)
        contract["restart_state"].pop("compiler")
        self.assertTrue(validate_contract(contract))

    def test_contract_rejects_a_plan_version_drift(self) -> None:
        contract = copy.deepcopy(self.contract)
        contract["plan_version"] = "0.0.0"
        self.assertTrue(validate_plan_version(self.root, contract))


if __name__ == "__main__":
    unittest.main()
