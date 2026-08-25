"""Check canonical encoding, line endings, and clang-format output."""

from __future__ import annotations

import argparse
import os
import pathlib
import shutil
import subprocess
import sys


SOURCE_ROOTS = ("include", "src", "apps", "tests", "examples")
SOURCE_SUFFIXES = {
    ".c",
    ".cc",
    ".cpp",
    ".cu",
    ".cuh",
    ".h",
    ".hip",
    ".hpp",
    ".inl",
}
CLANG_FORMAT_VERSION = "22.1.8"


def source_files(root: pathlib.Path) -> list[pathlib.Path]:
    return sorted(
        path
        for source_root in SOURCE_ROOTS
        for path in (root / source_root).rglob("*")
        if path.is_file()
        and path.suffix.lower() in SOURCE_SUFFIXES
        and not any(part == ".git" or part == "build" or part.startswith("build-") for part in path.parts)
    )


def encoding_violations(path: pathlib.Path) -> list[str]:
    data = path.read_bytes()
    relative = path.as_posix()
    violations: list[str] = []
    if data.startswith(b"\xef\xbb\xbf"):
        violations.append(f"{relative}: UTF-8 BOM")
    try:
        data.decode("utf-8")
    except UnicodeDecodeError as error:
        violations.append(f"{relative}: invalid UTF-8 ({error})")
    if b"\r" in data:
        violations.append(f"{relative}: non-LF line ending")
    if data and not data.endswith(b"\n"):
        violations.append(f"{relative}: missing final newline")
    return violations


def formatter_candidates() -> list[pathlib.Path]:
    candidates: list[pathlib.Path] = []
    configured = os.environ.get("CLANG_FORMAT")
    if configured:
        candidates.append(pathlib.Path(configured))
    located = shutil.which("clang-format")
    if located:
        candidates.append(pathlib.Path(located))
    user_scripts = pathlib.Path(os.environ.get("APPDATA", "")) / "Python"
    if user_scripts.is_dir():
        candidates.extend(user_scripts.glob("Python*/Scripts/clang-format.exe"))
    candidates.append(pathlib.Path(sys.prefix) / "Scripts" / "clang-format.exe")
    candidates.append(pathlib.Path(sys.prefix) / "bin" / "clang-format")
    return candidates


def find_formatter(explicit: str | None = None) -> pathlib.Path:
    candidates = [pathlib.Path(explicit)] if explicit else formatter_candidates()
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise FileNotFoundError(
        "clang-format is not installed; set CLANG_FORMAT or pass --clang-format"
    )


def format_violations(
    root: pathlib.Path,
    files: list[pathlib.Path],
    formatter: pathlib.Path,
) -> list[str]:
    violations: list[str] = []
    for path in files:
        result = subprocess.run(
            [str(formatter), "--dry-run", "--Werror", "--style=file", str(path)],
            cwd=root,
            check=False,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            detail = (result.stderr or result.stdout).strip().replace("\n", " ")
            violations.append(f"{path.relative_to(root).as_posix()}: {detail}")
    return violations


def validate(root: pathlib.Path, explicit_formatter: str | None = None) -> tuple[str, list[str]]:
    formatter = find_formatter(explicit_formatter)
    version = subprocess.check_output([str(formatter), "--version"], text=True).strip()
    files = source_files(root)
    violations = [item for path in files for item in encoding_violations(path)]
    if CLANG_FORMAT_VERSION not in version:
        violations.append(
            f"formatter version mismatch: expected {CLANG_FORMAT_VERSION}, got {version}"
        )
    violations.extend(format_violations(root, files, formatter))
    return version, violations


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[1])
    parser.add_argument("--clang-format")
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args(argv)
    if not arguments.check:
        parser.error("--check is required; this gate never rewrites files")
    try:
        version, violations = validate(arguments.root.resolve(), arguments.clang_format)
    except (OSError, subprocess.SubprocessError) as error:
        print(f"clang-format-gate: {error}", file=sys.stderr)
        return 1
    for violation in violations:
        print(f"clang-format-gate: {violation}", file=sys.stderr)
    print(
        f"clang-format-gate: {len(source_files(arguments.root.resolve()))} files, "
        f"formatter={version}, violations={len(violations)}",
        file=sys.stderr,
    )
    return int(bool(violations))


if __name__ == "__main__":
    sys.exit(main())
