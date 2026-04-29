#!/usr/bin/env python3
# @file tests/checks/suites/core/test_framework_core.py
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

from __future__ import annotations

from pathlib import Path

import python_tools.core.runner as runner_module
from python_tools.core.base_check import BaseCheck
from python_tools.core.io import collect_test_ids
from python_tools.core.models import CheckContext, CheckResult
from python_tools.core.reporting import ResultReporter
from python_tools.core.runner import CheckRunner, resolve_sequence
from tests.checks.catalog import load_check_registry, load_check_sequences, load_command_specs
from tests.checks.run import _build_root_parser, build_context


# @brief Defines the flow check type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class _FlowCheck(BaseCheck):
    name = "flow"

    # @brief Documents the init operation contract.
    # @param None This contract does not take explicit parameters.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def __init__(self) -> None:
        self.calls: list[str] = []

    # @brief Documents the preflight operation contract.
    # @param context Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _preflight(self, context: CheckContext, result: CheckResult) -> None:
        del context, result
        self.calls.append("pre")

    # @brief Documents the execute operation contract.
    # @param context Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        del context, result
        self.calls.append("run")

    # @brief Documents the postprocess operation contract.
    # @param context Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _postprocess(self, context: CheckContext, result: CheckResult) -> None:
        del context, result
        self.calls.append("post")


# @brief Defines the static check type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class _StaticCheck(BaseCheck):
    # @brief Documents the init operation contract.
    # @param ok Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def __init__(self, ok: bool) -> None:
        self._ok = ok
        self.name = "static"

    # @brief Documents the execute operation contract.
    # @param context Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        del context
        if not self._ok:
            result.add_error("failed")


# @brief Documents the context operation contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _context(tmp_path: Path) -> CheckContext:
    return CheckContext(root=tmp_path, with_launcher=False)


# @brief Documents the test base check template method order operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_base_check_template_method_order() -> None:
    check = _FlowCheck()
    result = check.run(CheckContext(root=Path(".").resolve()))
    assert result.ok
    assert check.calls == ["pre", "run", "post"]


# @brief Documents the test collect test ids includes rust tests operation contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_collect_test_ids_includes_rust_tests(tmp_path: Path) -> None:
    rust_test = tmp_path / "rust" / "blitzar-protocol" / "tests" / "protocol.rs"
    rust_test.parent.mkdir(parents=True)
    rust_test.write_text("#[test]\nfn tst_rust_prot_001_round_trip() {}\n", encoding="utf-8")
    assert "blitzar_protocol::tests::protocol::tst_rust_prot_001_round_trip" in collect_test_ids(tmp_path, set())


# @brief Documents the test result reporter handles success operation contract.
# @param capsys Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_result_reporter_handles_success(capsys) -> None:
    ok = ResultReporter().emit(CheckResult(name="x", success_message="ok"))
    captured = capsys.readouterr()
    assert ok
    assert "ok" in captured.out


# @brief Documents the test check catalog exposes expected sequences operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_check_catalog_exposes_expected_sequences() -> None:
    assert load_check_sequences()["all"] == ["ini", "mirror", "no_legacy", "quality", "test_catalog", "pr_policy", "repo"]
    assert "python_quality" in load_check_registry()
    assert "cpp_api_docs" in load_check_registry()


# @brief Documents the test command catalog includes expected dispatch entries operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_command_catalog_includes_expected_dispatch_entries() -> None:
    assert set(load_command_specs()) == {"clang_tidy", "ivv_gate", "main_delivery_gate", "pr_policy", "traceability_gate"}


# @brief Documents the test run parser builds clang tidy context with defaults operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_run_parser_builds_clang_tidy_context_with_defaults() -> None:
    parser = _build_root_parser()
    args = parser.parse_args(["clang_tidy", "--build-dir", "build-quality"])
    context = build_context(args)
    assert context.build_dir is not None
    assert context.build_dir.name == "build-quality"
    assert context.clang_tidy_checks == "-*,clang-analyzer-*,bugprone-unused-return-value"
    assert context.clang_tidy_jobs == 0
    assert context.clang_tidy_log_dir is None
    assert context.clang_tidy_diff_base == ""
    assert context.clang_tidy_diff_target == ""
    assert context.clang_tidy_header_filter == "([/\\\\]|^)(apps|engine|runtime|modules|tests)([/\\\\])"
    assert context.clang_tidy_file_timeout_sec == 0
    assert context.clang_tidy_timeout_fallback_checks == ""


# @brief Documents the test run parser uses environment defaults for gate commands operation contract.
# @param monkeypatch Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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


# @brief Documents the test resolve sequence all without launcher operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_resolve_sequence_all_without_launcher() -> None:
    assert resolve_sequence("all", False) == ["ini", "mirror", "no_legacy", "quality", "test_catalog", "pr_policy", "repo"]


# @brief Documents the test resolve sequence all with launcher operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_resolve_sequence_all_with_launcher() -> None:
    assert resolve_sequence("all", True) == ["ini", "mirror", "no_legacy", "launcher", "quality", "test_catalog", "pr_policy", "repo"]


# @brief Documents the test runner dispatches and aggregates operation contract.
# @param monkeypatch Input value used by this contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_runner_dispatches_and_aggregates(monkeypatch, tmp_path: Path) -> None:
    calls: list[str] = []

    # @brief Documents the factory operation contract.
    # @param name Input value used by this contract.
    # @param ok Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _factory(name: str, ok: bool):
        # @brief Documents the build operation contract.
        # @param None This contract does not take explicit parameters.
        # @return Value produced by this contract when applicable.
        # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
        def _build() -> BaseCheck:
            calls.append(name)
            return _StaticCheck(ok)

        return _build

    monkeypatch.setattr(runner_module, "ResultReporter", lambda: type("R", (), {"emit": staticmethod(lambda r: r.ok)})())
    runner = CheckRunner({"quality": _factory("quality", True), "test_catalog": _factory("test_catalog", False), "pr_policy": _factory("pr_policy", True)})
    assert runner.run("quality", _context(tmp_path)) is False
    assert calls == ["quality", "test_catalog", "pr_policy"]


# @brief Documents the test runner uses custom catalog sequences operation contract.
# @param monkeypatch Input value used by this contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_runner_uses_custom_catalog_sequences(monkeypatch, tmp_path: Path) -> None:
    calls: list[str] = []

    # @brief Documents the factory operation contract.
    # @param name Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _factory(name: str):
        # @brief Documents the build operation contract.
        # @param None This contract does not take explicit parameters.
        # @return Value produced by this contract when applicable.
        # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
        def _build() -> BaseCheck:
            calls.append(name)
            return _StaticCheck(True)

        return _build

    monkeypatch.setattr(runner_module, "ResultReporter", lambda: type("R", (), {"emit": staticmethod(lambda r: r.ok)})())
    runner = CheckRunner({"alpha": _factory("alpha"), "beta": _factory("beta")}, sequences={"bundle": ["beta", "alpha"]})
    assert runner.run("bundle", _context(tmp_path)) is True
    assert calls == ["beta", "alpha"]
