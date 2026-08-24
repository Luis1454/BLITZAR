import unittest
from pathlib import Path

from gdb_runner import build_command


class GdbRunnerTests(unittest.TestCase):
    def test_builds_batch_command_with_child_arguments(self) -> None:
        executable = Path("build") / "test_binary"
        command = build_command(executable, ["migration", "2"])

        self.assertIn("--batch", command)
        self.assertIn("--return-child-result", command)
        self.assertEqual(command[-3:], [str(executable), "migration", "2"])


if __name__ == "__main__":
    unittest.main()
