#!/usr/bin/env python3
# @file python_tools/policies/architecture_contract.py
# @project BLITZAR
# @brief Machine-checkable repository layout and filename contract.

from __future__ import annotations

import re
from pathlib import Path

from python_tools.core.base_check import BaseCheck
from python_tools.core.models import CheckContext, CheckResult

CONFIG_CHILDREN = (
    "args", "core", "directive", "env", "modes",
    "profile", "registry", "text", "validation",
)
LEAF_MODULES = (
    "engine/batch", "engine/core", "engine/graphics", "engine/platform",
    "engine/server", "engine/types", "engine/physics/core",
    "engine/physics/cuda", "engine/physics/fmm", "engine/physics/octree",
    "engine/physics/sph", "engine/physics/thermal", "engine/physics/treepm",
) + tuple(f"engine/config/{child}" for child in CONFIG_CHILDREN)
FRAGMENT_ROOTS = (
    "engine/physics/cuda/fragments",
    "engine/physics/octree/cuda/fragments",
    "engine/physics/sph/cuda/fragments",
    "engine/physics/thermal/cuda/fragments",
    "engine/physics/treepm/cuda/fragments",
    "engine/physics/treepm/src/fragments",
)
RESPONSIBILITY_PREFIXES = ("Cfg", "Cud", "Jit", "Oct", "Sph", "Srv", "Thm", "Tpm")
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
        self._check_config_aggregator(context.root, result)
        self._check_fragment_names(context.root, result)
        self._check_generic_names(context.root, result)
        self._check_manifest_paths(context.root, result)

    def _check_modules(self, root: Path, result: CheckResult) -> None:
        for relative in LEAF_MODULES:
            module = root / relative
            if not (module / "Module.cmake").is_file():
                result.add_error(f"missing module manifest: {relative}/Module.cmake")
            code_directories = ("include", "src", "cuda")
            if not any((module / directory).is_dir() for directory in code_directories):
                result.add_error(f"module has no code directory: {relative}")

    def _check_config_aggregator(self, root: Path, result: CheckResult) -> None:
        aggregator = root / "engine/config"
        for child in CONFIG_CHILDREN:
            if not (aggregator / child).is_dir():
                result.add_error(f"config aggregator child missing: engine/config/{child}")
        paths = aggregator.iterdir() if aggregator.is_dir() else ()
        for path in paths:
            if path.is_file() and path.suffix.lower() in {".cpp", ".cu", ".cuh", ".inl"}:
                result.add_error(f"config aggregator contains implementation: {path.relative_to(root)}")

    def _check_fragment_names(self, root: Path, result: CheckResult) -> None:
        for relative in FRAGMENT_ROOTS:
            directory = root / relative
            if not directory.is_dir():
                result.add_error(f"fragment root missing: {relative}")
                continue
            for path in directory.rglob("*.inl"):
                if not path.stem.startswith(RESPONSIBILITY_PREFIXES):
                    result.add_error(
                        f"fragment lacks responsibility prefix ({','.join(RESPONSIBILITY_PREFIXES)}): "
                        f"{path.relative_to(root)}"
                    )

    def _check_generic_names(self, root: Path, result: CheckResult) -> None:
        engine = root / "engine"
        for path in engine.rglob("*"):
            if not path.is_file() or path.suffix.lower() not in {".cpp", ".hpp", ".cu", ".cuh", ".inl"}:
                continue
            if path.stem in GENERIC_STEMS:
                result.add_error(f"generic engine filename lacks responsibility prefix: {path.relative_to(root)}")

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
