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

MODULE_RESPONSIBILITIES = {
    "engine/physics/core": {"vector", "particle", "force", "cuda"},
    "engine/physics/octree": {
        "model", "build", "force", "traversal", "morton", "adaptive", "cuda",
    },
    "engine/physics/treepm": {
        "model", "deposit", "fft", "field", "short_range", "layout", "graph", "cuda",
    },
    "engine/physics/fmm": {"model", "build", "evaluate", "metrics"},
    "engine/physics/sph": {"model", "grid", "kernels"},
    "engine/physics/thermal": {"model", "energy"},
    "engine/physics/jit": {"cache", "compilation", "execution", "benchmark"},
    "engine/server/simulation": {
        "configuration", "initialization", "lifecycle", "parsing", "persistence",
        "runtime", "state", "telemetry", "export",
    },
}

MODULE_RESPONSIBILITIES.update({
    "engine/batch": {"runner"},
    "engine/core": {"constants"},
    "engine/graphics": {"color", "types", "view"},
    "engine/platform": {"dynamic_library", "errors", "paths", "process", "socket"},
    "engine/types": {"simulation"},
})

CUDA_RESPONSIBILITIES = {
    "engine/physics/core": {"buffer", "core", "integration", "prelude", "runtime"},
    "engine/physics/octree": {"build", "force", "linear", "morton"},
    "engine/physics/treepm": {"deposit", "fft", "field", "graph", "layout"},
}

MODULE_RESPONSIBILITIES.update({
    "engine/config/args": {"options", "parsing"},
    "engine/config/core": {"configuration"},
    "engine/config/directive": {"parsing", "scene", "write"},
    "engine/config/env": {"platform"},
    "engine/config/modes": {"normalization"},
    "engine/config/model": {"cosmology", "scene"},
    "engine/config/profile": {"profile"},
    "engine/config/registry": {"application", "entries", "runtime"},
    "engine/config/text": {"parsing"},
    "engine/config/validation": {"physics", "render", "scenario"},
})

LEAF_MODULES = tuple(MODULE_RESPONSIBILITIES)

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

            expected = MODULE_RESPONSIBILITIES[relative]
            actual = {
                path.name for path in module.iterdir()
                if path.is_dir()
                and path.name != "tests"
                and any(candidate.is_file() for candidate in path.rglob("*"))
            }
            missing = expected - actual
            unexpected = actual - expected
            for responsibility in sorted(missing):
                result.add_error(
                    f"module responsibility missing: {relative}/{responsibility}"
                )
            for responsibility in sorted(unexpected):
                result.add_error(
                    f"module responsibility is not in contract: {relative}/{responsibility}"
                )

            backend_responsibilities = CUDA_RESPONSIBILITIES.get(relative)
            if backend_responsibilities is not None:
                cuda = module / "cuda"
                actual_backend = {
                    path.name for path in cuda.iterdir()
                    if path.is_dir() and any(candidate.is_file() for candidate in path.rglob("*"))
                } if cuda.is_dir() else set()
                for responsibility in sorted(backend_responsibilities - actual_backend):
                    result.add_error(
                        f"CUDA responsibility missing: {relative}/cuda/{responsibility}"
                    )
                for responsibility in sorted(actual_backend - backend_responsibilities):
                    result.add_error(
                        f"CUDA responsibility is not in contract: {relative}/cuda/{responsibility}"
                    )

            tests = module / "tests"
            if not tests.is_dir():
                result.add_error(f"module tests directory missing: {relative}/tests")
            elif not any(path.is_file() for path in tests.rglob("*")):
                result.add_error(f"module tests directory is empty: {relative}/tests")

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
                if len(parts) > 2 and parts[0] != "cuda":
                    result.add_error(
                        f"production source is nested below a responsibility directory: "
                        f"{path.relative_to(root)}"
                    )
                if len(parts) > 3 or (len(parts) == 3 and parts[0] != "cuda"):
                    result.add_error(
                        f"production source exceeds the allowed backend depth: "
                        f"{path.relative_to(root)}"
                    )

            for path in tests.rglob("*") if tests.is_dir() else ():
                if not path.is_file() or path.suffix.lower() not in {".cpp", ".cu", ".py"}:
                    continue
                if path.suffix.lower() in {".cpp", ".cu"} and not re.fullmatch(
                    r"[a-z0-9]+(?:_[a-z0-9]+)*\.(?:cpp|cu)", path.name
                ):
                    result.add_error(
                        f"module test filename must be snake_case: {path.relative_to(root)}"
                    )

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
                if not any(candidate.is_file() for candidate in directory.rglob("*")):
                    continue
                result.add_error(
                    f"forbidden generic directory exists: {directory.relative_to(root)}"
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
