## Script Layout

- `doctor.cmake`: local toolchain/build artifact checks
- `deps_graphics.cmake`: install graphics dependencies through `vcpkg`
- `run_qt.sh`: run client host with Qt runtime vars on Unix
- `ci/nightly/`: coverage generation/publish helpers for nightly workflow
- `ci/release/package_bundle.py`: release artifact packager (shared by release workflow)
- `ci/release/package_evidence.py`: qualification-oriented release evidence packager
- `ci/release/package_quality_index.py`: audit entry-point summary for release review
- `ci/release/package_tool_manifest.py`: generated toolchain version manifest for CI evidence
- `help.txt`: text shown by `make help`

## Naming

- Keep scripts at `scripts/` root (no deep tree unless needed).
- Use short snake_case names.
