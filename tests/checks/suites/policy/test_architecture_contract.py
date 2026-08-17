# @file tests/checks/suites/policy/test_architecture_contract.py
# @project BLITZAR
# @brief Tests for the repository architecture contract.

from pathlib import Path

from python_tools.core.models import CheckContext
from python_tools.policies.architecture_contract import ArchitectureContractCheck


def _run(root: Path):
    return ArchitectureContractCheck().run(CheckContext(root=root))


def _valid_tree(root: Path) -> None:
    modules = (
        "engine/batch", "engine/core", "engine/graphics", "engine/platform",
        "engine/server", "engine/types", "engine/physics/core",
        "engine/physics/cuda", "engine/physics/fmm", "engine/physics/octree",
        "engine/physics/sph", "engine/physics/thermal", "engine/physics/treepm",
        "engine/config/args", "engine/config/core", "engine/config/directive",
        "engine/config/env", "engine/config/modes", "engine/config/profile",
        "engine/config/registry", "engine/config/text", "engine/config/validation",
    )
    for module in modules:
        (root / module).mkdir(parents=True, exist_ok=True)
        (root / module / "Module.cmake").touch()
        (root / module / "CfgTest.cpp").touch()
    (root / "engine/config/Module.cmake").touch()
    for relative in (
        "engine/physics/cuda/fragments",
        "engine/physics/octree/cuda/fragments",
        "engine/physics/sph/cuda/fragments",
        "engine/physics/thermal/cuda/fragments",
        "engine/physics/treepm/cuda/fragments",
        "engine/physics/treepm/fragments",
    ):
        (root / relative).mkdir(parents=True, exist_ok=True)
    (root / "engine/physics/cuda/fragments/CudBuffer.inl").touch()


def test_accepts_valid_layout(tmp_path: Path) -> None:
    _valid_tree(tmp_path)
    assert _run(tmp_path).ok


def test_rejects_implementation_in_aggregator(tmp_path: Path) -> None:
    _valid_tree(tmp_path)
    (tmp_path / "engine/config/Bad.cpp").touch()
    result = _run(tmp_path)
    assert not result.ok
    assert any("config aggregator contains implementation" in error for error in result.errors)


def test_rejects_unscoped_fragment(tmp_path: Path) -> None:
    _valid_tree(tmp_path)
    (tmp_path / "engine/physics/cuda/fragments/Buffer.inl").touch()
    result = _run(tmp_path)
    assert not result.ok
    assert any("fragment lacks responsibility prefix" in error for error in result.errors)


def test_rejects_generic_engine_filename(tmp_path: Path) -> None:
    _valid_tree(tmp_path)
    (tmp_path / "engine/core/Core.cpp").touch()
    result = _run(tmp_path)
    assert not result.ok
    assert any("generic production filename" in error for error in result.errors)


def test_rejects_legacy_source_directory(tmp_path: Path) -> None:
    _valid_tree(tmp_path)
    legacy = tmp_path / "engine/core/src"
    legacy.mkdir()
    (legacy / "CfgLegacy.cpp").touch()
    result = _run(tmp_path)
    assert not result.ok
    assert any("legacy source directory" in error for error in result.errors)


def test_rejects_missing_manifest_source(tmp_path: Path) -> None:
    _valid_tree(tmp_path)
    manifest = tmp_path / "engine/core/Module.cmake"
    manifest.write_text(
        'set(BLITZAR_TEST_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/core/src")\n'
        'set(BLITZAR_TEST_SOURCES "${BLITZAR_TEST_SOURCE_DIR}/Missing.cpp")\n',
        encoding="utf-8",
    )
    result = _run(tmp_path)
    assert not result.ok
    assert any("module manifest references missing path" in error for error in result.errors)
