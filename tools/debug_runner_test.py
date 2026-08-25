from contextlib import redirect_stderr
from io import StringIO
import unittest
from pathlib import Path

from debug_runner import (
    build_command,
    evaluate_exit_code,
    parse_lldb_exit_status,
    select_backend,
)


class DebugRunnerTests(unittest.TestCase):
    def test_builds_gdb_batch_command_with_child_arguments(self) -> None:
        executable = Path("build") / "test_binary"
        command = build_command(executable, ["migration", "2"], "gdb", "gdb")

        self.assertIn("--batch", command)
        self.assertIn("--return-child-result", command)
        python_command = next(item for item in command if item.startswith("python "))
        self.assertIn("gdb.selected_inferior()", python_command)
        self.assertIn("inferior.pid > 0", python_command)
        self.assertIn("gdb.execute('bt full')", python_command)
        self.assertEqual(command[-3:], [str(executable), "migration", "2"])

    def test_builds_cdb_batch_command_with_native_crash_trace(self) -> None:
        executable = Path("build") / "test_binary.exe"
        command = build_command(executable, ["migration", "2"], "cdb", "cdb.exe")

        self.assertIn("-G", command)
        self.assertIn("sxe av; g; k; q", command)
        self.assertEqual(command[-3:], [str(executable), "migration", "2"])

    def test_builds_lldb_batch_command_with_crash_trace(self) -> None:
        executable = Path("build") / "test_binary"
        command = build_command(executable, ["migration", "2"], "lldb", "lldb")

        self.assertIn("--batch", command)
        self.assertIn("--one-line-on-crash", command)
        self.assertIn("bt all", command)
        self.assertEqual(command[-4:], ["--", str(executable), "migration", "2"])

    def test_prefers_native_backend_for_each_platform(self) -> None:
        self.assertEqual(
            select_backend("auto", "Linux", {"gdb": "/usr/bin/gdb"}),
            ("gdb", "/usr/bin/gdb"),
        )
        self.assertEqual(
            select_backend("auto", "Windows", {"cdb": "C:/cdb.exe", "gdb": "C:/gdb.exe"}),
            ("cdb", "C:/cdb.exe"),
        )
        self.assertEqual(
            select_backend("auto", "Darwin", {"lldb": "/usr/bin/lldb"}),
            ("lldb", "/usr/bin/lldb"),
        )

    def test_expected_exit_code_turns_matching_probe_into_success(self) -> None:
        executable = Path("build") / "status_probe"

        self.assertEqual(evaluate_exit_code(7, 7, executable), 0)
        errors = StringIO()
        with redirect_stderr(errors):
            self.assertEqual(evaluate_exit_code(0, 7, executable), 1)
        self.assertIn("expected exit code 7", errors.getvalue())

    def test_extracts_lldb_target_exit_status(self) -> None:
        output = "Process 42 exited with status = 7 (0x00000007)"

        self.assertEqual(parse_lldb_exit_status(output), 7)
        self.assertIsNone(parse_lldb_exit_status("Process 42 stopped"))


if __name__ == "__main__":
    unittest.main()
