#!/usr/bin/env bash
set -euo pipefail

gcovr \
  --root "${GITHUB_WORKSPACE}" \
  --object-directory build-integration-cov \
  --filter "engine/src" \
  --filter "runtime/src" \
  --exclude "build-integration-cov/_deps" \
  --exclude "tests/" \
  --xml-pretty \
  --output build-integration-cov/coverage.xml \
  --html-details build-integration-cov/coverage.html

gcovr \
  --root "${GITHUB_WORKSPACE}" \
  --object-directory build-integration-cov \
  --filter "engine/src" \
  --filter "runtime/src" \
  --exclude "build-integration-cov/_deps" \
  --exclude "tests/" \
  --print-summary | tee build-integration-cov/coverage-summary.txt

LINES_PCT="$(awk '/^lines:/{gsub("%","",$2); print $2}' build-integration-cov/coverage-summary.txt)"
FUNCS_PCT="$(awk '/^functions:/{gsub("%","",$2); print $2}' build-integration-cov/coverage-summary.txt)"
BRANCH_PCT="$(awk '/^branches:/{gsub("%","",$2); print $2}' build-integration-cov/coverage-summary.txt)"

echo "lines_pct=${LINES_PCT}" >> "$GITHUB_OUTPUT"
echo "funcs_pct=${FUNCS_PCT}" >> "$GITHUB_OUTPUT"
echo "branch_pct=${BRANCH_PCT}" >> "$GITHUB_OUTPUT"

{
  echo "## Integration coverage percentages"
  echo ""
  echo "| Metric | Percent |"
  echo "|---|---:|"
  echo "| Lines | ${LINES_PCT}% |"
  echo "| Functions | ${FUNCS_PCT}% |"
  echo "| Branches | ${BRANCH_PCT}% |"
  echo ""
  echo "Raw summary:"
  echo '```text'
  cat build-integration-cov/coverage-summary.txt
  echo '```'
} >> "$GITHUB_STEP_SUMMARY"

echo "::notice title=Coverage percentages::lines=${LINES_PCT}% functions=${FUNCS_PCT}% branches=${BRANCH_PCT}%"
