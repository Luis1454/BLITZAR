#!/usr/bin/env python3
from __future__ import annotations

from python_tools.core.base_check import BaseCheck
from python_tools.core.models import CheckContext, CheckResult

MIRROR_PAIRS = (
    ("engine/include/backend", "engine/src/backend"),
    ("engine/include/config", "engine/src/config"),
    ("engine/include/platform", "engine/src/platform"),
    ("runtime/include/backend", "runtime/src/backend"),
    ("runtime/include/frontend", "runtime/src/frontend"),
    ("runtime/include/protocol", "runtime/src/protocol"),
    ("engine/include/physics", "engine/src/physics"),
    ("engine/include/platform/common", "engine/src/platform/common"),
    ("engine/include/platform/internal", "engine/src/platform/internal"),
    ("engine/include/platform/posix", "engine/src/platform/posix"),
    ("engine/include/platform/win", "engine/src/platform/win"),
    ("modules/qt/include/ui", "modules/qt/ui"),
)
HEADER_ONLY = {
    "engine/include/backend/SimulationInitConfig.hpp",
    "engine/include/config/EnvUtils.hpp",
    "engine/include/platform/DynamicLibrary.hpp",
    "engine/include/platform/PlatformErrors.hpp",
    "engine/include/platform/PlatformPaths.hpp",
    "engine/include/platform/PlatformProcess.hpp",
    "engine/include/platform/SocketPlatform.hpp",
    "engine/include/types/SimulationTypes.hpp",
    "runtime/include/frontend/ErrorBuffer.hpp",
    "runtime/include/frontend/FrontendModuleApi.hpp",
    "runtime/include/frontend/IFrontendRuntime.hpp",
    "runtime/include/frontend/ILocalBackend.hpp",
    "runtime/include/protocol/BackendProtocol.hpp",
    "engine/include/platform/internal/DynamicLibraryOps.hpp",
    "engine/include/platform/internal/ProcessOps.hpp",
    "engine/include/platform/internal/SocketOps.hpp",
    "engine/include/physics/Gpu.hpp",
    "engine/include/physics/Octree.hpp",
}


class MirrorCheck(BaseCheck):
    name = "mirror"
    success_message = "Header/CPP mirror validation passed"
    failure_title = "Header/CPP mirror validation failed:"

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        for hdr_rel, src_rel in MIRROR_PAIRS:
            hdr_dir = context.root / hdr_rel
            src_dir = context.root / src_rel
            if not hdr_dir.exists():
                continue
            if not src_dir.exists():
                result.add_error(f"Missing source mirror directory: {src_rel}")
                continue
            self._check_pair(hdr_rel, src_rel, hdr_dir, src_dir, result)

    def _check_pair(self, hdr_rel: str, src_rel: str, hdr_dir, src_dir, result: CheckResult) -> None:
        headers = sorted(hdr_dir.glob("*.hpp"))
        sources = sorted(src_dir.glob("*.cpp"))
        header_bases = {h.stem for h in headers}
        for header in headers:
            base = header.stem
            header_rel = f"{hdr_rel}/{base}.hpp"
            if header_rel not in HEADER_ONLY and not (src_dir / f"{base}.cpp").exists():
                result.add_error(f"Missing cpp for {header_rel} -> expected {src_rel}/{base}.cpp")
        for source in sources:
            base = source.stem
            if base not in header_bases:
                result.add_error(f"Missing hpp for {src_rel}/{base}.cpp -> expected {hdr_rel}/{base}.hpp")

