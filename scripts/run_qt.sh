#!/usr/bin/env bash
# File: scripts/run_qt.sh
# Purpose: Automation script for BLITZAR build, release, or operations tasks.

set -euo pipefail

BIN=""
CONFIG=""
MODULE="qt"
QT_DIR=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --bin)
            BIN="$2"
            shift 2
            ;;
        --config)
            CONFIG="$2"
            shift 2
            ;;
        --module)
            MODULE="$2"
            shift 2
            ;;
        --qt-dir)
            QT_DIR="$2"
            shift 2
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 2
            ;;
    esac
done

if [[ -z "${BIN}" ]]; then
    echo "Missing --bin" >&2
    exit 2
fi
if [[ -z "${CONFIG}" ]]; then
    echo "Missing --config" >&2
    exit 2
fi
if [[ -z "${QT_DIR}" ]]; then
    echo "Missing --qt-dir" >&2
    exit 2
fi

QT_PLUGIN_PATH="${QT_DIR}/plugins"
QT_LIB_DIR="${QT_DIR}/lib"

case "$(uname -s)" in
    Darwin)
        export QT_PLUGIN_PATH
        export DYLD_LIBRARY_PATH="${QT_LIB_DIR}:${DYLD_LIBRARY_PATH:-}"
        ;;
    *)
        export QT_PLUGIN_PATH
        export LD_LIBRARY_PATH="${QT_LIB_DIR}:${LD_LIBRARY_PATH:-}"
        ;;
esac

"${BIN}" --config "${CONFIG}" --module "${MODULE}" "$@"
