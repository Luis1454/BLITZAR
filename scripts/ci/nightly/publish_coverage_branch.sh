#!/usr/bin/env bash
# File: scripts/ci/nightly/publish_coverage_branch.sh
# Purpose: Automation script for BLITZAR build, release, or operations tasks.

set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <coverage-dashboard-dir>" >&2
  exit 2
fi

payload_dir="$1"
if [[ ! -d "$payload_dir" ]]; then
  echo "coverage payload directory not found: $payload_dir" >&2
  exit 2
fi

: "${GITHUB_TOKEN:?GITHUB_TOKEN is required}"
: "${GITHUB_REPOSITORY:?GITHUB_REPOSITORY is required}"
: "${GITHUB_RUN_NUMBER:?GITHUB_RUN_NUMBER is required}"

repo_url="https://x-access-token:${GITHUB_TOKEN}@github.com/${GITHUB_REPOSITORY}.git"
publish_dir="$(mktemp -d)"

git config --global user.name "github-actions[bot]"
git config --global user.email "41898282+github-actions[bot]@users.noreply.github.com"

if git clone --depth 1 --branch coverage-data "$repo_url" "$publish_dir" 2>/dev/null; then
  echo "Reusing existing coverage-data branch"
else
  git init "$publish_dir"
  git -C "$publish_dir" checkout --orphan coverage-data
  git -C "$publish_dir" remote add origin "$repo_url"
fi

find "$publish_dir" -mindepth 1 -maxdepth 1 ! -name .git -exec rm -rf {} +
cp -R "$payload_dir"/. "$publish_dir"/
touch "$publish_dir/.nojekyll"

git -C "$publish_dir" add -A
if git -C "$publish_dir" diff --cached --quiet; then
  echo "Coverage payload unchanged"
  exit 0
fi

git -C "$publish_dir" commit -m "chore: update coverage payload for run ${GITHUB_RUN_NUMBER}"
git -C "$publish_dir" push origin HEAD:coverage-data --force
