# Tool Qualification Manifest

CI evidence lanes must emit a tool manifest describing runner OS and toolchain versions.

## Required Fields

- runner OS metadata
- Python version
- CMake version
- compiler version or explicit unavailability
- `clang-tidy` version or explicit unavailability
- CI run references when available

## Retrieval

- PR quality lane artifact: `tool-qualification-pr-fast-<run>`
- Release lane artifact: `tool-qualification-release-<tag-or-run>`
- Release bundle copy: `tool_manifest.json`

## Generation

- Local: `python -m python_tools.release.cli tool_manifest --lane manual --profile prod`
- CI: `pr-fast.yml` and `release-lane.yml`
