"""Validate the frozen clean-room plan and its executable test contract."""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys

from tools.gates.naming_gate import validate as validate_naming_contract
from tools.architecture.architecture_sources import configured_paths, load_source_completeness


DEFAULT_ROOT = pathlib.Path(__file__).resolve().parents[2]
ROOT = DEFAULT_ROOT
MANIFEST = ROOT / "plan" / "manifest.json"
QUALITY = ROOT / "plan" / "quality.json"
OUTPUT_CONTRACT = ROOT / "plan" / "output_contract.json"
CMESSAGE = ROOT / "CMakeLists.txt"

TEST_ID_PATTERN = re.compile(r"^TST-[A-Z0-9]+(?:-[A-Z0-9]+)+$")
CHECK_ID_PATTERN = re.compile(r"^CHK-[A-Z0-9]+(?:-[A-Z0-9]+)+$")
TEST_PATTERN = re.compile(
    r"add_test\s*\(\s*NAME\s+([^\s\)]+)\s+COMMAND\s+([^\)]*)\)",
    re.MULTILINE,
)
SOURCE_SUFFIXES = {
    ".c",
    ".cpp",
    ".cu",
    ".cuh",
    ".hip",
    ".h",
    ".hpp",
    ".inl",
}
SEMVER_PATTERN = re.compile(r"^[0-9]+\.[0-9]+\.[0-9]+$")
INFORMATIONAL_ARCHITECTURE_RULES = (
    "function length and count",
    "branching complexity",
    "allocation behavior",
    "include dependencies",
    "responsibility boundaries",
)
ARCHITECTURE_PROFILES = {
    "production",
    "tests",
    "examples",
    "tools",
    "build-metadata",
    "documentation",
}
GENERIC_DETAIL_PATTERN = re.compile(
    r"\bnamespace\s+detail\b|"
    r"\bnamespace\s+[A-Za-z_]\w*\s*::\s*detail\b|"
    r"::\s*detail\s*::"
)
SHARED_PARTICLE_ARENA_PATTERN = re.compile(
    r"\bstd::shared_ptr\s*<\s*(?:[A-Za-z_]\w*::)*ParticleArena\s*>"
)


def fail(message: str) -> None:
    print(f"plan-check: {message}", file=sys.stderr)
    raise SystemExit(1)


def load_json(path: pathlib.Path, label: str) -> dict:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        fail(f"{label} is not valid JSON: {error}")
    if not isinstance(value, dict):
        fail(f"{label} must contain a JSON object")
    return value


def normalize_manifest_path(value: object, label: str) -> pathlib.PurePosixPath:
    if not isinstance(value, str) or not value:
        fail(f"{label} must contain non-empty relative paths")
    if any(part in {"", ".", ".."} for part in value.split("/")):
        fail(f"{label} contains unsafe path: {value}")
    path = pathlib.PurePosixPath(value)
    if path.is_absolute():
        fail(f"{label} contains unsafe path: {value}")
    if "\\" in value:
        fail(f"{label} must use forward slashes: {value}")
    return path


def validate_roots(data: dict) -> None:
    roots = data.get("roots")
    deferred = data.get("deferred_roots", [])
    if not isinstance(roots, list) or not roots:
        fail("roots must be a non-empty list")
    if not isinstance(deferred, list):
        fail("deferred_roots must be a list")

    root_paths = [normalize_manifest_path(root, "roots") for root in roots]
    deferred_paths = [
        normalize_manifest_path(root, "deferred_roots") for root in deferred
    ]
    all_paths = [path.as_posix().casefold() for path in root_paths + deferred_paths]
    if len(all_paths) != len(set(all_paths)):
        fail("roots and deferred_roots must be unique")

    for root in root_paths:
        directory = ROOT.joinpath(*root.parts)
        if not directory.is_dir():
            fail(f"materialized root is missing: {root}")
        if not any(directory.iterdir()):
            fail(f"materialized root is empty: {root}")

    for root in deferred_paths:
        directory = ROOT.joinpath(*root.parts)
        if directory.exists():
            fail(f"deferred root is materialized; promote it to roots: {root}")


