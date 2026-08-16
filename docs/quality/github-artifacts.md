# GitHub Artifact Policy

BLITZAR has three different artifact authorities. They must not be treated as interchangeable.

| Class | Prefixes | Retention | Authority |
| --- | --- | ---: | --- |
| Pull request | `pr-*`, `tool-qualification-pr-*` | 7 days | CI diagnostics only |
| Nightly evidence | `nightly-*`, `release-lane-logs-*` | 14 days | CI diagnostics only |
| GPU health | `gpu-runner-health-*` | 7 days | CI readiness diagnostics only |
| Release staging | `release-*`, `desktop-installer-*` | 30 days | Temporary staging; GitHub Release assets are authoritative |
| Release qualification | `tool-qualification-release-*` | 30 days | Temporary release qualification staging |
| GitHub Pages | `github-pages` | Managed by GitHub | Pages deployment artifact; not an Actions retention class |

Published GitHub Release assets are the distribution authority. Release workflow artifacts are
temporary copies used to transfer and verify the bundle; they must not be presented as a second
release channel. Unclassified artifacts are a policy finding and require an explicit classification
before a workflow is merged.

The inventory command queries the live repository and reports counts, bytes, oldest artifact,
expired records, stale entries, and unclassified names:

```text
python scripts/ci/audit_github_artifacts.py \
  --repo Luis1454/BLITZAR \
  --token "$GITHUB_TOKEN" \
  --fail-on-stale \
  --fail-on-unclassified
```

`--fail-on-stale` returns exit code `2`. `--fail-on-unclassified` returns exit code `3`.
The latter is the mandatory policy gate for new artifact names. Expired records remain visible in
the GitHub API and in the report, but are counted separately and are excluded from stale retention
violations.

The local inventory is read-only and reports generated trees, binary counts, sizes, and timestamps:

```text
python scripts/ci/audit_local_artifacts.py --root . --output dist/local-artifacts.json
```

Local `build-*`, `dist/`, `artifacts/`, `exports/`, and `outputs/` directories are disposable
developer outputs. They are ignored by Git and are not release authority. Keep only active build
trees and the latest evidence needed for the current investigation. Deletion remains an explicit
operator action after reviewing the report; no CI job deletes local workspace data.
