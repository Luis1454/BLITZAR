import pathlib
import tempfile
import unittest

from workspace_gate import validate


class WorkspaceGateTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = pathlib.Path(self.temp_dir.name)
        (self.root / ".gitignore").write_text(
            "/build/\n/build-*/\n__pycache__/\n*.py[cod]\n",
            encoding="utf-8",
        )

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def policy(self) -> dict[str, object]:
        return {
            "external_build_root": "../.blitzar-build",
            "ci_build_root": "${RUNNER_TEMP}/blitzar-build",
            "required_gitignore": [
                "/build/",
                "/build-*/",
                "__pycache__/",
                "*.py[cod]",
            ],
            "generated_path_components": ["build", "__pycache__"],
            "generated_file_names": ["CMakeCache.txt"],
            "generated_file_suffixes": [".ninja_log"],
        }

    def test_external_root_and_clean_tracked_tree_pass(self) -> None:
        violations, counts = validate(self.root, self.policy(), [])

        self.assertEqual(violations, [])
        self.assertEqual(counts["in_repository_build_directories"], 0)

    def test_generated_tracked_path_is_rejected(self) -> None:
        violations, _ = validate(
            self.root, self.policy(), ["build/CMakeCache.txt", "src/Model.cpp"]
        )

        self.assertEqual(len(violations), 1)
        self.assertIn("build/CMakeCache.txt", violations[0])

    def test_repository_build_root_is_rejected(self) -> None:
        policy = self.policy()
        policy["external_build_root"] = "build"

        violations, _ = validate(self.root, policy, [])

        self.assertIn("outside the repository", violations[0])


if __name__ == "__main__":
    unittest.main()
