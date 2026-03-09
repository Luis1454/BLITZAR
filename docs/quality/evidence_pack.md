# Release Evidence Pack

Release candidates and major review milestones must publish a qualification-oriented evidence pack.

## Format

- Archive name: `CUDA-GRAVITY-SIMULATION-<tag>-evidence.zip`
- Machine-readable record: `release_evidence_pack.json`
- Human-readable summary: `README.md`
- Bundled referenced files: `evidence/<repo-relative-path>`

## Required Fields

- `format_version`: versioned pack format identifier.
- `tag`: reviewed tag or milestone label.
- `profile`: `prod` or `dev`.
- `generated_at_utc`: UTC timestamp of pack generation.
- `requirement_ids`: covered requirement IDs.
- `requirements`: requirement rows with test patterns and `EVD_*` refs.
- `verification_activities`: executed commands/checks for the reviewed lane.
- `analyzers`: analyzer statuses recorded for the pack.
- `ci_context`: workflow, ref, SHA, run identifiers, and lane source when available.
- `evidence_refs`: bundled evidence references resolved from `quality_manifest.json`.
- `open_exceptions`: open waivers/deviations resolved from the canonical register.

## Generation

- Local: `python -m python_tools.release.cli package_evidence --root . --dist-dir dist/evidence-pack --profile prod`
- CI: `.github/workflows/release-lane.yml`

## Notes

- The pack is generated from the canonical quality manifest and must not use raw file paths in requirement rows.
- `prod` packs are qualification-oriented evidence; `dev` packs are review aids only until reproduced under `prod` constraints.
- Reviewers should open the release-quality index first, then drill down into the evidence pack.
