import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from tools.gates.capability_gate import STATES, validate_snapshot_description, validate_state


class CapabilityGateTests(unittest.TestCase):
    def test_known_states_are_accepted(self) -> None:
        for state in STATES:
            self.assertEqual(validate_state(state), [])

    def test_unknown_state_is_rejected(self) -> None:
        errors = validate_state("runtime-proven")

        self.assertEqual(errors, ["unknown capability state: runtime-proven"])

    def test_qualified_snapshot_contract_is_accepted(self) -> None:
        plan = "SnapshotFrameView is implemented and contract-qualified. "
        plan += "Binary or HDF5 persistence is not implemented."

        self.assertEqual(validate_snapshot_description(plan), [])

    def test_unqualified_snapshot_contract_is_rejected(self) -> None:
        errors = validate_snapshot_description("SnapshotHeader remains a contract hook only")

        self.assertEqual(
            errors,
            [
                "PLAN.md must describe the qualified SnapshotFrameView contract",
                "PLAN.md must keep binary and HDF5 persistence deferred",
            ],
        )


if __name__ == "__main__":
    unittest.main()