def _manifest_paths(data: dict) -> list[pathlib.PurePosixPath]:
    return [
        normalize_manifest_path(root, "roots")
        for root in data.get("roots", [])
    ] + [
        normalize_manifest_path(root, "deferred_roots")
        for root in data.get("deferred_roots", [])
    ]


def _is_path_covered(
    path: pathlib.PurePosixPath,
    declared_paths: list[pathlib.PurePosixPath],
) -> bool:
    return any(
        path == declared or declared in path.parents
        for declared in declared_paths
    )


def validate_repository_layout(data: dict) -> None:
    declared_paths = _manifest_paths(data)
    plan_text = (ROOT / "PLAN.md").read_text(encoding="utf-8")
    shape_match = re.search(
        r"## Repository Shape\s+```text\n(?P<shape>.*?)\n```",
        plan_text,
        re.DOTALL,
    )
    if shape_match is None:
        fail("PLAN.md repository shape block is missing")

    shape_paths = {
        pathlib.PurePosixPath(match.group(1).rstrip("/"))
        for match in re.finditer(
            r"^((?:include|src|apps)(?:/[^\s/]+)*|tests|examples)/?\s{2,}",
            shape_match.group("shape"),
            re.MULTILINE,
        )
    }
    for path in shape_paths:
        if not _is_path_covered(path, declared_paths):
            fail(f"repository shape path is not covered by manifest: {path}")

    deferred_paths = [
        normalize_manifest_path(root, "deferred_roots")
        for root in data.get("deferred_roots", [])
    ]
    for path in deferred_paths:
        if path not in shape_paths:
            fail(f"deferred root is missing from PLAN.md repository shape: {path}")

    cmake_text = CMESSAGE.read_text(encoding="utf-8")
    for match in re.finditer(
        r"\b(src/[A-Za-z0-9_/-]+\.(?:c|cpp|cu|hip|h|hpp))\b", cmake_text
    ):
        source_path = pathlib.PurePosixPath(match.group(1))
        if not _is_path_covered(source_path.parent, declared_paths):
            fail(f"CMake source is not covered by manifest roots: {source_path}")

    for parent_name in ("src", "include", "apps", "tests", "examples"):
        parent = ROOT / parent_name
        if not parent.is_dir():
            continue
        for child in parent.iterdir():
            if not child.is_dir():
                continue
            path = pathlib.PurePosixPath(child.relative_to(ROOT).as_posix())
            if not _is_path_covered(path, declared_paths):
                fail(f"materialized production root is not declared: {path}")


def validate_phases(data: dict) -> set[str]:
    phases = data.get("phases")
    if not isinstance(phases, list) or not phases:
        fail("at least one phase is required")
    phase_ids = [phase.get("id") for phase in phases]
    expected_ids = [f"P{index}" for index in range(len(phases))]
    if phase_ids != expected_ids:
        fail("phase identifiers must be contiguous and ordered")
    if any(not phase.get("name") for phase in phases):
        fail("every phase needs a name")
    return set(phase_ids)


def validate_forbidden_references(data: dict) -> None:
    forbidden = data.get("forbidden_references")
    if not isinstance(forbidden, list) or not forbidden:
        fail("forbidden_references must be declared")
    code_roots = [
        ROOT / "src",
        ROOT / "include",
        ROOT / "apps",
        ROOT / "tests",
        ROOT / "examples",
    ]
    for directory in code_roots:
        if not directory.is_dir():
            continue
        for path in directory.rglob("*"):
            if not path.is_file():
                continue
            text = path.read_text(encoding="utf-8", errors="ignore").lower()
            for reference in forbidden:
                if str(reference).lower() in text:
                    fail(
                        f"{path.relative_to(ROOT)} contains forbidden reference: {reference}"
                    )


