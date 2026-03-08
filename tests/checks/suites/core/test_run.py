#!/usr/bin/env python3
from __future__ import annotations

from tests.checks.catalog import load_check_registry, load_check_sequences, load_command_specs
from tests.checks.run import _build_root_parser, build_context


def test_check_catalog_exposes_expected_sequences() -> None:
    assert load_check_sequences()["all"] == ["ini", "mirror", "no_legacy", "quality", "test_catalog", "pr_policy", "repo"]
    assert "python_quality" in load_check_registry()


def test_command_catalog_includes_expected_dispatch_entries() -> None:
    commands = load_command_specs()
    assert set(commands) == {"clang_tidy", "ivv_gate", "main_delivery_gate", "pr_policy", "traceability_gate"}


def test_run_parser_builds_clang_tidy_context_with_defaults() -> None:
    parser = _build_root_parser()
    args = parser.parse_args(["clang_tidy", "--build-dir", "build-quality"])

    context = build_context(args)

    assert context.build_dir is not None
    assert context.build_dir.name == "build-quality"
    assert context.clang_tidy_checks == "-*,clang-analyzer-*,bugprone-unused-return-value"


def test_run_parser_uses_environment_defaults_for_gate_commands(monkeypatch) -> None:
    monkeypatch.setenv("GITHUB_EVENT_NAME", "pull_request")
    monkeypatch.setenv("GITHUB_EVENT_PATH", "event.json")
    monkeypatch.setenv("GITHUB_REPOSITORY", "owner/repo")
    monkeypatch.setenv("GITHUB_TOKEN", "secret")
    parser = _build_root_parser()
    args = parser.parse_args(["ivv_gate"])

    context = build_context(args)

    assert context.event_name == "pull_request"
    assert context.event_path == "event.json"
    assert context.options == {"repo": "owner/repo", "token": "secret"}
