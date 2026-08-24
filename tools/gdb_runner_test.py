import unittest
from pathlib import Path

from gdb_runner import build_command


class GdbRunnerTests(unittest.TestCase):
    def test_builds_batch_command_with_child_arguments(self) -> None:
        executable = Path("build") / "test_binary"
        command = build_command(executable, ["migration", "2"])

        self.assertIn("--batch", command)
        self.assertIn("--return-child-result", command)
        python_command = next(item for item in command if item.startswith("python "))
        self.assertIn("gdb.parse_and_eval('$_exitcode')", python_command)
        self.assertIn("gdb.execute('bt full')", python_command)
        self.assertEqual(command[-3:], [str(executable), "migration", "2"])


if __name__ == "__main__":
    unittest.main()