def validate_release_identity(data: dict) -> None:
    product_version = data.get("product_version")
    plan_version = data.get("plan_version")
    if not isinstance(product_version, str) or not SEMVER_PATTERN.fullmatch(
        product_version
    ):
        fail("manifest product_version must be a semantic version")
    if not isinstance(plan_version, str) or not SEMVER_PATTERN.fullmatch(
        plan_version
    ):
        fail("manifest plan_version must be a semantic version")

    plan_text = (ROOT / "PLAN.md").read_text(encoding="utf-8")
    if not re.search(
        rf"Product/API version:\s*\*\*{re.escape(product_version)}\*\*",
        plan_text,
    ):
        fail("PLAN.md product/API version does not match the manifest")
    if not re.search(
        rf"Plan version:\s*\*\*{re.escape(plan_version)}\*\*", plan_text
    ):
        fail("PLAN.md plan version does not match the manifest")

    cmake_text = CMESSAGE.read_text(encoding="utf-8")
    required_cmake_patterns = (
        r"file\(READ\s+\"\$\{CMAKE_CURRENT_SOURCE_DIR\}/plan/manifest\.json\"",
        r"string\(JSON\s+BLITZAR_PRODUCT_VERSION[\s\S]*?product_version\)",
        r"string\(JSON\s+BLITZAR_PLAN_VERSION[\s\S]*?plan_version\)",
        r"project\(BLITZAR\s+VERSION\s+\$\{BLITZAR_PRODUCT_VERSION\}",
    )
    if any(not re.search(pattern, cmake_text) for pattern in required_cmake_patterns):
        fail("CMake must derive product and plan versions from plan/manifest.json")

    config_path = ROOT / "cmake" / "blitzar_config.cmake.in"
    if not config_path.is_file():
        fail("cmake/blitzar_config.cmake.in is missing")
    config_text = config_path.read_text(encoding="utf-8")
    required_config_values = (
        'set(BLITZAR_VERSION "@PROJECT_VERSION@")',
        'set(BLITZAR_PRODUCT_VERSION "@PROJECT_VERSION@")',
        'set(BLITZAR_PLAN_VERSION "@BLITZAR_PLAN_VERSION@")',
    )
    if any(value not in config_text for value in required_config_values):
        fail("installed package config must expose product and plan versions")


