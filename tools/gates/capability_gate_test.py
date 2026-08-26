import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from tools.gates.capability_gate import STATES, validate_state


class CapabilityGateTests(unittest.TestCase):
    def test_known_states_are_accepted(self) -> None:
        for state in STATES:
            self.assertEqual(validate_state(state), [])

    def test_unknown_state_is_rejected(self) -> None:
        errors = validate_state("runtime-proven")

        self.assertEqual(errors, ["unknown capability state: runtime-proven"])


if __name__ == "__main__":
    unittest.main()
