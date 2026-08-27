import copy
import os
import pathlib
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from tools.audit.audit_contract import is_inside, load_json, owner_for, validate_contract
from tools.audit.audit_report import find_integration_commit, lane_errors, lane_records
from tools.audit.audit_scan import audit_files


class FinalAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = pathlib.Path(__file__).resolve().parents[2]
        cls.contract = load_json(cls.root / "plan" / "final_audit.json")

    def test_contract_is_valid(self) -> None:
        self.assertEqual(validate_contract(self.root, self.contract), [])

    def test_specific_owner_overrides_plan_area(self) -> None:
        owner = owner_for(
            "plan/decisions/034-final-repository-qualification.md", self.contract
        )

        self.assertIsNotNone(owner)
        self.assertEqual(owner["category"], "architecture-decision")

    def test_every_tracked_file_has_owner_and_clean_content(self) -> None:
        matrix, errors = audit_files(self.root, self.contract)

        self.assertGreaterEqual(len(matrix), 309)
        self.assertEqual(errors, [])
        self.assertTrue(all(item["owner"] and item["category"] for item in matrix))

    def test_contract_rejects_missing_issue_evidence(self) -> None:
        invalid = copy.deepcopy(self.contract)
        invalid["milestone_issues"][0]["evidence"] = []

        errors = validate_contract(self.root, invalid)

        self.assertTrue(any("needs evidence" in error for error in errors))

    def test_lane_state_is_read_from_environment(self) -> None:
        with patch.dict(os.environ, {"BLITZAR_LANE_BUILD": "success"}, clear=False):
            records = lane_records(self.contract)

        build = next(item for item in records if item["id"] == "build")

        self.assertEqual(build["state"], "success")

    def test_squash_merge_integration_commit_is_resolved(self) -> None:
        commit = "a" * 40
        history = f"{commit}\tIssue #645: Integrate configured outputs (#660)"

        with patch("tools.audit.audit_report.git_output", return_value=history):
            resolved = find_integration_commit(self.root, 645)

        self.assertEqual(resolved, commit)

    def test_squash_merge_with_issue_style_title_is_resolved_by_pull_request(self) -> None:
        commit = "b" * 40
        history = f"{commit}\trefactor(issue-656): isolate particle staging boundary (#657)"

        with patch("tools.audit.audit_report.git_output", return_value=history):
            resolved = find_integration_commit(
                self.root, 656, "https://github.com/Luis1454/BLITZAR/pull/657"
            )

        self.assertEqual(resolved, commit)

    def test_strict_mode_rejects_failed_lane(self) -> None:
        lanes = [{"id": "build", "state": "failure"}]

        errors = lane_errors(lanes, True)

        self.assertEqual(errors, ["required CI lane is not successful: build=failure"])

    def test_output_must_be_external(self) -> None:
        self.assertTrue(is_inside(self.root / "plan" / "audit", self.root))
        self.assertFalse(is_inside(self.root.parent / "audit", self.root))


if __name__ == "__main__":
    unittest.main()
