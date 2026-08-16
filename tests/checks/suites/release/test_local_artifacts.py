# @file tests/checks/suites/release/test_local_artifacts.py
# @author Luis1454
# @project BLITZAR
# @brief Deterministic tests for the read-only local artifact inventory.

from __future__ import annotations

from pathlib import Path

from python_tools.ci.local_artifacts import build_report, is_generated_root


def test_generated_root_detection() -> None:
    assert is_generated_root(Path("build-raii-cuda"))
    assert is_generated_root(Path("dist"))
    assert not is_generated_root(Path("engine"))


def test_report_counts_binary_and_text_outputs(tmp_path: Path) -> None:
    build = tmp_path / "build-test"
    build.mkdir()
    (build / "module.obj").write_bytes(b"obj")
    (build / "report.txt").write_text("report", encoding="utf-8")
    (tmp_path / "engine").mkdir()
    report = build_report(tmp_path)
    assert report["total_file_count"] == 2
    assert report["total_binary_count"] == 1
    assert report["total_bytes"] == 9
    assert report["roots"][0]["path"].endswith("build-test")
