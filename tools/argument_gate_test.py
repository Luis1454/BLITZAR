import pathlib
import unittest

from argument_gate import parameter_count, scan_source


class ArgumentGateTests(unittest.TestCase):
    def test_counts_nested_types_and_defaults(self):
        self.assertEqual(
            parameter_count("std::array<int, 2> values, Callback callback = [](int) {}"),
            2,
        )

    def test_finds_multiline_definition(self):
        source = """
        blitzar_status Example::Run(
            int first,
            int second,
            int third,
            int fourth,
            int fifth) noexcept
        {
            return 0;
        }
        """
        findings = scan_source(source, "src/Example.cpp", 4)
        self.assertEqual([(item.name, item.parameters) for item in findings], [("Example::Run", 5)])

    def test_ignores_calls_and_standard_library(self):
        source = """
        void Example::Run() {
            return std::merge(a, b, c, d, e, f);
        }
        """
        self.assertEqual(scan_source(source, "src/Example.cpp", 4), [])

    def test_repository_exceptions_are_structured(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        text = (root / "plan" / "parameter_exceptions.json").read_text(encoding="utf-8")
        self.assertIn('"issue": 573', text)
        self.assertIn('"issue": 574', text)


if __name__ == "__main__":
    unittest.main()
