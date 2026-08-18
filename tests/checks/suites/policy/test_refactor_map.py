from __future__ import annotations

from pathlib import Path

from scripts.analysis.refactor_map import build_report


def test_refactor_map_groups_sources_and_classifies_pointers(tmp_path: Path) -> None:
    source = tmp_path / "engine" / "server" / "SrvSample.cpp"
    header = tmp_path / "engine" / "server" / "SrvSample.hpp"
    source.parent.mkdir(parents=True)
    source.write_text("int run() { int value = 1; return value; }\n", encoding="utf-8")
    header.write_text("#ifndef SAMPLE_HPP\n#define SAMPLE_HPP\nint run();\n#endif\n", encoding="utf-8")

    report = build_report(tmp_path)

    assert report["source_files"] == 2
    assert report["modules"]["engine/server"]["files"] == 2
    assert report["functions"] == 1
    assert report["potential_dead_files"] == ["engine/server/SrvSample.cpp"]
