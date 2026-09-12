"""Deterministic regression tests for the frozen-plan checker."""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]


class PlanCheckTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = Path(self.temp_dir.name)
        (self.root / "src").mkdir()
        (self.root / "plan").mkdir()
        (self.root / "cmake").mkdir()
        (self.root / ".github" / "workflows").mkdir(parents=True)
        (self.root / "PLAN.md").write_text(
            "Product/API version: **1.0.0**\n"
            "Plan version: **1.0.6**\n\n"
            "## Repository Shape\n\n"
            "```text\n"
            "src/                Fixture root\n"
            "```\n",
            encoding="utf-8",
        )
        (self.root / "src" / "Valid.cpp").write_text(
            "int valid_value = 0;\n", encoding="utf-8"
        )
        (self.root / "CMakeLists.txt").write_text(
            "file(READ \"${CMAKE_CURRENT_SOURCE_DIR}/plan/manifest.json\" "
            "BLITZAR_PLAN_MANIFEST)\n"
            "string(JSON BLITZAR_PRODUCT_VERSION GET "
            "\"${BLITZAR_PLAN_MANIFEST}\" product_version)\n"
            "string(JSON BLITZAR_PLAN_VERSION GET "
            "\"${BLITZAR_PLAN_MANIFEST}\" plan_version)\n"
            "project(BLITZAR VERSION ${BLITZAR_PRODUCT_VERSION})\n"
            "add_test(NAME TST-P0-001 COMMAND fixture_test)\n",
            encoding="utf-8",
        )
        (self.root / "cmake" / "blitzar_config.cmake.in").write_text(
            'set(BLITZAR_VERSION "@PROJECT_VERSION@")\n'
            'set(BLITZAR_PRODUCT_VERSION "@PROJECT_VERSION@")\n'
            'set(BLITZAR_PLAN_VERSION "@BLITZAR_PLAN_VERSION@")\n',
            encoding="utf-8",
        )
        (self.root / ".github" / "workflows" / "plan.yml").write_text(
            "changed=$(git diff --name-only)\n"
            "if grep -Eq '^(PLAN.md|plan/.*\\.(json|md))$' <<< \"$changed\"; then\n"
            "  git log --format=%s\n"
            "  grep -q '^plan-change:'\n"
            "  grep -Eq '^plan/decisions/[^/]+\\.md$'\n"
            "fi\n",
            encoding="utf-8",
        )
        self.write_manifest()
        self.write_output_contract()
        self.write_repository_tree()
        (self.root / "plan" / "architecture_reviews.json").write_text(
            json.dumps({"schema_version": 1, "reviews": []}), encoding="utf-8"
        )
        self.write_quality()

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def write_manifest(self, roots: list[str] | None = None) -> None:
        manifest = {
            "product_version": "1.0.0",
            "plan_version": "1.0.6",
            "status": "frozen",
            "source_of_truth": ["plan/repository_tree.json"],
            "roots": roots if roots is not None else ["src"],
            "deferred_roots": [],
            "forbidden_references": ["never-present-token"],
            "phases": [{"id": "P0", "name": "contracts", "depends_on": []}],
        }
        (self.root / "plan" / "manifest.json").write_text(
            json.dumps(manifest), encoding="utf-8"
        )

    def write_output_contract(self) -> None:
        contract = json.loads(
            (REPOSITORY_ROOT / "plan" / "output_contract.json").read_text(
                encoding="utf-8"
            )
        )
        contract["plan_version"] = "1.0.6"
        (self.root / "plan" / "output_contract.json").write_text(
            json.dumps(contract), encoding="utf-8"
        )

    def write_repository_tree(self) -> None:
        (self.root / "plan" / "repository_tree.json").write_text(
            json.dumps(
                {
                    "schema_version": 1,
                    "plan_version": "1.0.6",
                    "status": "frozen",
                    "materialization": "planned",
                    "aliases": {"metadata": "md", "config": "cfg"},
                    "language_separation": [
                        {"root": "include/blitzar", "children": ["c", "cpp"]}
                    ],
                    "source_test_pairs": [
                        {
                            "source": "src",
                            "tests": "tests",
                            "responsibility": "fixture",
                        }
                    ],
                    "allowed_missing": [],
                    "test_suffix": {
                        "suffix": "Test",
                        "extensions": [".c", ".cpp", ".cmake"],
                        "entrypoint_policy": "registered tests end in Test",
                        "non_test_entrypoints": ["examples/c/Example.c"],
                    },
                    "target_test_entrypoints": ["tests/FixtureTest.cpp"],
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
            ),
            encoding="utf-8",
        )

    def write_quality(self, evidence_policy: str = "registration-only") -> None:
        quality = {
            "evidence_policy": evidence_policy,
            "repository_tree": "plan/repository_tree.json",
            "tests": [
                {
                    "id": "TST-P0-001",
                    "name": "fixture",
                    "command": "fixture_test",
                    "phase": "P0",
                }
            ],
            "architecture": {
                "report_schema": 1,
                "command": "python -B -m tools.architecture.architecture_report --root . --check",
                "review_registry": "plan/architecture_reviews.json",
                "line_count_policy": "informational",
                "thresholds": {
                    "max_parameters": 4,
                    "max_function_lines": 80,
                    "max_functions_per_file": 12,
                    "max_branch_points": 12,
                    "max_allocation_sites": 8,
                    "max_internal_includes": 12,
                },
            },
            "checks": [
                {
                    "id": "CHK-P0-001",
                    "name": "fixture-check",
                    "command": "python -B -m tools.gates.plan_check_test",
                    "phase": "P0",
                },
                {
                    "id": "CHK-P0-050",
                    "name": "directory-symmetry",
                    "command": "python -m tools.gates.directory_symmetry_gate --root . --check",
                    "phase": "P0",
                },
                {
                    "id": "CHK-P0-051",
                    "name": "directory-symmetry-fixtures",
                    "command": "python -m tools.gates.directory_symmetry_gate_test",
                    "phase": "P0",
                },
                {
                    "id": "CHK-P0-052",
                    "name": "repository-tree-contract",
                    "command": "python -m tools.gates.repository_tree_gate --root . --plan",
                    "phase": "P0",
                },
                {
                    "id": "CHK-P0-053",
                    "name": "repository-tree-contract-fixtures",
                    "command": "python -m tools.gates.repository_tree_gate_test",
                    "phase": "P0",
                },
            ],
        }
        quality["directory_symmetry"] = {
            "pairs": [{"source": "src", "tests": "tests"}],
            "allowed_missing": [],
        }
        (self.root / "plan" / "quality.json").write_text(
            json.dumps(quality), encoding="utf-8"
        )

    def run_checker(self) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [sys.executable, "-m", "tools.gates.plan_check", "--root", str(self.root)],
            check=False,
            capture_output=True,
            text=True,
            cwd=REPOSITORY_ROOT,
        )

    def test_valid_fixture(self) -> None:
        result = self.run_checker()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("runtime evidence is not claimed", result.stdout)

    def test_rejects_empty_path_component(self) -> None:
        self.write_manifest(["src//broken"])
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("unsafe path", result.stderr)

    def test_rejects_missing_materialized_root(self) -> None:
        self.write_manifest(["missing"])
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("materialized root is missing", result.stderr)

    def test_rejects_materialized_deferred_root(self) -> None:
        deferred = self.root / "future"
        deferred.mkdir()
        manifest_path = self.root / "plan" / "manifest.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        manifest["deferred_roots"] = ["future"]
        manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("deferred root is materialized", result.stderr)

    def test_rejects_missing_directory_symmetry_policy(self) -> None:
        quality_path = self.root / "plan" / "quality.json"
        quality = json.loads(quality_path.read_text(encoding="utf-8"))
        del quality["directory_symmetry"]
        quality_path.write_text(json.dumps(quality), encoding="utf-8")

        result = self.run_checker()

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("directory_symmetry policy", result.stderr)

    def test_rejects_missing_repository_tree_policy(self) -> None:
        (self.root / "plan" / "repository_tree.json").unlink()

        result = self.run_checker()

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("repository_tree.json is missing", result.stderr)

    def test_rejects_unprotected_plan_workflow(self) -> None:
        workflow_path = self.root / ".github" / "workflows" / "plan.yml"
        workflow = workflow_path.read_text(encoding="utf-8")
        workflow_path.write_text(
            workflow.replace("plan/.*\\.(json|md)", "PLAN.md"),
            encoding="utf-8",
        )

        result = self.run_checker()

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("plan workflow must guard", result.stderr)

    def test_rejects_uncovered_repository_shape_path(self) -> None:
        known = self.root / "src" / "known"
        known.mkdir()
        (known / "Owned.cpp").write_text("int owned = 0;\n", encoding="utf-8")
        self.write_manifest(["src/known"])
        plan_path = self.root / "PLAN.md"
        plan_path.write_text(
            plan_path.read_text(encoding="utf-8").replace("src/", "src/cuda_runtime/"),
            encoding="utf-8",
        )
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("repository shape path is not covered", result.stderr)

    def test_rejects_noncontiguous_phase_ids(self) -> None:
        manifest_path = self.root / "plan" / "manifest.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        manifest["phases"] = [
            {"id": "P1", "name": "wrong-order", "depends_on": []}
        ]
        manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("phase identifiers", result.stderr)

    def test_rejects_runtime_evidence_claim(self) -> None:
        self.write_quality("executed")
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("evidence_policy", result.stderr)

    def test_rejects_inconsistent_output_contract(self) -> None:
        contract_path = self.root / "plan" / "output_contract.json"
        contract = json.loads(contract_path.read_text(encoding="utf-8"))
        contract["snapshot"]["version"] = 2
        contract_path.write_text(json.dumps(contract), encoding="utf-8")
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("output contract snapshot", result.stderr)

    def test_rejects_duplicate_quality_id(self) -> None:
        quality_path = self.root / "plan" / "quality.json"
        quality = json.loads(quality_path.read_text(encoding="utf-8"))
        quality["tests"].append(quality["tests"][0])
        quality_path.write_text(json.dumps(quality), encoding="utf-8")
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("invalid or duplicate quality test ID", result.stderr)

    def test_rejects_ctest_mismatch(self) -> None:
        cmake_path = self.root / "CMakeLists.txt"
        cmake = cmake_path.read_text(encoding="utf-8")
        cmake_path.write_text(
            cmake.replace("TST-P0-001", "TST-P0-002"), encoding="utf-8"
        )
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("CTest manifest mismatch", result.stderr)

    def test_rejects_duplicate_source_filename(self) -> None:
        duplicate_dir = self.root / "src" / "Nested"
        duplicate_dir.mkdir()
        (duplicate_dir / "Valid.cpp").write_text(
            "int duplicate_value = 0;\n", encoding="utf-8"
        )
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("duplicate source filename", result.stderr)

    def test_rejects_forbidden_reference(self) -> None:
        (self.root / "src" / "Valid.cpp").write_text(
            "const char* marker = \"never-present-token\";\n",
            encoding="utf-8",
        )
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("forbidden reference", result.stderr)

    def test_rejects_generic_detail_namespace(self) -> None:
        (self.root / "src" / "Valid.cpp").write_text(
            "namespace blitzar { namespace detail {} }\n",
            encoding="utf-8",
        )
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("generic nested detail namespace", result.stderr)

    def test_rejects_shared_particle_arena(self) -> None:
        (self.root / "src" / "Valid.cpp").write_text(
            "std::shared_ptr<ParticleArena> arena;\n", encoding="utf-8"
        )
        result = self.run_checker()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("shared ParticleArena ownership", result.stderr)


if __name__ == "__main__":
    unittest.main()
