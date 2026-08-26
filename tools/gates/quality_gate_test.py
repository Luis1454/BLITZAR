import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from tools.gates.quality_gate import command_arguments, select_checks


class QualityGateTests(unittest.TestCase):
    def test_default_group_is_static(self) -> None:
        checks = [
            {"id": "CHK-1", "command": "python first.py"},
            {"id": "CHK-2", "command": "cmake --build build", "group": "build"},
        ]

        selected = select_checks(checks, "static")

        self.assertEqual([item["id"] for item in selected], ["CHK-1"])

    def test_explicit_group_is_selected(self) -> None:
        checks = [
            {"id": "CHK-1", "command": "python first.py", "group": "build"},
            {"id": "CHK-2", "command": "python second.py", "group": "static"},
        ]

        selected = select_checks(checks, "build")

        self.assertEqual([item["id"] for item in selected], ["CHK-1"])

    def test_python_command_uses_active_interpreter(self) -> None:
        arguments = command_arguments("python -B tools/example.py")

        self.assertEqual(arguments[0], sys.executable)
        self.assertEqual(arguments[1:], ["-B", "tools/example.py"])

    def test_empty_group_is_rejected(self) -> None:
        with self.assertRaisesRegex(ValueError, "empty"):
            select_checks([], "static")


if __name__ == "__main__":
    unittest.main()
