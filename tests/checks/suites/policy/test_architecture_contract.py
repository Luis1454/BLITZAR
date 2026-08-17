# @file tests/checks/suites/policy/test_architecture_contract.py
# @project BLITZAR
# @brief Tests for the uniform responsibility layout contract.

from pathlib import Path

from python_tools.core.models import CheckContext
from python_tools.policies.architecture_contract import (
    CUDA_RESPONSIBILITIES,
    MODULE_RESPONSIBILITIES,
    ArchitectureContractCheck,
)


def _run(root: Path):
    return ArchitectureContractCheck().run(CheckContext(root=root))


def _valid_tree(root: Path) -> None:
    for module, responsibilities in MODULE_RESPONSIBILITIES.items():
        (root / module).mkdir(parents=True, exist_ok=True)
        (root / module / "Module.cmake").touch()
        for responsibility in responsibilities:
            production_roots: tuple[Path, ...] = (root / module / responsibility,)
            if responsibility == "cuda" and module in CUDA_RESPONSIBILITIES:
                production_roots = tuple(
                    root / module / responsibility / child
                    for child in CUDA_RESPONSIBILITIES[module]
                )
            for production_root in production_roots:
                production = production_root / "CfgTest.cpp"
                production.parent.mkdir(parents=True, exist_ok=True)
                production.touch()
        tests = root / module / "tests"
        tests.mkdir(parents=True, exist_ok=True)
        (tests / "test.cpp").touch()
    for aggregator in ("engine/config", "engine/physics", "engine/server"):
        (root / aggregator).mkdir(parents=True, exist_ok=True)
        (root / aggregator / "Module.cmake").touch()


def test_accepts_valid_layout(tmp_path: Path) -> None:
    _valid_tree(tmp_path)
    assert _run(tmp_path).ok


def test_rejects_implementation_in_aggregator(tmp_path: Path) -> None:
    _valid_tree(tmp_path)
    (tmp_path / "engine/config/Bad.cpp").touch()
    result = _run(tmp_path)
    assert not result.ok
    assert any("aggregator contains production source" in error for error in result.errors)


def test_rejects_generic_directory(tmp_path: Path) -> None:
    _valid_tree(tmp_path)
    legacy = tmp_path / "engine/core/src"
    legacy.mkdir()
    (legacy / "CfgLegacy.cpp").touch()
    result = _run(tmp_path)
    assert not result.ok
    assert any("forbidden generic directory" in error for error in result.errors)


def test_rejects_root_production_source(tmp_path: Path) -> None:
    _valid_tree(tmp_path)
    (tmp_path / "engine/core/CfgLegacy.cpp").touch()
    result = _run(tmp_path)
    assert not result.ok
    assert any("under a responsibility directory" in error for error in result.errors)


def test_rejects_missing_manifest_source(tmp_path: Path) -> None:
    _valid_tree(tmp_path)
    manifest = tmp_path / "engine/core/Module.cmake"
    manifest.write_text(
        'set(BLITZAR_TEST_DIR "${BLITZAR_ROOT_DIR}/engine/core/model")\n'
        'set(BLITZAR_TEST_SOURCES "${BLITZAR_TEST_DIR}/Missing.cpp")\n',
        encoding="utf-8",
    )
    result = _run(tmp_path)
    assert not result.ok
    assert any("module manifest references missing path" in error for error in result.errors)