EXPECTED_OUTPUT_CONTRACT = {
    "configuration": {
        "path_base": "config_parent",
        "directives": {
            "output": {
                "optional": True,
                "arguments": [
                    {"name": "directory", "type": "quoted_path", "required": True},
                    {
                        "name": "format",
                        "type": "enum(binary|hdf5)",
                        "required": False,
                        "default": "binary",
                    },
                    {"name": "every_steps", "type": "positive_int64", "required": True},
                    {"name": "write_initial", "type": "boolean", "required": True},
                    {"name": "write_final", "type": "boolean", "required": True},
                ],
            },
            "diagnostics": {
                "optional": True,
                "arguments": [
                    {"name": "every_steps", "type": "positive_int64", "required": True},
                    {"name": "energy", "type": "boolean", "required": True},
                    {"name": "momentum", "type": "boolean", "required": True},
                    {"name": "relative_error", "type": "boolean", "required": True},
                ],
            },
            "restart": {
                "optional": True,
                "arguments": [
                    {"name": "directory", "type": "quoted_path", "required": True},
                    {"name": "step", "type": "nonnegative_uint64", "required": True},
                ],
            },
            "execution": {
                "optional": True,
                "arguments": [
                    {"name": "mode", "type": "enum(strict|fast)", "required": True},
                ],
                "default": "strict",
            },
        },
    },
    "layout": {
        "manifest": "manifest.json",
        "state_directory": "states",
        "state_patterns": {
            "binary": "state-%08d.bin",
            "hdf5": "state-%08d.h5",
        },
        "state_shard_patterns": {
            "binary": "state-%08d.rank-%08d.bin",
            "hdf5": "state-%08d.rank-%08d.h5",
        },
        "default_state_format": "binary",
        "diagnostics_directory": "diagnostics",
        "diagnostics_file": "conservation.csv",
        "postprocess_directory": "postProcessing",
    },
    "manifest": {
        "schema_version": 1,
        "field_order": [
            "schema_version",
            "product_version",
            "plan_version",
            "configuration",
            "capabilities",
            "distribution",
            "completed_output_count",
            "completed_outputs",
        ],
        "configuration_order": [
            "simulation",
            "gravity",
            "units",
            "barnes_hut",
            "generation",
            "execution",
            "output",
            "diagnostics",
        ],
        "execution_order": [
            "mode",
            "cpu",
            "hip",
            "mpi",
            "precision",
            "compiler",
            "device",
            "rng",
            "compensator",
            "ordering",
            "bitwise_reproducible",
        ],
        "backend_execution_policy_order": ["fma", "reduction"],
        "capabilities_order": [
            "implemented_solver_mask",
            "unsupported_solver_mask",
            "deferred_feature_mask",
            "compiled_backend_mask",
        ],
        "distribution_order": ["rank_count", "rank_index"],
        "completed_output_order": ["step", "path|shards"],
        "state_path_patterns": {
            "binary": "states/state-%08d.bin",
            "hdf5": "states/state-%08d.h5",
        },
        "state_shard_path_patterns": {
            "binary": "states/state-%08d.rank-%08d.bin",
            "hdf5": "states/state-%08d.rank-%08d.h5",
        },
        "temporary_suffix": ".tmp",
        "atomic_publication": True,
        "wall_clock_metadata": False,
    },
    "snapshot": {
        "format": "binary",
        "version": 1,
        "magic": "BZRS",
        "endianness": "little",
        "scalar": "IEEE-754 binary64",
        "payload_order": [
            "ids",
            "position_x",
            "position_y",
            "position_z",
            "velocity_x",
            "velocity_y",
            "velocity_z",
            "mass",
        ],
        "header_order": [
            "magic",
            "version",
            "scalar_bytes",
            "particle_count",
            "step",
            "time",
            "rank_count",
            "rank_index",
            "endianness",
            "distribution",
            "id_policy",
        ],
        "wire_header_bytes": 43,
        "wire_payload_bytes_per_particle": 64,
        "wire_checksum_bytes": 8,
        "checksum_scope": "payload",
        "checksum": "FNV-1a-64",
        "atomic_publication": True,
        "max_particle_count": 100000,
        "mass_validation": "finite_nonnegative",
    },
    "semantics": {
        "single_rank": "implemented_first",
        "multi_rank": "explicit_rank_shards",
        "shards": {
            "manifest_owner": "rank_zero",
            "rank_file_count_per_step": "rank_count",
            "local_particle_count": True,
            "id_order": "strictly_increasing_global_stable_ids",
            "rank_width": 8,
            "online_diagnostics": "unsupported_until_global_reduction",
        },
        "initial_step": 0,
        "periodic_only": True,
        "final_write_no_duplicate": True,
        "overwrite": "reject_non_empty",
        "metadata_deterministic": True,
        "restart": {
            "source_manifest": "manifest.json",
            "state_path_patterns": {
                "binary": "states/state-%08d.bin",
                "hdf5": "states/state-%08d.h5",
            },
            "step_semantics": "absolute_final_run_step",
            "selection": "explicit_step",
            "compatibility": [
                "product_version",
                "plan_version",
                "solver",
                "integrator",
                "timestep",
                "gravity",
                "units",
                "barnes_hut",
                "generation",
                "execution",
                "scalar_bytes",
                "particle_count",
                "checksum",
                "ids",
                "distribution",
            ],
            "transactional": True,
            "distributed": "same_rank_count_shards",
            "postprocess_distributed": "explicit_shard_reconstruction",
            "hdf5": "supported-when-capability-available",
        },
        "timestamp": "excluded",
    },
    "diagnostics": {
        "format": "csv",
        "columns": [
            "step",
            "time",
            "particle_count",
            "kinetic_energy",
            "potential_energy",
            "total_energy",
            "momentum_x",
            "momentum_y",
            "momentum_z",
            "relative_energy_error",
            "relative_momentum_error",
        ],
        "precision": 17,
        "accumulation": "fixed_index_order",
    },
    "postprocess": {
        "cli_mode": "--post-process <run-directory>",
        "input_manifest": "manifest.json",
        "input_state_directory": "states",
        "output_directory": "postProcessing",
        "output_file": "conservation.csv",
        "state_order": "manifest_completed_outputs_strictly_increasing",
        "version_policy": "current_product_and_plan_and_snapshot_v1",
        "extra_files": "reject",
        "relative_error_reference": "first_emitted_record",
        "disabled_metrics": "nan",
        "atomic_publication": True,
    },
    "qualification": {
        "capture_schedule": "configured_output_checkpoints_only",
        "single_rank_mpi": "byte_equal_to_direct_cpu_cli",
        "multi_rank": "explicit_shards_before_manifest_publication",
        "hidden_full_gather": "forbidden_for_cli_output_and_postprocess",
        "snapshot_backend_headers": "mpi_and_hip_independent",
        "hip_device_path": "capability_gated",
        "rdma_path": "unverified",
        "timing": {
            "physics": "steady_clock_step_only",
            "output": "steady_clock_capture_and_publication",
            "persisted": False,
        },
    },
}


