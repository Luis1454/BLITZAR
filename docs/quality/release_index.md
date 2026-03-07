# Release Quality Index

The release-quality index is the audit entry point for a release candidate or review milestone.

## Format

- Archive name: `CUDA-GRAVITY-SIMULATION-<tag>-quality-index.zip`
- Machine-readable record: `release_quality_index.json`
- Human-readable summary: `README.md`

## Required Content

- reviewed `tag` and `profile`
- summarized requirement IDs for the release scope
- major evidence references used for the review
- analyzer and verification activity status summary
- unresolved deviations linked to the reviewed requirements
- CI context for the reviewed lane when available

## Generation

- Local: `python scripts/ci/release/package_quality_index.py --root . --dist-dir dist/release-quality-index --profile prod`
- CI: `.github/workflows/release-lane.yml`

## Maintainer Workflow

- Generate or refresh the index whenever a release candidate tag or major review milestone is prepared.
- Treat `release_quality_index.json` as the first audit document before opening the full evidence pack.
