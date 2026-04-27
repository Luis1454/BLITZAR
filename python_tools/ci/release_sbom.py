#!/usr/bin/env python3
# @file python_tools/ci/release_sbom.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import hashlib
import json
from datetime import UTC, datetime
from pathlib import Path

from python_tools.ci.release_support import resolve_release_tag


# @brief Defines the release sbom packager type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class ReleaseSbomPackager:
    # @brief Documents the resolve tag operation contract.
    # @param explicit Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def resolve_tag(self, explicit: str | None) -> str:
        return resolve_release_tag(explicit)

    # @brief Documents the package operation contract.
    # @param artifacts_dir Input value used by this contract.
    # @param dist_dir Input value used by this contract.
    # @param tag Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def package(self, artifacts_dir: Path, dist_dir: Path, tag: str) -> Path:
        dist_dir.mkdir(parents=True, exist_ok=True)
        sbom_path = dist_dir / f"blitzar-{tag}-sbom.cdx.json"
        payload = self._build_payload(artifacts_dir, tag)
        sbom_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
        return sbom_path

    # @brief Documents the build payload operation contract.
    # @param artifacts_dir Input value used by this contract.
    # @param tag Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _build_payload(self, artifacts_dir: Path, tag: str) -> dict[str, object]:
        components = [self._build_component(path, artifacts_dir) for path in self._iter_artifacts(artifacts_dir)]
        return {
            "bomFormat": "CycloneDX",
            "specVersion": "1.5",
            "version": 1,
            "serialNumber": f"urn:uuid:blitzar-{tag}",
            "metadata": {
                "timestamp": datetime.now(UTC).replace(microsecond=0).isoformat().replace("+00:00", "Z"),
                "component": {
                    "type": "application",
                    "name": "BLITZAR",
                    "version": tag,
                },
            },
            "components": components,
        }

    # @brief Documents the iter artifacts operation contract.
    # @param artifacts_dir Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _iter_artifacts(self, artifacts_dir: Path) -> list[Path]:
        if not artifacts_dir.exists():
            raise FileNotFoundError(f"artifacts directory not found: {artifacts_dir}")
        return sorted(path for path in artifacts_dir.rglob("*") if path.is_file())

    # @brief Documents the build component operation contract.
    # @param path Input value used by this contract.
    # @param artifacts_dir Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _build_component(self, path: Path, artifacts_dir: Path) -> dict[str, object]:
        rel = path.relative_to(artifacts_dir).as_posix()
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        return {
            "type": "file",
            "name": path.name,
            "version": "1",
            "bom-ref": rel,
            "scope": "required",
            "hashes": [{"alg": "SHA-256", "content": digest}],
            "properties": [
                {"name": "gravity:path", "value": rel},
                {"name": "gravity:size-bytes", "value": str(path.stat().st_size)},
            ],
        }
