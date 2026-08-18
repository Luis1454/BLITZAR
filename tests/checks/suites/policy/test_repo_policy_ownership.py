# @file tests/checks/suites/policy/test_repo_policy_ownership.py
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR ownership policy.

from __future__ import annotations

from pathlib import Path

from tests.checks.suites.policy.test_repo_policy import _run, _write
from python_tools.policies.human_layout_policy import check_human_layout


# @brief Documents the raw pointer member rejection contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Owning members must use RAII instead of a raw pointer.
def test_repo_policy_rejects_unqualified_raw_pointer_member(tmp_path: Path) -> None:
    _write(
        tmp_path / "engine" / "include" / "bad.hpp",
        "#ifndef BAD_HPP\n#define BAD_HPP\nstruct Bad {\n    int* value = nullptr;\n};\n#endif\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("raw pointer data member requires RAII" in error for error in errors)


# @brief Documents the explicit borrowed view acceptance contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Boundary and view contracts are intentionally non-owning.
def test_repo_policy_accepts_explicit_borrowed_view_member(tmp_path: Path) -> None:
    _write(
        tmp_path / "engine" / "physics" / "core" / "particle" / "PhyParticleSoAView.hpp",
        "#ifndef VIEW_HPP\n#define VIEW_HPP\nstruct View {\n    float* values;\n};\n#endif\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors


# @brief Documents the internal ExportsV1 pointer rejection contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note ABI table pointers must not escape the module boundary into runtime internals.
def test_repo_policy_rejects_internal_exports_pointer(tmp_path: Path) -> None:
    _write(
        tmp_path / "runtime" / "src" / "client" / "module" / "CliLoad.cpp",
        "#include \"client/module/CliApi.hpp\"\nvoid load() { const ExportsV1* table = nullptr; }\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert not ok
    assert any("ExportsV1 raw pointer is restricted" in error for error in errors)


# @brief Documents the ABI ExportsV1 pointer acceptance contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note The C ABI declaration remains an intentional non-owning boundary.
def test_repo_policy_accepts_exports_abi_pointer(tmp_path: Path) -> None:
    _write(
        tmp_path / "runtime" / "client" / "module" / "CliApi.hpp",
        "#ifndef API_HPP\n#define API_HPP\nstruct ExportsV1 {};\ntypedef const ExportsV1* (*EntryPointFn)();\n#endif\n",
    )
    ok, errors, _ = _run(tmp_path, tmp_path / "allowlist.txt")
    assert ok
    assert not errors


# @brief Documents the strict no-raw-pointer contract for the scene editor.
# @param tmp_path Input value used by this contract.
# @return None.
# @note Qt construction boundaries outside this module remain separately classified.
def test_human_layout_rejects_scene_raw_pointer_declaration() -> None:
    errors = check_human_layout(
        "modules/qt/window/scene/BadScene.cpp",
        "void build(QWidget* widget) {}\n",
    )
    assert any("raw pointer declaration is forbidden" in error for error in errors)


# @brief Documents the strict no-raw-pointer contract for the configuration editor.
# @return None.
# @note Qt casts remain isolated at the Qt boundary and are stored in QPointer values.
def test_human_layout_rejects_configuration_raw_pointer_declaration() -> None:
    errors = check_human_layout(
        "modules/qt/window/config/BadConfiguration.cpp",
        "void build(QWidget* widget) {}\n",
    )
    assert any("raw pointer declaration is forbidden" in error for error in errors)