def validate_output_contract(data: dict, plan_version: str) -> None:
    if data.get("schema_version") != 1:
        fail("output contract schema_version must be 1")
    if data.get("plan_version") != plan_version:
        fail("output contract plan_version does not match the manifest")
    for section, expected in EXPECTED_OUTPUT_CONTRACT.items():
        if data.get(section) != expected:
            fail(f"output contract {section} is incomplete or inconsistent")


def validate_namespace_boundaries() -> None:
    code_roots = [
        ROOT / "src",
        ROOT / "include",
        ROOT / "apps",
        ROOT / "tests",
        ROOT / "examples",
    ]
    for directory in code_roots:
        if not directory.is_dir():
            continue
        for path in directory.rglob("*"):
            if not path.is_file() or path.suffix.lower() not in SOURCE_SUFFIXES:
                continue
            text = path.read_text(encoding="utf-8", errors="ignore")
            if GENERIC_DETAIL_PATTERN.search(text):
                fail(
                    f"generic nested detail namespace in "
                    f"{path.relative_to(ROOT)}"
                )


def validate_arena_ownership() -> None:
    code_roots = [
        ROOT / "src",
        ROOT / "include",
        ROOT / "apps",
        ROOT / "tests",
        ROOT / "examples",
    ]
    for directory in code_roots:
        if not directory.is_dir():
            continue
        for path in directory.rglob("*"):
            if not path.is_file() or path.suffix.lower() not in SOURCE_SUFFIXES:
                continue
            text = path.read_text(encoding="utf-8", errors="ignore")
            if SHARED_PARTICLE_ARENA_PATTERN.search(text):
                fail(
                    f"shared ParticleArena ownership in "
                    f"{path.relative_to(ROOT)}"
                )


