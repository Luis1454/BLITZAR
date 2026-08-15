#!/usr/bin/env python3
"""Prepare, inspect, validate, run, and report a BLITZAR case."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
from datetime import UTC, datetime
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_TEMPLATE = REPO_ROOT / "simulation.ini"


def sha256(path: Path) -> str | None:
    if not path.is_file():
        return None
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def resolve_case(value: str) -> Path:
    return Path(value).expanduser().resolve()


def config_path(case: Path) -> Path:
    return case / "simulation.ini"


def relative_runtime_path(path: Path) -> str:
    try:
        return "/workspace/" + path.relative_to(REPO_ROOT).as_posix()
    except ValueError as error:
        raise RuntimeError("Docker cases must be located inside the BLITZAR repository") from error


def native_binary(value: str | None) -> str:
    candidates = [
        Path(value) if value else None,
        REPO_ROOT / "build-prod" / "blitzar-headless",
        REPO_ROOT / "build" / "blitzar-headless",
        REPO_ROOT / "dist" / "local-linux" / "blitzar-headless-linux-x86_64",
    ]
    for candidate in candidates:
        if candidate and candidate.is_file() and not (
            sys.platform == "win32" and candidate.suffix.lower() not in {".exe", ".com"}
        ):
            return str(candidate.resolve())
    found = shutil.which(value or "blitzar-headless")
    if found:
        return found
    if sys.platform == "win32":
        raise RuntimeError("Windows headless binary not found; build/pass blitzar-headless.exe or use --runtime docker")
    raise RuntimeError("headless binary not found; pass --binary or use --runtime docker")


def command_for(args: argparse.Namespace, action: str, case: Path, output: Path | None) -> list[str]:
    config = config_path(case)
    if not config.is_file():
        raise RuntimeError(f"case configuration not found: {config}")
    if args.runtime == "docker":
        config_argument = relative_runtime_path(config)
        command = [
            args.docker,
            "run",
            "--rm",
            "--init",
            "--mount",
            f"type=bind,source={REPO_ROOT},target=/workspace",
            "-w",
            "/workspace",
            args.docker_image,
            "/blitzar/blitzar-headless",
        ]
    else:
        config_argument = str(config)
        command = [native_binary(args.binary)]
    command += [f"--{action}", "--config", config_argument]
    for flag, value in (("--profile", args.profile), ("--solver", args.solver), ("--integrator", args.integrator)):
        if value:
            command += [flag, value]
    if args.deterministic is not None:
        command += ["--deterministic", "true" if args.deterministic else "false"]
    if action == "run":
        command += [
            "--target-steps",
            str(args.steps),
            "--export-on-exit",
            "true",
            "--export-format",
            args.format,
        ]
        if args.particle_count is not None:
            command += ["--particle-count", str(args.particle_count)]
        if args.init_seed is not None:
            command += ["--init-seed", str(args.init_seed)]
        if output is None:
            raise RuntimeError("run output path is required")
        command += ["--export-path", relative_runtime_path(output) if args.runtime == "docker" else str(output)]
    command += list(args.extra or [])
    return command


def run_action(args: argparse.Namespace, action: str) -> int:
    case = resolve_case(args.case)
    output = case / "outputs" / args.output if action == "run" else None
    if output:
        output.parent.mkdir(parents=True, exist_ok=True)
    command = command_for(args, action, case, output)
    log_path = case / "logs" / f"{action}.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    result = subprocess.run(command, cwd=REPO_ROOT, capture_output=True, text=True, check=False)
    transcript = result.stdout + result.stderr
    log_path.write_text(transcript, encoding="utf-8")
    print(transcript, end="")
    manifest = {
        "action": action,
        "case": str(case),
        "command": command,
        "returncode": result.returncode,
        "config_sha256": sha256(config_path(case)),
        "output": str(output) if output else None,
        "output_sha256": sha256(output) if output else None,
        "created_utc": datetime.now(UTC).isoformat(),
    }
    manifest_path = case / "manifests" / f"{action}.json"
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"[case] log={log_path}")
    print(f"[case] manifest={manifest_path}")
    return result.returncode


def initialize_case(args: argparse.Namespace) -> int:
    case = resolve_case(args.case)
    config = config_path(case)
    if config.exists() and not args.force:
        raise RuntimeError(f"case already exists: {config}; pass --force to replace it")
    template = Path(args.template).expanduser().resolve() if args.template else DEFAULT_TEMPLATE
    if not template.is_file():
        raise RuntimeError(f"template not found: {template}")
    case.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(template, config)
    for directory in ("outputs", "logs", "manifests"):
        (case / directory).mkdir(exist_ok=True)
    print(f"[case] initialized {case}")
    print(f"[case] configuration={config}")
    return 0


def report_case(args: argparse.Namespace) -> int:
    case = resolve_case(args.case)
    manifest_path = case / "manifests" / "run.json"
    if not manifest_path.is_file():
        raise RuntimeError(f"run manifest not found: {manifest_path}")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    output = Path(manifest["output"]) if manifest.get("output") else None
    manifest["output_sha256_current"] = sha256(output) if output else None
    print(json.dumps(manifest, indent=2))
    return 0 if manifest.get("returncode") == 0 and manifest.get("output_sha256_current") else 5


def parser() -> argparse.ArgumentParser:
    root = argparse.ArgumentParser(description="BLITZAR case workflow")
    subparsers = root.add_subparsers(dest="command", required=True)
    init = subparsers.add_parser("init", help="create a case directory")
    init.add_argument("case")
    init.add_argument("--template")
    init.add_argument("--force", action="store_true")
    for action in ("inspect", "validate", "run"):
        item = subparsers.add_parser(action, help=f"{action} a case")
        item.add_argument("--case", required=True)
        item.add_argument("--runtime", choices=("native", "docker"), default="native")
        item.add_argument("--binary")
        item.add_argument("--docker", default="docker")
        item.add_argument("--docker-image", default="blitzar-cpu:local")
        item.add_argument("--profile")
        item.add_argument("--solver")
        item.add_argument("--integrator")
        item.add_argument("--deterministic", action=argparse.BooleanOptionalAction, default=None)
        if action == "run":
            item.add_argument("--steps", type=int, default=100)
            item.add_argument("--particle-count", type=int)
            item.add_argument("--init-seed", type=int)
            item.add_argument("--format", choices=("xyz", "vtk", "vtk_binary", "bin"), default="xyz")
            item.add_argument("--output", default="final.xyz")
        item.add_argument("extra", nargs=argparse.REMAINDER)
    report = subparsers.add_parser("report", help="show run provenance and output hash")
    report.add_argument("--case", required=True)
    return root


def main() -> int:
    args = parser().parse_args()
    try:
        if args.command == "init":
            return initialize_case(args)
        if args.command == "report":
            return report_case(args)
        return run_action(args, args.command)
    except (OSError, RuntimeError, ValueError, json.JSONDecodeError) as error:
        print(f"[case] error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
