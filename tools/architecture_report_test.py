import json
import tempfile
import unittest
from pathlib import Path

from architecture_report import FileMetric, FunctionMetric, analyze_file, validate_reviews


class ArchitectureReportTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = Path(self.temp_dir.name)
        (self.root / "src").mkdir()
        (self.root / "plan").mkdir()

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def write_source(self, text: str) -> Path:
        path = self.root / "src" / "Example.cpp"
        path.write_text(text, encoding="utf-8")
        return path

    def test_line_count_is_informational(self) -> None:
        path = self.write_source("int Value() { return 0; }\n" + "\n" * 200)
        metric = analyze_file(self.root, path, {
            "max_parameters": 4,
            "max_function_lines": 80,
            "max_functions_per_file": 12,
            "max_branch_points": 12,
            "max_allocation_sites": 8,
            "max_internal_includes": 12,
        }, set())

        self.assertGreater(metric.line_count, 80)
        self.assertEqual(metric.signals, ())

    def test_complexity_signals_require_review(self) -> None:
        path = self.write_source(
            "#include \"core/One.hpp\"\n"
            "#include \"core/Two.hpp\"\n"
            "int Example(int value) {\n"
            "    if (value && value > 0) {\n"
            "        std::vector<int> values;\n"
            "        values.reserve(1);\n"
            "        values.push_back(value);\n"
            "    }\n"
            "    return value;\n"
            "}\n"
        )
        metric = analyze_file(self.root, path, {
            "max_parameters": 4,
            "max_function_lines": 2,
            "max_functions_per_file": 0,
            "max_branch_points": 0,
            "max_allocation_sites": 1,
            "max_internal_includes": 1,
        }, set())

        self.assertEqual(
            metric.signals,
            ("allocation", "branching", "function_count", "function_length",
             "include_dependencies"),
        )

    def test_documented_exception_does_not_create_parameter_signal(self) -> None:
        path = self.write_source(
            "int Example(int one, int two, int three, int four, int five) {\n"
            "    return one + two + three + four + five;\n"
            "}\n"
        )
        metric = analyze_file(self.root, path, {
            "max_parameters": 4,
            "max_function_lines": 80,
            "max_functions_per_file": 12,
            "max_branch_points": 12,
            "max_allocation_sites": 8,
            "max_internal_includes": 12,
        }, {("src/Example.cpp", "Example")})

        self.assertNotIn("parameter_count", metric.signals)

    def test_review_registry_must_match_report(self) -> None:
        metric = FileMetric(
            path="src/Example.cpp",
            line_count=10,
            include_count=1,
            internal_include_count=1,
            allocation_sites=0,
            functions=(FunctionMetric("Example", 1, 1, 3, 0, 0),),
            signals=("function_length",),
        )
        (self.root / "plan" / "architecture_reviews.json").write_text(
            json.dumps({"reviews": [{
                "path": "src/Example.cpp",
                "signals": ["function_length"],
                "issue": 575,
                "status": "accepted",
                "decision": "Keep the fixture responsibility intact.",
            }]}),
            encoding="utf-8",
        )

        validate_reviews(self.root, [metric])

        (self.root / "plan" / "architecture_reviews.json").write_text(
            json.dumps({"reviews": []}), encoding="utf-8"
        )
        with self.assertRaisesRegex(ValueError, "missing"):
            validate_reviews(self.root, [metric])


if __name__ == "__main__":
    unittest.main()
