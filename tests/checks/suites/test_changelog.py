# File: tests/checks/suites/test_changelog.py
# Purpose: Verification coverage for the BLITZAR quality gate.

"""Unit tests for the changelog generator."""
from __future__ import annotations

from python_tools.ci.changelog import (
    ChangelogData,
    CommitEntry,
    parse_commits,
    render_markdown,
)

_SEP = "|||"


# Description: Executes the _raw operation.
def _raw(sha: str, subject: str) -> str:
    return f"{sha}{_SEP}{subject}"


# Description: Defines the TestParseCommits contract.
class TestParseCommits:
    # Description: Executes the test_tst_unt_rel_001_feat_parsed_correctly operation.
    def test_tst_unt_rel_001_feat_parsed_correctly(self) -> None:
        lines = [_raw("abc123456789", "feat(engine): add GPU octree")]
        entries = parse_commits(lines)
        assert len(entries) == 1
        e = entries[0]
        assert e.commit_type == "feat"
        assert e.scope == "engine"
        assert e.description == "add GPU octree"
        assert not e.breaking

    # Description: Executes the test_tst_unt_rel_002_fix_parsed_correctly operation.
    def test_tst_unt_rel_002_fix_parsed_correctly(self) -> None:
        lines = [_raw("def000000001", "fix: resolve null deref in particle system")]
        entries = parse_commits(lines)
        assert len(entries) == 1
        assert entries[0].commit_type == "fix"
        assert entries[0].scope == ""

    # Description: Executes the test_tst_unt_rel_003_non_conventional_skipped operation.
    def test_tst_unt_rel_003_non_conventional_skipped(self) -> None:
        lines = [
            _raw("aaa000000001", "update some stuff"),
            _raw("bbb000000002", "feat: valid commit"),
        ]
        entries = parse_commits(lines)
        assert len(entries) == 1
        assert entries[0].commit_type == "feat"

    # Description: Executes the test_tst_unt_rel_004_empty_input_returns_empty operation.
    def test_tst_unt_rel_004_empty_input_returns_empty(self) -> None:
        entries = parse_commits([])
        assert entries == []

    # Description: Executes the test_tst_unt_rel_005_breaking_flag_detected operation.
    def test_tst_unt_rel_005_breaking_flag_detected(self) -> None:
        lines = [_raw("ccc000000003", "feat!: drop legacy API")]
        entries = parse_commits(lines)
        assert len(entries) == 1
        assert entries[0].breaking is True


# Description: Defines the TestRenderMarkdown contract.
class TestRenderMarkdown:
    # Description: Executes the _make_entry operation.
    def _make_entry(
        self, ctype: str, desc: str, scope: str = "", breaking: bool = False
    ) -> CommitEntry:
        return CommitEntry(sha="sha12345", commit_type=ctype, scope=scope, breaking=breaking, description=desc)

    # Description: Executes the test_tst_unt_rel_006_features_section_present operation.
    def test_tst_unt_rel_006_features_section_present(self) -> None:
        data = ChangelogData(tag="v1.0.0", previous_tag="v0.9.0")
        data.by_type["feat"] = [self._make_entry("feat", "add new thing")]
        md = render_markdown(data)
        assert "## Features" in md
        assert "add new thing" in md

    # Description: Executes the test_tst_unt_rel_007_breaking_section_present operation.
    def test_tst_unt_rel_007_breaking_section_present(self) -> None:
        data = ChangelogData(tag="v2.0.0", previous_tag="v1.0.0")
        entry = self._make_entry("feat", "drop legacy", breaking=True)
        data.breaking.append(entry)
        data.by_type["feat"] = [entry]
        md = render_markdown(data)
        assert "Breaking Changes" in md
        assert "drop legacy" in md

    # Description: Executes the test_tst_unt_rel_008_empty_data_produces_header_only operation.
    def test_tst_unt_rel_008_empty_data_produces_header_only(self) -> None:
        data = ChangelogData(tag="v0.1.0", previous_tag="")
        md = render_markdown(data)
        assert "# Release v0.1.0" in md
        assert "## Features" not in md
