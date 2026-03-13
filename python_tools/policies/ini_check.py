#!/usr/bin/env python3
from __future__ import annotations

import re

from python_tools.core.base_check import BaseCheck
from python_tools.core.models import CheckContext, CheckResult

INT_RE = re.compile(r"^-?[0-9]+$")
FLOAT_RE = re.compile(r"^-?([0-9]+(\.[0-9]*)?|\.[0-9]+)([eE][-+]?[0-9]+)?$")


class IniCheck(BaseCheck):
    name = "ini"
    success_message = "simulation.ini validation OK"
    failure_title = "simulation.ini validation failed:"

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        config_path = context.config
        if config_path is None or not config_path.exists():
            result.add_error(f"missing config file: {config_path}")
            return
        values = self._parse_ini(config_path.read_text(encoding="utf-8"))
        self._check_required_keys(values, result)
        self._check_int_constraints(values, result)
        self._check_float_constraints(values, result)
        self._check_enum_constraints(values, result)

    def _parse_ini(self, content: str) -> dict[str, str]:
        values: dict[str, str] = {}
        for raw_line in content.splitlines():
            line = raw_line.strip()
            if not line or line.startswith("#") or line.startswith(";"):
                continue
            if self._parse_directive(line, values):
                continue
            if "=" not in line:
                continue
            key, value = line.split("=", 1)
            key = key.strip()
            if key and key not in values:
                values[key] = value.strip()
        return values

    def _parse_directive(self, line: str, values: dict[str, str]) -> bool:
        if "(" not in line or not line.endswith(")"):
            return False
        directive, raw_args = line.split("(", 1)
        directive = directive.strip()
        body = raw_args[:-1]
        if not directive:
            return False
        args: dict[str, str] = {}
        token = ""
        quote = ""
        for char in body:
            if quote:
                if char == quote:
                    quote = ""
                token += char
                continue
            if char in {"'", '"'}:
                quote = char
                token += char
                continue
            if char == ",":
                self._consume_directive_arg(token, args)
                token = ""
                continue
            token += char
        self._consume_directive_arg(token, args)
        mapping = {
            "simulation": {
                "particle_count": "particle_count",
                "particles": "particle_count",
                "dt": "dt",
                "solver": "solver",
                "integrator": "integrator",
            },
            "substeps": {"target_dt": "substep_target_dt", "max": "max_substeps"},
            "octree": {"theta": "octree_theta", "softening": "octree_softening"},
            "client": {
                "draw_cap": "client_particle_cap",
                "ui_fps": "ui_fps_limit",
                "command_timeout_ms": "client_remote_command_timeout_ms",
                "status_timeout_ms": "client_remote_status_timeout_ms",
                "snapshot_timeout_ms": "client_remote_snapshot_timeout_ms",
            },
            "export": {"format": "export_format", "directory": "export_directory"},
            "scene": {
                "style": "init_config_style",
                "preset": "preset_structure",
                "mode": "init_mode",
                "format": "input_format",
            },
            "sph": {"enabled": "sph_enabled"},
            "energy": {"every_steps": "energy_measure_every_steps", "sample_limit": "energy_sample_limit"},
        }
        directive_map = mapping.get(directive)
        if directive_map is None:
            return True
        for key, target in directive_map.items():
            if key in args and target not in values:
                values[target] = args[key]
        return True

    def _consume_directive_arg(self, token: str, values: dict[str, str]) -> None:
        entry = token.strip()
        if not entry or "=" not in entry:
            return
        key, value = entry.split("=", 1)
        cleaned = value.strip()
        if len(cleaned) >= 2 and cleaned[0] == cleaned[-1] and cleaned[0] in {"'", '"'}:
            cleaned = cleaned[1:-1]
        values[key.strip()] = cleaned

    def _check_required_keys(self, values: dict[str, str], result: CheckResult) -> None:
        required_keys = (
            "particle_count", "dt", "solver", "integrator", "octree_theta", "octree_softening",
            "client_particle_cap", "ui_fps_limit", "export_format", "input_format", "init_config_style",
            "preset_structure", "init_mode", "sph_enabled",
        )
        for key in required_keys:
            if key not in values:
                result.add_error(f"{key}: missing key")

    def _check_int_constraints(self, values: dict[str, str], result: CheckResult) -> None:
        for key, minimum in (
            ("particle_count", 1), ("client_particle_cap", 1), ("ui_fps_limit", 1),
            ("energy_measure_every_steps", 1), ("energy_sample_limit", 1),
            ("client_remote_command_timeout_ms", 10), ("client_remote_status_timeout_ms", 10),
            ("client_remote_snapshot_timeout_ms", 10),
        ):
            raw = values.get(key)
            if raw is None:
                continue
            if not INT_RE.match(raw):
                result.add_error(f"{key}: expected integer, got '{raw}'")
                continue
            if int(raw) < minimum:
                result.add_error(f"{key}: expected >= {minimum}, got {raw}")

    def _check_float_constraints(self, values: dict[str, str], result: CheckResult) -> None:
        for key, minimum in (
            ("dt", 0.0), ("octree_theta", 0.0), ("octree_softening", 0.0), ("default_zoom", 0.0), ("default_luminosity", 0.0),
        ):
            raw = values.get(key)
            if raw is None:
                continue
            if not FLOAT_RE.match(raw):
                result.add_error(f"{key}: expected float, got '{raw}'")
                continue
            if float(raw) < minimum:
                result.add_error(f"{key}: expected >= {minimum}, got {raw}")

    def _check_enum_constraints(self, values: dict[str, str], result: CheckResult) -> None:
        enums = (
            ("solver", ("pairwise_cuda", "octree_gpu", "octree_cpu")),
            ("integrator", ("euler", "rk4")),
            ("export_format", ("vtk", "vtk_binary", "xyz", "bin")),
            ("input_format", ("auto", "vtk", "vtk_binary", "xyz", "bin")),
            ("init_config_style", ("preset", "detailed")),
            ("preset_structure", ("disk_orbit", "random_cloud", "file")),
            ("init_mode", ("disk_orbit", "random_cloud", "file")),
            ("sph_enabled", ("true", "false")),
        )
        for key, options in enums:
            raw = values.get(key)
            if raw is None:
                continue
            if raw not in options:
                result.add_error(f"{key}: expected one of [{', '.join(options)}], got '{raw}'")

