"""Test the Grid, PM, and TreePM qualification contract."""

from __future__ import annotations

import copy
import pathlib
import unittest

from tools.evidence.pm_contract import load_contract, validate_contract
from tools.evidence.pm_evidence import external_output


class PmEvidenceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.root = pathlib.Path(__file__).resolve().parents[2]
        self.contract = load_contract(self.root)

    def test_repository_contract_is_valid(self) -> None:
        self.assertEqual(validate_contract(self.contract), [])

    def test_grid_dimensions_are_frozen(self) -> None:
        contract = copy.deepcopy(self.contract)
        contract["grid"]["dimensions"] = [16, 16, 16]
        self.assertTrue(validate_contract(contract))

    def test_distributed_mesh_requires_a_later_contract(self) -> None:
        contract = copy.deepcopy(self.contract)
        contract["distributed"]["status"] = "implemented"
        self.assertTrue(validate_contract(contract))

    def test_evidence_output_must_be_external(self) -> None:
        root = pathlib.Path("C:/repo").resolve()
        self.assertTrue(external_output(root, root.parent / "evidence"))
        self.assertFalse(external_output(root, root / "evidence"))


if __name__ == "__main__":
    unittest.main()
