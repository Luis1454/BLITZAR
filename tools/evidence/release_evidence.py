"""Run the versioned scaling matrix and write evidence outside the source tree."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
import os
import pathlib
import platform
import re
import shlex
import shutil
import socket
import subprocess
import sys
from typing import Any

from tools.evidence.evidence_contract import (
    aggregate_records,
    expand_workloads,
    load_contract,
    parse_scale_record,
    REQUIRED_RECORD_FIELDS,
    validate_contract,
)


@dataclass(frozen=True)
class RunContext:
    root: pathlib.Path
    output: pathlib.Path
    executable: pathlib.Path | None
    contract: dict[str, Any]
    timeout: int


def is_inside(path: pathlib.Path, root: pathlib.Path) -> bool:
    try:
        path.relative_to(root)
        return True
    except ValueError:
        return False


def run_process(
    command: list[str], cwd: pathlib.Path, environment: dict[str, str], timeout: int
) -> dict[str, Any]:
    try:
        completed = subprocess.run(
            command,
            cwd=cwd,
            env=environment,
            text=True,
            capture_output=True,
            check=False,
            timeout=timeout,
        )
        return {
            "returncode": completed.returncode,
            "stdout": completed.stdout,
            "stderr": completed.stderr,
            "cwd": str(cwd),
            "timed_out": False,
            "missing": False,
        }
    except FileNotFoundError:
        return {
            "returncode": None,
            "stdout": "",
            "stderr": "",
            "cwd": str(cwd),
            "timed_out": False,
            "missing": True,
        }
    except subprocess.TimeoutExpired as error:
        return {
            "returncode": None,
            "stdout": error.stdout or "",
            "stderr": error.stderr or "",
            "cwd": str(cwd),
            "timed_out": True,
            "missing": False,
        }


def selected_environment(environment: dict[str, str]) -> dict[str, str]:
    names = {
        "CC",
        "CXX",
        "CMAKE_BUILD_TYPE",
        "CMAKE_HIP_PLATFORM",
        "HIP_PLATFORM",
        "OMP_NUM_THREADS",
        "OMP_PLACES",
        "OMP_PROC_BIND",
        "OMPI_ALLOW_RUN_AS_ROOT",
        "OMPI_ALLOW_RUN_AS_ROOT_CONFIRM",
        "OMPI_MCA_rmaps_base_oversubscribe",
        "CUDA_VISIBLE_DEVICES",
        "ROCR_VISIBLE_DEVICES",
    }
    return {name: environment[name] for name in sorted(names) if name in environment}


def write_log(
    path: pathlib.Path, command: list[str], environment: dict[str, str], result: dict[str, Any]
) -> None:
    content = [
        f"command: {shlex.join(command)}",
        f"cwd: {result['cwd']}",
        f"environment: {json.dumps(selected_environment(environment), sort_keys=True)}",
        f"returncode: {result['returncode']}",
        f"timed_out: {result['timed_out']}",
        "",
        "[stdout]",
        str(result["stdout"]),
        "",
        "[stderr]",
        str(result["stderr"]),
        "",
    ]
    path.write_text("\n".join(content), encoding="utf-8")


def executable_path(build_dir: pathlib.Path, target: str) -> pathlib.Path | None:
    candidates = [build_dir / target]
    if os.name == "nt":
        candidates.insert(0, build_dir / f"{target}.exe")
    return next((candidate for candidate in candidates if candidate.is_file()), None)


def launcher_path(ranks: int) -> pathlib.Path | None:
    if ranks == 1:
        return None
    selected = shutil.which("mpiexec") or shutil.which("mpirun")
    return pathlib.Path(selected) if selected else None


def workload_command(
    executable: pathlib.Path, workload: dict[str, Any], contract: dict[str, Any], launcher: pathlib.Path | None
) -> list[str] | None:
    arguments = [
        str(executable),
        "--particles",
        str(workload["particles"]),
        "--warmup",
        str(contract["warmup_steps"]),
        "--steps",
        str(contract["timed_steps"]),
        "--seed",
        str(contract["seed"]),
        "--distribution",
        str(workload["distribution"]),
        "--solver",
        str(workload["solver"]),
        "--overlap",
        str(workload["overlap"]),
        "--tolerance",
        str(contract["oracle_tolerance"]),
    ]
    if workload["oracle"]:
        arguments.append("--oracle")
    if workload["migration"]:
        arguments.append("--migration")
    if workload["ranks"] == 1:
        return arguments
    if launcher is None:
        return None
    return [str(launcher), "-np", str(workload["ranks"]), *arguments]


def parse_output(output: str) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    for line in output.splitlines():
        try:
            record = parse_scale_record(line)
        except (TypeError, ValueError):
            continue
        if record is not None and "rank" in record:
            records.append(record)
    return records


def validate_result(result: dict[str, Any]) -> tuple[str, str]:
    if result["returncode"] is None:
        if result["timed_out"]:
            return "failed", "command timed out"
        return "failed", "command did not start"
    if result["returncode"] != 0:
        return "failed", f"command returned {result['returncode']}"
    if not result["records"]:
        return "failed", "no BLITZAR SCALE record was emitted"
    missing = sorted(
        field
        for record in result["records"]
        for field in REQUIRED_RECORD_FIELDS
        if field not in record
    )
    if missing:
        return "failed", f"rank records are missing fields: {sorted(set(missing))}"
    if any(
        record["schema"] != result["workload"]["record_schema_version"]
        for record in result["records"]
    ):
        return "failed", "rank record schema does not match the contract"
    if any(
        record["distribution"] != result["workload"]["distribution"]
        for record in result["records"]
    ):
        return "failed", "rank record distribution does not match the workload"
    aggregate = result["aggregate"]
    if not aggregate["ranks_complete"]:
        return "failed", "rank records are incomplete"
    if not aggregate["status_ok"]:
        return "failed", f"rank status codes: {aggregate['status_codes']}"
    if aggregate["elapsed_ns"] <= 0 or aggregate["mean_step_ns"] <= 0:
        return "failed", "timing metrics are not positive"
    if aggregate["min_step_ns"] > aggregate["max_step_ns"]:
        return "failed", "step timing bounds are inconsistent"
    if result["workload"]["allocation_policy"] == "zero_after_warmup" and not aggregate[
        "allocation_free"
    ]:
        return "failed", "steady-state allocation policy was violated"
    if result["workload"]["oracle"] and not aggregate["oracle_pass"]:
        return "failed", "Direct CPU oracle parity failed"
    if result["workload"]["kind"] == "migration" and not aggregate["migration_observed"]:
        return "failed", "no remote migration was observed"
    if result["workload"]["kind"] == "overlap":
        expected = result["workload"]["overlap"] == "overlapped"
        if aggregate["overlap_has_overlap"] != expected:
            return "failed", "overlap timeline does not match the selected mode"
    return "passed", ""


def run_workload(context: RunContext, workload: dict[str, Any]) -> dict[str, Any]:
    launcher = launcher_path(workload["ranks"])
    command = (
        None
        if context.executable is None
        else workload_command(context.executable, workload, context.contract, launcher)
    )
    result: dict[str, Any] = {"workload": workload, "command": command, "records": []}
    log_directory = context.output / "logs"
    log_directory.mkdir(parents=True, exist_ok=True)
    log_path = log_directory / f"run-{len(list(log_directory.glob('run-*.log'))):03d}.log"
    environment = dict(os.environ)

    if context.executable is None:
        result.update(
            {
                "returncode": None,
                "stdout": "",
                "stderr": "",
                "cwd": str(context.root),
                "timed_out": False,
            }
        )
        result["state"] = "failed"
        result["reason"] = "scaling executable is missing"
        result["log"] = log_path.relative_to(context.output).as_posix()
        return result
    if command is None:
        result.update(
            {
                "returncode": None,
                "stdout": "",
                "stderr": "",
                "cwd": str(context.root),
                "timed_out": False,
            }
        )
        result["state"] = "skipped"
        result["reason"] = "mpiexec or mpirun is unavailable"
        result["log"] = log_path.relative_to(context.output).as_posix()
        return result

    if workload["ranks"] > 1:
        environment.setdefault("OMPI_ALLOW_RUN_AS_ROOT", "1")
        environment.setdefault("OMPI_ALLOW_RUN_AS_ROOT_CONFIRM", "1")
        environment.setdefault("OMPI_MCA_rmaps_base_oversubscribe", "1")

    process = run_process(command, context.root, environment, context.timeout)
    combined = f"{process['stdout']}\n{process['stderr']}"
    result.update(process)
    result["records"] = parse_output(combined)
    if result["records"]:
        result["aggregate"] = aggregate_records(result["records"], workload)
    else:
        result["aggregate"] = None
    state, reason = validate_result(result)
    result["state"] = state
    result["reason"] = reason
    result["log"] = log_path.relative_to(context.output).as_posix()
    write_log(log_path, command, environment, process)
    return result


def read_cache_flag(build_dir: pathlib.Path, name: str) -> bool | None:
    cache = build_dir / "CMakeCache.txt"
    if not cache.is_file():
        return None
    pattern = re.compile(rf"^{re.escape(name)}(?::[^=]+)?=(.*)$")
    for line in cache.read_text(encoding="utf-8", errors="replace").splitlines():
        match = pattern.match(line)
        if match:
            return match.group(1).strip().upper() in {"ON", "1", "TRUE"}
    return None


def probe_gpu(root: pathlib.Path, build_dir: pathlib.Path, output: pathlib.Path) -> dict[str, Any]:
    (output / "logs").mkdir(parents=True, exist_ok=True)
    compilers = {
        name: shutil.which(name) for name in ("hipcc", "nvcc") if shutil.which(name) is not None
    }
    executable = executable_path(build_dir, "blitzar_accelerator_test")
    probe: dict[str, Any] = {
        "compile": {
            "configured": read_cache_flag(build_dir, "BLITZAR_HIP_ENABLED"),
            "compilers": compilers,
            "state": "unknown",
            "reason": "HIP build configuration was not found",
        },
        "fallback": {"state": "unknown", "reason": "HIP qualification target unavailable"},
        "device_execution": {"state": "unknown", "reason": "HIP qualification target unavailable"},
    }
    if executable is None:
        return probe

    environment = dict(os.environ)
    process = run_process([str(executable)], root, environment, 180)
    combined = f"{process['stdout']}\n{process['stderr']}"
    log_path = output / "logs" / "hip-qualification.log"
    write_log(log_path, [str(executable)], environment, process)
    probe["log"] = log_path.relative_to(output).as_posix()
    if "qualification skipped" in combined.lower() and process["returncode"] == 0:
        probe["fallback"] = {"state": "passed", "reason": "CPU fallback was exercised"}
        probe["device_execution"] = {
            "state": "skipped",
            "reason": "no physical GPU was exposed",
        }
    elif process["returncode"] == 0:
        probe["fallback"] = {"state": "not-observed", "reason": "device path was selected"}
        probe["device_execution"] = {"state": "passed", "reason": "HIP device test passed"}
    else:
        probe["device_execution"] = {"state": "failed", "reason": "HIP device test failed"}
    if probe["compile"]["configured"] is True:
        probe["compile"]["state"] = "passed"
        probe["compile"]["reason"] = "HIP backend was enabled in the build"
    elif probe["compile"]["configured"] is False:
        probe["compile"]["state"] = "skipped"
        probe["compile"]["reason"] = "build was configured CPU-only"
    return probe


def capture_command(command: list[str], root: pathlib.Path) -> str | None:
    result = run_process(command, root, dict(os.environ), 30)
    if result["returncode"] != 0:
        return None
    return str(result["stdout"]).strip()


def worktree_git_dir(root: pathlib.Path) -> pathlib.Path | None:
    git_file = root / ".git"
    if not git_file.is_file():
        return None
    content = git_file.read_text(encoding="utf-8").strip()
    prefix = "gitdir:"
    if not content.lower().startswith(prefix):
        return None
    raw_path = content[len(prefix) :].strip()
    candidate = pathlib.Path(raw_path)
    if not candidate.is_dir() and re.match(r"^[A-Za-z]:[\\/]", raw_path):
        drive = raw_path[0].lower()
        relative = raw_path[2:].replace("\\", "/").lstrip("/")
        candidate = pathlib.Path("/mnt") / drive / relative
    return candidate if candidate.is_dir() else None


def repository_revision(root: pathlib.Path) -> str | None:
    git_dir = worktree_git_dir(root)
    if git_dir is not None:
        revision = capture_command(["git", f"--git-dir={git_dir}", "rev-parse", "HEAD"], root)
        if revision is not None:
            return revision
    return capture_command(["git", "rev-parse", "HEAD"], root)


def collect_metadata(root: pathlib.Path, contract: dict[str, Any]) -> dict[str, Any]:
    compiler = os.environ.get("CXX", "c++")
    environment = dict(os.environ)
    compiler_path = shutil.which(compiler)
    thread_count = environment.get("OMP_NUM_THREADS") or "runtime-default"
    return {
        "revision": repository_revision(root),
        "platform": {
            "system": platform.system(),
            "release": platform.release(),
            "machine": platform.machine(),
            "python": platform.python_version(),
            "hostname": socket.gethostname(),
        },
        "cpu_count": os.cpu_count(),
        "execution": {
            "compiler": {
                "name": compiler,
                "path": compiler_path,
                "version": capture_command([compiler, "--version"], root),
            },
            "backend": "recorded_per_workload",
            "cpu_capability": {
                "architecture": platform.machine(),
                "processor": platform.processor(),
                "logical_threads": os.cpu_count(),
            },
            "gpu_capability": {
                "hipcc": shutil.which("hipcc"),
                "nvcc": shutil.which("nvcc"),
                "nvidia_smi": shutil.which("nvidia-smi"),
                "rocminfo": shutil.which("rocminfo"),
                "cuda_visible_devices": environment.get("CUDA_VISIBLE_DEVICES"),
                "rocr_visible_devices": environment.get("ROCR_VISIBLE_DEVICES"),
            },
            "thread_count": thread_count,
            "problem_size": {
                "field": "results[].aggregate.particles",
                "unit": "particles",
            },
        },
        "toolchain": {
            "cc": os.environ.get("CC"),
            "cxx": compiler,
            "cxx_path": compiler_path,
            "cxx_version": capture_command([compiler, "--version"], root),
            "cmake_version": capture_command(["cmake", "--version"], root),
            "mpiexec": shutil.which("mpiexec") or shutil.which("mpirun"),
            "hipcc": shutil.which("hipcc"),
            "nvcc": shutil.which("nvcc"),
            "nvidia_smi": shutil.which("nvidia-smi"),
            "rocminfo": shutil.which("rocminfo"),
        },
        "environment": selected_environment(environment),
        "contract": {
            "schema_version": contract["schema_version"],
            "record_schema_version": contract["record_schema_version"],
            "plan_version": contract["plan_version"],
            "precision": contract["precision"],
            "seed": contract["seed"],
            "oracle_tolerance": contract["oracle_tolerance"],
            "distributions": [item["id"] for item in contract["distributions"]],
            "metrics": contract["metrics"],
            "oracle": contract["oracle"],
            "allocation_policy": contract["allocation_policy"],
        },
        "topology": {
            "scope": "single-host",
            "multi_node_qualified": False,
            "rule": contract["artifacts"]["topology_rule"],
        },
    }


def add_overlap_comparisons(results: list[dict[str, Any]]) -> None:
    pairs: dict[tuple[Any, ...], dict[str, dict[str, Any]]] = {}
    for result in results:
        workload = result["workload"]
        key = (workload["id"], workload["particles"], workload["ranks"], workload["solver"])
        pairs.setdefault(key, {})[workload["overlap"]] = result
    for modes in pairs.values():
        overlapped = modes.get("overlapped")
        serialized = modes.get("serialized")
        if not overlapped or not serialized or not overlapped.get("aggregate") or not serialized.get("aggregate"):
            continue
        left = overlapped["aggregate"]
        right = serialized["aggregate"]
        left["volume_matches_serialized"] = all(
            left[field] == right[field]
            for field in ("local_packets", "ghost_packets", "send_bytes", "receive_bytes")
        )
        left["speedup_vs_serialized"] = (
            right["elapsed_ns"] / left["elapsed_ns"] if left["elapsed_ns"] else 0.0
        )
        if not left["volume_matches_serialized"] and overlapped["state"] == "passed":
            overlapped["state"] = "failed"
            overlapped["reason"] = "overlap and serialized communication volumes differ"


def render_summary(
    metadata: dict[str, Any], results: list[dict[str, Any]], gpu: dict[str, Any]
) -> str:
    lines = [
        "# BLITZAR Reproducible Evidence",
        "",
        f"- revision: `{metadata['revision']}`",
        f"- platform: `{metadata['platform']['system']} {metadata['platform']['release']}`",
        f"- precision: `{metadata['contract']['precision']}`",
        f"- seed: `{metadata['contract']['seed']}`",
        f"- oracle tolerance: `{metadata['contract']['oracle_tolerance']}`",
        "- topology: `single-host`; multi-node qualification: `not qualified`",
        "",
        "## Workloads",
        "",
        "| State | Workload | Distribution | Kind | Particles | Ranks | Solver | Mode | Backend | Elapsed ns | Mean step ns | Allocations | Throughput/s | Peak RSS | Oracle error | Scope | Reason |",
        "| --- | --- | --- | --- | ---: | ---: | --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |",
    ]
    for result in results:
        aggregate = result.get("aggregate") or {}
        lines.append(
            "| {state} | {workload} | {distribution} | {kind} | {particles} | {ranks} | "
            "{solver} | {mode} | {backend} | {elapsed} | {mean_step} | {allocations} | "
            "{throughput:.6g} | {rss} | {error} | {scope} | {reason} |".format(
                state=result["state"],
                workload=result["workload"]["id"],
                distribution=result["workload"]["distribution"],
                kind=result["workload"]["kind"],
                particles=result["workload"]["particles"],
                ranks=result["workload"]["ranks"],
                solver=result["workload"]["solver"],
                mode=result["workload"]["overlap"],
                backend=aggregate.get("backend", "unknown"),
                elapsed=aggregate.get("elapsed_ns", 0),
                mean_step=aggregate.get("mean_step_ns", 0),
                allocations=aggregate.get("allocation_count", 0),
                throughput=aggregate.get("throughput_particles_per_second", 0.0),
                rss=aggregate.get("peak_rss_bytes", 0),
                error=aggregate.get("oracle_max_error", 0.0),
                scope=aggregate.get("scope", "unavailable"),
                reason=result["reason"] or "none",
            )
        )
    lines.extend(
        [
            "",
            "## GPU Qualification",
            "",
            "| Evidence | State | Reason |",
            "| --- | --- | --- |",
            f"| HIP/CUDA compile | {gpu['compile']['state']} | {gpu['compile']['reason']} |",
            f"| CPU fallback | {gpu['fallback']['state']} | {gpu['fallback']['reason']} |",
            f"| Physical GPU execution | {gpu['device_execution']['state']} | {gpu['device_execution']['reason']} |",
            "",
            "Raw command output is stored under `logs/`; this directory is external to the source tree.",
        ]
    )
    return "\n".join(lines) + "\n"


def write_artifacts(
    output: pathlib.Path, metadata: dict[str, Any], results: list[dict[str, Any]], gpu: dict[str, Any]
) -> None:
    output.mkdir(parents=True, exist_ok=True)
    (output / "logs").mkdir(parents=True, exist_ok=True)
    (output / "metadata.json").write_text(json.dumps({**metadata, "gpu": gpu}, indent=2) + "\n", encoding="utf-8")
    (output / "results.json").write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
    (output / "summary.md").write_text(render_summary(metadata, results, gpu), encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[2])
    parser.add_argument("--build-dir", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--timeout", type=int, default=240)
    parser.add_argument("--strict", action="store_true")
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()

    try:
        contract = load_contract(root)
        errors = validate_contract(contract)
        if errors:
            for error in errors:
                print(f"release-evidence: {error}", file=sys.stderr)
            return 1
        if arguments.check:
            print("release-evidence: scaling contract is valid")
            return 0
        if arguments.build_dir is None or arguments.output is None:
            parser.error("--build-dir and --output are required unless --check is used")
        output = arguments.output.resolve()
        if is_inside(output, root):
            raise ValueError("evidence output must be outside the source tree")
        build_dir = arguments.build_dir.resolve()
        metadata = collect_metadata(root, contract)
        if not metadata["revision"]:
            raise ValueError("Git revision could not be captured")
        executable = executable_path(build_dir, contract["target"])
        context = RunContext(root, output, executable, contract, arguments.timeout)
        results = [run_workload(context, workload) for workload in expand_workloads(contract)]
        add_overlap_comparisons(results)
        gpu = probe_gpu(root, build_dir, output)
        write_artifacts(output, metadata, results, gpu)
        failed = [result for result in results if result["state"] == "failed"]
        unresolved = [result for result in results if result["state"] == "unknown"]
        print(f"release-evidence: output={output}")
        print(f"release-evidence: passed={sum(r['state'] == 'passed' for r in results)}")
        print(f"release-evidence: skipped={sum(r['state'] == 'skipped' for r in results)}")
        print(f"release-evidence: failed={len(failed)} unknown={len(unresolved)}")
        return 1 if failed or (arguments.strict and unresolved) else 0
    except (OSError, ValueError, json.JSONDecodeError, KeyError) as error:
        print(f"release-evidence: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
