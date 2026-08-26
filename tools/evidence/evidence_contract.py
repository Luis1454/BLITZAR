"""Pure helpers for the reproducible scaling evidence contract."""

from __future__ import annotations

import json
import math
import pathlib
from typing import Any


KINDS = {"strong", "weak", "migration", "overlap"}
SOLVERS = {"direct", "barnes-hut", "fmm"}
OVERLAP_MODES = {"overlapped", "serialized"}
INTEGER_FIELDS = {
    "rank",
    "ranks",
    "particles",
    "warmup_steps",
    "timed_steps",
    "seed",
    "status",
    "backend",
    "elapsed_ns",
    "peak_rss_bytes",
    "local_before",
    "local_after",
    "local_packets",
    "ghost_packets",
    "send_bytes",
    "receive_bytes",
    "migration_sent_remote",
    "migration_received_remote",
}
BOOLEAN_FIELDS = {
    "migration_observed",
    "overlap_has_overlap",
    "oracle_checked",
    "oracle_pass",
}


def is_integer(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def load_contract(root: pathlib.Path) -> dict[str, Any]:
    path = root / "plan" / "scaling.json"
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError("scaling contract must be a JSON object")
    return value


def validate_contract(contract: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    if contract.get("schema_version") != 1:
        errors.append("schema_version must be 1")
    if not isinstance(contract.get("plan_version"), str):
        errors.append("plan_version must be a string")
    if contract.get("target") != "blitzar_scaling_test":
        errors.append("target must be blitzar_scaling_test")
    if contract.get("runner") != "tools/evidence/release_evidence.py":
        errors.append("runner must be tools/evidence/release_evidence.py")
    if not isinstance(contract.get("precision"), str) or not contract["precision"]:
        errors.append("precision must be a non-empty string")
    if not is_integer(contract.get("seed")) or contract["seed"] < 0:
        errors.append("seed must be a non-negative integer")
    if not is_integer(contract.get("warmup_steps")) or contract["warmup_steps"] < 0:
        errors.append("warmup_steps must be a non-negative integer")
    if not is_integer(contract.get("timed_steps")) or contract["timed_steps"] <= 0:
        errors.append("timed_steps must be a positive integer")
    tolerance = contract.get("oracle_tolerance")
    if not isinstance(tolerance, (int, float)) or not math.isfinite(tolerance) or tolerance < 0:
        errors.append("oracle_tolerance must be a non-negative finite number")
    workloads = contract.get("workloads")
    if not isinstance(workloads, list) or not workloads:
        errors.append("workloads must be a non-empty list")
    else:
        errors.extend(validate_workloads(workloads))
    errors.extend(validate_artifacts(contract.get("artifacts")))
    return errors


def validate_workloads(workloads: list[Any]) -> list[str]:
    errors: list[str] = []
    seen: set[str] = set()
    for index, workload in enumerate(workloads):
        prefix = f"workloads[{index}]"
        if not isinstance(workload, dict):
            errors.append(f"{prefix} must be an object")
            continue
        workload_id = workload.get("id")
        if not isinstance(workload_id, str) or not workload_id:
            errors.append(f"{prefix}.id must be a non-empty string")
        elif workload_id in seen:
            errors.append(f"duplicate workload id: {workload_id}")
        else:
            seen.add(workload_id)
        errors.extend(validate_workload(prefix, workload))
    return errors


def validate_workload(prefix: str, workload: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    kind = workload.get("kind")
    if kind not in KINDS:
        errors.append(f"{prefix}.kind is invalid")
    counts = workload.get("particle_counts")
    if not isinstance(counts, list) or any(not is_integer(item) or item <= 0 for item in counts):
        errors.append(f"{prefix}.particle_counts must contain positive integers")
    per_rank = workload.get("particles_per_rank")
    if not is_integer(per_rank) or per_rank < 0:
        errors.append(f"{prefix}.particles_per_rank must be non-negative")
    ranks = workload.get("ranks")
    if not isinstance(ranks, list) or not ranks or any(
        not is_integer(item) or item <= 0 for item in ranks
    ):
        errors.append(f"{prefix}.ranks must contain positive integers")
    elif len(set(ranks)) != len(ranks):
        errors.append(f"{prefix}.ranks must be unique")
    solvers = workload.get("solvers")
    if not isinstance(solvers, list) or not solvers or any(item not in SOLVERS for item in solvers):
        errors.append(f"{prefix}.solvers contains an invalid solver")
    modes = workload.get("overlap_modes")
    if not isinstance(modes, list) or not modes or any(item not in OVERLAP_MODES for item in modes):
        errors.append(f"{prefix}.overlap_modes contains an invalid mode")
    if not isinstance(workload.get("oracle"), bool):
        errors.append(f"{prefix}.oracle must be boolean")
    if not isinstance(workload.get("migration"), bool):
        errors.append(f"{prefix}.migration must be boolean")
    errors.extend(validate_workload_shape(prefix, workload))
    return errors


def validate_workload_shape(prefix: str, workload: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    kind = workload.get("kind")
    counts = workload.get("particle_counts")
    per_rank = workload.get("particles_per_rank")
    ranks = workload.get("ranks")
    modes = workload.get("overlap_modes")
    if kind == "strong" and (not counts or per_rank != 0):
        errors.append(f"{prefix} strong scaling needs particle_counts and zero particles_per_rank")
    if kind == "weak" and (counts or not isinstance(per_rank, int) or per_rank <= 0):
        errors.append(f"{prefix} weak scaling needs empty particle_counts and particles_per_rank")
    valid_ranks = isinstance(ranks, list) and all(is_integer(item) for item in ranks)
    valid_modes = isinstance(modes, list)
    if kind == "migration" and (
        not workload.get("migration") or not valid_ranks or 1 in ranks
    ):
        errors.append(f"{prefix} migration must enable migration and use distributed ranks")
    if kind == "overlap" and (not valid_modes or set(modes) != OVERLAP_MODES):
        errors.append(f"{prefix} overlap must contain both overlap modes")
    if kind != "overlap" and (not valid_modes or len(modes) != 1):
        errors.append(f"{prefix} non-overlap workloads must select one overlap mode")
    if kind != "migration" and workload.get("migration"):
        errors.append(f"{prefix} only migration workloads may enable migration")
    return errors


def validate_artifacts(artifacts: Any) -> list[str]:
    if not isinstance(artifacts, dict):
        return ["artifacts must be an object"]
    errors: list[str] = []
    if artifacts.get("generated_outside_source") is not True:
        errors.append("generated_outside_source must be true")
    files = artifacts.get("files")
    required = {"metadata.json", "results.json", "summary.md", "logs/"}
    if not isinstance(files, list) or not required.issubset(set(files)):
        errors.append("artifacts.files must list metadata, results, summary, and logs")
    states = artifacts.get("evidence_states")
    if not isinstance(states, list) or set(states) != {"passed", "failed", "skipped", "unknown"}:
        errors.append("artifacts.evidence_states is incomplete")
    for key in ("topology_rule", "gpu_rule"):
        if not isinstance(artifacts.get(key), str) or not artifacts[key]:
            errors.append(f"artifacts.{key} must be a non-empty string")
    return errors


def expand_workloads(contract: dict[str, Any]) -> list[dict[str, Any]]:
    expanded: list[dict[str, Any]] = []
    for workload in contract["workloads"]:
        for ranks in workload["ranks"]:
            counts = workload["particle_counts"]
            if workload["kind"] == "weak":
                counts = [workload["particles_per_rank"] * ranks]
            for particles in counts:
                for solver in workload["solvers"]:
                    for overlap in workload["overlap_modes"]:
                        expanded.append(
                            {
                                "id": workload["id"],
                                "kind": workload["kind"],
                                "particles": particles,
                                "ranks": ranks,
                                "solver": solver,
                                "overlap": overlap,
                                "oracle": workload["oracle"] and solver != "direct",
                                "migration": workload["migration"],
                            }
                        )
    return expanded


def parse_scale_record(line: str) -> dict[str, Any] | None:
    if not line.startswith("BLITZAR SCALE "):
        return None
    record: dict[str, Any] = {}
    for token in line.strip().split()[2:]:
        key, separator, value = token.partition("=")
        if not separator:
            continue
        if key in INTEGER_FIELDS:
            record[key] = int(value)
        elif key in BOOLEAN_FIELDS:
            record[key] = value == "1"
        elif key == "oracle_max_error":
            record[key] = float(value)
        else:
            record[key] = value
    return record


def aggregate_records(records: list[dict[str, Any]], workload: dict[str, Any]) -> dict[str, Any]:
    ranks = sorted({int(record["rank"]) for record in records})
    backends = sorted({str(record.get("backend", "unknown")) for record in records})
    checked = [record for record in records if record.get("oracle_checked", False)]
    return {
        "rank_count": len(ranks),
        "expected_ranks": workload["ranks"],
        "ranks": ranks,
        "ranks_complete": ranks == list(range(workload["ranks"])),
        "status_codes": sorted({int(record.get("status", -1)) for record in records}),
        "status_ok": all(record.get("status") == 0 for record in records),
        "backend": backends[0] if len(backends) == 1 else "mixed",
        "elapsed_ns": max(int(record.get("elapsed_ns", 0)) for record in records),
        "peak_rss_bytes": max(int(record.get("peak_rss_bytes", 0)) for record in records),
        "local_before": sum(int(record.get("local_before", 0)) for record in records),
        "local_after": sum(int(record.get("local_after", 0)) for record in records),
        "local_packets": sum(int(record.get("local_packets", 0)) for record in records),
        "ghost_packets": sum(int(record.get("ghost_packets", 0)) for record in records),
        "send_bytes": sum(int(record.get("send_bytes", 0)) for record in records),
        "receive_bytes": sum(int(record.get("receive_bytes", 0)) for record in records),
        "migration_observed": any(
            record.get("migration_observed", False) for record in records
        ),
        "migration_sent_remote": sum(
            int(record.get("migration_sent_remote", 0)) for record in records
        ),
        "migration_received_remote": sum(
            int(record.get("migration_received_remote", 0)) for record in records
        ),
        "overlap_has_overlap": any(
            record.get("overlap_has_overlap", False) for record in records
        ),
        "oracle_checked": bool(checked),
        "oracle_pass": all(record.get("oracle_pass", False) for record in checked),
        "oracle_max_error": max(
            (float(record.get("oracle_max_error", 0.0)) for record in checked), default=0.0
        ),
        "particles": workload["particles"],
        "solver": workload["solver"],
        "overlap": workload["overlap"],
        "kind": workload["kind"],
        "workload_id": workload["id"],
        "migration": workload["migration"],
        "scope": "single-host-multi-rank" if workload["ranks"] > 1 else "single-host",
    }