def validate_quality_tests(phase_ids: set[str]) -> None:
    quality = load_json(QUALITY, "quality manifest")
    if quality.get("evidence_policy") != "registration-only":
        fail(
            "quality manifest evidence_policy must be 'registration-only'; "
            "runtime execution is validated separately"
        )
    tests = quality.get("tests")
    if not isinstance(tests, list) or not tests:
        fail("quality manifest needs named tests with IDs")

    commands: dict[str, str] = {}
    ids: set[str] = set()
    for test in tests:
        if not isinstance(test, dict):
            fail("every quality test must be an object")
        test_id = test.get("id")
        name = test.get("name")
        command = test.get("command")
        phase = test.get("phase")
        if (
            not isinstance(test_id, str)
            or not TEST_ID_PATTERN.fullmatch(test_id)
            or test_id in ids
        ):
            fail(f"invalid or duplicate quality test ID: {test_id}")
        if not isinstance(name, str) or not name:
            fail(f"quality test {test_id} needs a name")
        if (
            not isinstance(command, str)
            or not command
            or command in commands.values()
        ):
            fail(f"quality test {test_id} needs a unique command")
        if phase not in phase_ids:
            fail(f"quality test {test_id} references unknown phase: {phase}")
        ids.add(test_id)
        commands[test_id] = command

    def normalize_cmake_command(command: str) -> str:
        normalized = command
        normalized = normalized.replace("${MPIEXEC_EXECUTABLE}", "mpiexec")
        normalized = normalized.replace("${MPIEXEC_NUMPROC_FLAG}", "-np")
        normalized = normalized.replace("${MPIEXEC_PREFLAGS}", "")
        normalized = normalized.replace("${MPIEXEC_POSTFLAGS}", "")
        normalized = re.sub(
            r"\$<TARGET_FILE:([^>]+)>",
            r"\1",
            normalized,
        )
        return " ".join(normalized.split())

    configured_cmake = load_source_completeness(ROOT)
    cmake_text = "\n".join(
        path.read_text(encoding="utf-8")
        for path in configured_paths(ROOT, configured_cmake["cmake_files"])
    )
    cmake_tests = {
        test_id: normalize_cmake_command(command)
        for test_id, command in TEST_PATTERN.findall(
            cmake_text
        )
    }
    if set(cmake_tests) != ids:
        missing = sorted(ids - set(cmake_tests))
        extra = sorted(set(cmake_tests) - ids)
        fail(f"CTest manifest mismatch; missing={missing}, extra={extra}")
    for test_id, command in commands.items():
        if cmake_tests[test_id] != command:
            fail(
                f"CTest command mismatch for {test_id}: "
                f"manifest={command}, CMake={cmake_tests[test_id]}"
            )

    architecture = quality.get("architecture")
    if not isinstance(architecture, dict):
        fail("quality manifest needs an architecture report contract")
    report_schema = architecture.get("report_schema")
    if report_schema not in {1, 2}:
        fail("architecture report schema must be 1 or 2")
    if architecture.get("line_count_policy") != "informational":
        fail("architecture line_count_policy must remain informational")
    command = architecture.get("command")
    if (
        not isinstance(command, str)
        or "tools.architecture.architecture_report" not in command
        or "--check" not in command
    ):
        fail("architecture report command must run the architecture_report module with --check")
    if report_schema == 2 and "--all" not in command:
        fail("architecture report schema 2 must qualify all repository profiles")

    if report_schema == 2:
        profiles = architecture.get("profiles")
        if not isinstance(profiles, dict) or set(profiles) != ARCHITECTURE_PROFILES:
            fail("architecture profiles must cover the complete repository")
        for profile_name, profile in profiles.items():
            if not isinstance(profile, dict):
                fail(f"architecture profile {profile_name} must be an object")
            roots = profile.get("roots", [])
            paths = profile.get("paths", [])
            suffixes = profile.get("suffixes", [])
            if (
                not isinstance(roots, list)
                or not isinstance(paths, list)
                or not isinstance(suffixes, list)
                or not roots and not paths
                or not all(isinstance(item, str) and item for item in roots + paths + suffixes)
            ):
                fail(f"architecture profile {profile_name} has invalid paths or suffixes")
        source_completeness = architecture.get("source_completeness")
        if not isinstance(source_completeness, dict):
            fail("architecture source_completeness contract is missing")
        for key in ("cmake_files", "source_roots", "suffixes"):
            value = source_completeness.get(key)
            if not isinstance(value, list) or not value or not all(
                isinstance(item, str) and item for item in value
            ):
                fail(f"architecture source_completeness.{key} is invalid")

        naming = architecture.get("naming")
        if not isinstance(naming, dict):
            fail("architecture naming contract is missing")
        max_lengths = naming.get("max_filename_length")
        if (
            not isinstance(max_lengths, dict)
            or set(max_lengths) != ARCHITECTURE_PROFILES
            or any(not isinstance(value, int) or value <= 0 for value in max_lengths.values())
        ):
            fail("architecture naming filename limits are incomplete")
        stem_pairs = naming.get("allowed_stem_pairs")
        if not isinstance(stem_pairs, list) or not stem_pairs:
            fail("architecture naming stem pairs are missing")
        for pair in stem_pairs:
            if (
                not isinstance(pair, dict)
                or not isinstance(pair.get("extensions"), list)
                or len(pair["extensions"]) != 2
                or not all(isinstance(item, str) and item.startswith(".") for item in pair["extensions"])
                or not isinstance(pair.get("reason"), str)
                or not pair["reason"]
            ):
                fail("architecture naming stem pair is invalid")
        for key in (
            "name_exceptions",
            "type_mapping_exceptions",
            "standalone_implementation_exceptions",
        ):
            value = naming.get(key)
            if not isinstance(value, list):
                fail(f"architecture naming {key} is invalid")
            for exception in value:
                if (
                    not isinstance(exception, dict)
                    or not isinstance(exception.get("path"), str)
                    or not exception["path"]
                    or not isinstance(exception.get("reason"), str)
                    or not exception["reason"]
                ):
                    fail(f"architecture naming {key} entry is invalid")
        responsibility_prefixes = naming.get("responsibility_prefixes")
        if not isinstance(responsibility_prefixes, list) or not responsibility_prefixes:
            fail("architecture naming responsibility prefixes are missing")
        seen_responsibility_paths: set[str] = set()
        for entry in responsibility_prefixes:
            path = entry.get("path") if isinstance(entry, dict) else None
            prefix = entry.get("prefix") if isinstance(entry, dict) else None
            if (
                not isinstance(path, str)
                or not re.fullmatch(r"[A-Za-z0-9_/-]+", path)
                or path in seen_responsibility_paths
                or not isinstance(prefix, str)
                or re.fullmatch(r"[A-Z][A-Za-z0-9]*", prefix) is None
            ):
                fail("architecture naming responsibility prefix entry is invalid")
            seen_responsibility_paths.add(path)
        for key in ("forbidden_path_components", "forbidden_name_prefixes"):
            value = naming.get(key)
            if not isinstance(value, list) or not value or not all(
                isinstance(item, str) and item for item in value
            ):
                fail(f"architecture naming {key} is invalid")

        formatting = quality.get("format")
        if not isinstance(formatting, dict):
            fail("format contract is missing")
        if (
            formatting.get("command") != "python -m tools.format.format_clang_gate --root . --check"
            or formatting.get("clang_format_version") != "22.1.8"
            or formatting.get("encoding") != "UTF-8"
            or formatting.get("bom") is not False
            or formatting.get("line_endings") != "LF"
            or formatting.get("final_newline") is not True
            or formatting.get("custom_grouping_command")
            != "python -m tools.format.format_blocks --check"
        ):
            fail("format contract is incomplete or inconsistent")

    registry = architecture.get("review_registry")
    registry_path = normalize_manifest_path(registry, "architecture.review_registry")
    if not ROOT.joinpath(*registry_path.parts).is_file():
        fail(f"architecture review registry is missing: {registry_path}")

    thresholds = architecture.get("thresholds")
    required_thresholds = {
        "max_parameters",
        "max_function_lines",
        "max_functions_per_file",
        "max_branch_points",
        "max_allocation_sites",
        "max_internal_includes",
    }
    if not isinstance(thresholds, dict) or set(thresholds) != required_thresholds:
        fail("architecture thresholds are incomplete")
    if thresholds.get("max_parameters") != 4:
        fail("architecture max_parameters must remain 4")
    if any(
        not isinstance(thresholds[name], int) or thresholds[name] <= 0
        for name in required_thresholds
    ):
        fail("architecture thresholds must be positive integers")

    checks = quality.get("checks")
    if not isinstance(checks, list) or not checks:
        fail("quality manifest needs executable quality checks")
    check_ids: set[str] = set()
    check_commands: set[str] = set()
    for check in checks:
        if not isinstance(check, dict):
            fail("every quality check must be an object")
        check_id = check.get("id")
        check_name = check.get("name")
        check_command = check.get("command")
        if (
            not isinstance(check_id, str)
            or not CHECK_ID_PATTERN.fullmatch(check_id)
            or check_id in check_ids
        ):
            fail(f"invalid or duplicate quality check ID: {check_id}")
        if not isinstance(check_name, str) or not check_name:
            fail(f"quality check {check_id} needs a name")
        if (
            not isinstance(check_command, str)
            or not check_command
            or check_command in check_commands
        ):
            fail(f"quality check {check_id} needs a unique command")
        if check.get("phase") not in phase_ids:
            fail(f"quality check {check_id} references unknown phase")
        check_ids.add(check_id)
        check_commands.add(check_command)


