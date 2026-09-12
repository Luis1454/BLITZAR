"""Fixtures for the frozen repository taxonomy gate."""

from __future__ import annotations

import copy
import json
import tempfile
import unittest
from pathlib import Path

from tools.gates.repository_tree_gate import validate_materialized, validate_plan, validate_policy


class RepositoryTreeGateTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = Path(self.temp_dir.name)
        (self.root / "plan").mkdir()
        self.write_policy()
        self.write_manifest()

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def policy(self) -> dict[str, object]:
        return {
            "schema_version": 1,
            "plan_version": "1.0.0",
            "status": "frozen",
            "materialization": "planned",
            "aliases": {"metadata": "md", "config": "cfg"},
            "language_separation": [
                {"root": "include/blitzar", "children": ["c", "cpp"]}
            ],
            "source_test_pairs": [
                {
                    "source": "src/solvers",
                    "tests": "tests/solvers",
                    "responsibility": "solver families",
                }
            ],
            "allowed_missing": [],
            "test_suffix": {
                "suffix": "Test",
                "extensions": [".c", ".cpp", ".cmake"],
                "entrypoint_policy": "registered tests end in Test",
                "non_test_entrypoints": ["examples/c/Example.c"],
            },
            "target_test_entrypoints": ["tests/solvers/fmm/FmmTest.cpp"],
            "python": {
                "root": "tools",
                "domains": ["gates"],
                "test_suffix": "_test.py",
                "domain_policy": "group by responsibility",
            },
            "forbidden_paths": [".keep"],
            "migration": {
                "state": "planned",
                "rule": "migrate in one reviewed change",
                "directory_moves": [["src/old", "src/new"]],
            },
        }

    def write_policy(self, policy: dict[str, object] | None = None) -> None:
        value = self.policy() if policy is None else policy
        (self.root / "plan" / "repository_tree.json").write_text(
            json.dumps(value), encoding="utf-8"
        )

    def write_manifest(self, plan_version: str = "1.0.0") -> None:
        (self.root / "plan" / "manifest.json").write_text(
            json.dumps(
                {
                    "plan_version": plan_version,
                    "source_of_truth": ["plan/repository_tree.json"],
                }
            ),
            encoding="utf-8",
        )

    def test_accepts_frozen_plan(self) -> None:
        self.assertEqual(validate_policy(self.root), [])
        self.assertEqual(validate_plan(self.root), [])

    def test_rejects_plan_version_drift(self) -> None:
        self.write_manifest("1.0.1")
        errors = validate_plan(self.root)
        self.assertTrue(any("plan_version" in error for error in errors))

    def test_rejects_duplicate_alias_value(self) -> None:
        policy = self.policy()
        policy["aliases"] = {"metadata": "md", "other": "md"}
        self.write_policy(policy)
        errors = validate_policy(self.root)
        self.assertTrue(any("alias values must be unique" in error for error in errors))

    def test_rejects_non_test_target_entrypoint(self) -> None:
        policy = self.policy()
        policy["target_test_entrypoints"] = ["tests/solvers/fmm/FmmRunner.cpp"]
        self.write_policy(policy)
        errors = validate_policy(self.root)
        self.assertTrue(any("must end in Test" in error for error in errors))

    def test_rejects_allowed_missing_outside_pair(self) -> None:
        policy = self.policy()
        policy["allowed_missing"] = [
            {
                "source": "src/other",
                "tests": "tests/other",
                "reason": "not a solver directory",
            }
        ]
        self.write_policy(policy)
        errors = validate_policy(self.root)
        self.assertTrue(any("outside exactly one" in error for error in errors))

    def test_rejects_duplicate_language_child(self) -> None:
        policy = self.policy()
        policy["language_separation"] = [
            {"root": "include/blitzar", "children": ["c", "c"]}
        ]
        self.write_policy(policy)
        errors = validate_policy(self.root)
        self.assertTrue(any("children must be unique" in error for error in errors))

    def materialized_policy(self) -> dict[str, object]:
        policy = copy.deepcopy(self.policy())
        policy["materialization"] = "materialized"
        policy["migration"]["state"] = "materialized"  # type: ignore[index]
        return policy

    def materialize_valid_tree(self) -> None:
        self.write_policy(self.materialized_policy())
        self.write_manifest()
        for path in (
            "include/blitzar/c",
            "include/blitzar/cpp",
            "src/solvers/fmm",
            "tests/solvers/fmm",
            "tools/gates",
        ):
            (self.root / path).mkdir(parents=True, exist_ok=True)
        (self.root / "include/blitzar/c/Abi.h").write_text("", encoding="utf-8")
        (self.root / "include/blitzar/cpp/Facade.hpp").write_text("", encoding="utf-8")
        (self.root / "tests/solvers/fmm/FmmTest.cpp").write_text(
            "int main() { return 0; }\n", encoding="utf-8"
        )
        (self.root / "tools/gates/tree_gate.py").write_text("", encoding="utf-8")

    def test_accepts_promoted_materialized_tree(self) -> None:
        self.materialize_valid_tree()
        self.assertEqual(validate_materialized(self.root), [])

    def test_rejects_materialization_before_promotion(self) -> None:
        errors = validate_materialized(self.root)
        self.assertTrue(any("materialization is planned" in error for error in errors))

    def test_allows_unregistered_test_support_source_without_suffix(self) -> None:
        self.materialize_valid_tree()
        (self.root / "tests/solvers/fmm/FmmRunner.cpp").write_text(
            "int main() { return 0; }\n", encoding="utf-8"
        )
        self.assertEqual(validate_materialized(self.root), [])

    def test_rejects_placeholder_file_at_any_depth(self) -> None:
        self.materialize_valid_tree()
        (self.root / "include" / ".keep").write_text("", encoding="utf-8")

        errors = validate_materialized(self.root)

        self.assertTrue(any("include/.keep" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
