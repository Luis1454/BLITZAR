#!/usr/bin/env python3
# @file python_tools/policies/architecture_contract.py
# @project BLITZAR
# @brief Machine-checkable uniform responsibility layout contract.

from __future__ import annotations

import re
from pathlib import Path

from python_tools.core.base_check import BaseCheck
from python_tools.core.models import CheckContext, CheckResult

CONFIG_CHILDREN = (
    "args", "core", "directive", "env", "modes", "model", "profile",
    "registry", "text", "validation",
)

LEAF_MODULES = (
    "engine/batch", "engine/core", "engine/graphics", "engine/platform",
    "engine/types", "engine/server/simulation", "engine/physics/core",
    "engine/physics/cuda", "engine/physics/fmm", "engine/physics/jit",
    "engine/physics/octree", "engine/physics/sph", "engine/physics/thermal",
    "engine/physics/treepm",
) + tuple(f"engine/config/{child}" for child in CONFIG_CHILDREN)

AGGREGATORS = ("engine/config", "engine/physics", "engine/server")

RESPONSIBILITY_PREFIXES = (
    "Bat", "Cfg", "Cli", "Cmd", "Cud", "Ffi", "Fmm", "Fnd", "Gfx",
    "Gui", "Jit", "Oct", "Phy", "Plt", "Ptc", "Pxy", "Srv", "Sph",
    "Thm", "Tpm", "Typ",
)

SOURCE_SUFFIXES = {".cpp", ".hpp", ".cu", ".cuh", ".inl"}
FORBIDDEN_DIRECTORY_NAMES = {"src", "include", "private", "public", "api", "details", "fragments"}
GENERIC_STEMS = {
    "Assignment", "Benchmark", "Bounds", "Buffer", "Build", "Cache", "Core",
    "Deposit", "Evaluation", "Execution", "Fft", "Field", "Force", "Grid",
    "Graph", "Kernels", "Layout", "Math", "Neighbor", "Prelude", "Preparation",
    "ShortRange", "Source", "State", "Thermal", "TreeForce", "TreePmForce", "Update",
}


class ArchitectureContractCheck(BaseCheck):
    name = "architecture"
    success_message = "Repository architecture contract passed."
    failure_title = "Repository architecture contract failed:"

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        self._check_modules(context.root, result)
        self._check_aggregators(context.root, result)
        self._check_forbidden_directories(context.root, result)
        self._check_production_names(context.root, result)
        self._check_manifest_paths(context.root, result)

    def _check_modules(self, root: Path, result: CheckResult) -> None:
        for relative in LEAF_MODULES:
            module = root / relative
            if not (module / "Module.cmake").is_file():
                result.add_error(f"missing module manifest: {relative}/Module.cmake")
                continue

            sources = [
                path for path in module.rglob("*")
                if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES
                and "tests" not in path.parts
            ]
            if not sources:
                result.add_error(f"module has no production source: {relative}")

            for path in sources:
                parts = path.relative_to(module).parts
                if len(parts) < 2:
                    result.add_error(
                        f"production source must be under a responsibility directory: {path.relative_to(root)}"
                    )
                if parts and parts[0] in FORBIDDEN_DIRECTORY_NAMES:
                    result.add_error(f"generic source directory is not allowed: {path.relative_to(root)}")

    def _check_aggregators(self, root: Path, result: CheckResult) -> None:
        for relative in AGGREGATORS:
            aggregator = root / relative
            if not aggregator.is_dir():
                result.add_error(f"missing domain aggregator: {relative}")
                continue
            for path in aggregator.iterdir():
                if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES:
                    result.add_error(f"aggregator contains production source: {path.relative_to(root)}")

        config = root / "engine/config"
        for child in CONFIG_CHILDREN:
            if not (config / child).is_dir():
                result.add_error(f"config aggregator child missing: engine/config/{child}")

    def _check_forbidden_directories(self, root: Path, result: CheckResult) -> None:
        for base in (root / "engine", root / "modules"):
            if not base.is_dir():
                continue
            for directory in base.rglob("*"):
                if not directory.is_dir() or directory.name not in FORBIDDEN_DIRECTORY_NAMES:
                    continue
                if any(path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES for path in directory.rglob("*")):
                    result.add_error(
                        f"forbidden generic directory contains production source: {directory.relative_to(root)}"
                    )

    def _check_production_names(self, root: Path, result: CheckResult) -> None:
        for base in (root / "engine", root / "runtime", root / "modules"):
            if not base.is_dir():
                continue
            for path in base.rglob("*"):
                if "tests" in path.parts or not path.is_file() or path.suffix.lower() not in SOURCE_SUFFIXES:
                    continue
                if path.stem in GENERIC_STEMS:
                    result.add_error(
                        f"generic production filename lacks responsibility prefix: {path.relative_to(root)}"
                    )
                elif path.name == "Module.cpp" and "module" in path.parts:
                    continue
                elif not path.stem.startswith(RESPONSIBILITY_PREFIXES):
                    result.add_error(
                        f"production filename lacks responsibility prefix ({','.join(RESPONSIBILITY_PREFIXES)}): "
                        f"{path.relative_to(root)}"
                    )

    def _check_manifest_paths(self, root: Path, result: CheckResult) -> None:
        assignment = re.compile(r'set\((\w+)_DIR\s+"\$\{BLITZAR_ROOT_DIR\}/([^\"]+)"')
        source = re.compile(r'"\$\{(\w+)_DIR\}/([^\"]+\.(?:cpp|hpp|cu|cuh|inl))"')
        for manifest in root.glob("engine/**/Module.cmake"):
            text = manifest.read_text(encoding="utf-8")
            directories = {match.group(1): match.group(2) for match in assignment.finditer(text)}
            for match in source.finditer(text):
                base = directories.get(match.group(1))
                if base is not None and not (root / base / match.group(2)).is_file():
                    result.add_error(
                        f"module manifest references missing path: {manifest.relative_to(root)}: "
                        f"{base}/{match.group(2)}"
                    )