def validate_naming() -> None:
    for violation in validate_naming_contract(ROOT):
        fail(violation)


def configure_root(root: pathlib.Path) -> None:
    global ROOT, MANIFEST, QUALITY, OUTPUT_CONTRACT, CMESSAGE
    ROOT = root.resolve()
    MANIFEST = ROOT / "plan" / "manifest.json"
    QUALITY = ROOT / "plan" / "quality.json"
    OUTPUT_CONTRACT = ROOT / "plan" / "output_contract.json"
    CMESSAGE = ROOT / "CMakeLists.txt"


def main(argv: list[str] | None = None) -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=pathlib.Path,
        default=DEFAULT_ROOT,
        help="repository root to validate",
    )
    args = parser.parse_args(argv)
    configure_root(args.root)

    if not (ROOT / "PLAN.md").is_file():
        fail("PLAN.md is missing")
    if not MANIFEST.is_file():
        fail("plan/manifest.json is missing")
    if not QUALITY.is_file():
        fail("plan/quality.json is missing")
    if not OUTPUT_CONTRACT.is_file():
        fail("plan/output_contract.json is missing")
    if not CMESSAGE.is_file():
        fail("CMakeLists.txt is missing")

    manifest = load_json(MANIFEST, "manifest")
    if manifest.get("status") != "frozen":
        fail("manifest status must remain 'frozen'")
    if not manifest.get("plan_version"):
        fail("plan_version is required")
    validate_release_identity(manifest)
    validate_output_contract(
        load_json(OUTPUT_CONTRACT, "output contract"), manifest["plan_version"]
    )
    validate_roots(manifest)
    validate_repository_layout(manifest)
    phase_ids = validate_phases(manifest)
    validate_forbidden_references(manifest)
    validate_quality_tests(phase_ids)
    validate_namespace_boundaries()
    validate_arena_ownership()
    validate_naming()
    print(
        f"plan-check: frozen plan {manifest['plan_version']} is valid; "
        "CTest entries are registered only, runtime evidence is not claimed"
    )
    print(
        "plan-check: informational architecture rules: "
        + ", ".join(INFORMATIONAL_ARCHITECTURE_RULES)
    )


if __name__ == "__main__":
    main()
