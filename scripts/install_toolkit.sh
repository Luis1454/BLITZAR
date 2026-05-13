#!/usr/bin/env bash
set -euo pipefail

PKGS_COMMON=(cmake ninja-build clang-format clang)

run_apt() {
    echo "Detected apt-get. Installing packages: ${PKGS_COMMON[*]} build-essential"
    sudo apt-get update
    sudo apt-get install -y build-essential "${PKGS_COMMON[@]}"
}

run_dnf() {
    echo "Detected dnf. Installing packages: ${PKGS_COMMON[*]} @development-tools"
    sudo dnf install -y @development-tools ${PKGS_COMMON[*]}
}

run_pacman() {
    echo "Detected pacman. Installing packages: base-devel ${PKGS_COMMON[*]}"
    sudo pacman -Syu --noconfirm --needed base-devel ${PKGS_COMMON[*]}
}

run_zypper() {
    echo "Detected zypper. Installing packages: gcc gcc-c++ ${PKGS_COMMON[*]}"
    sudo zypper refresh
    sudo zypper install -y gcc gcc-c++ ${PKGS_COMMON[*]}
}

main() {
    if command -v apt-get >/dev/null 2>&1; then
        run_apt
    elif command -v dnf >/dev/null 2>&1; then
        run_dnf
    elif command -v pacman >/dev/null 2>&1; then
        run_pacman
    elif command -v zypper >/dev/null 2>&1; then
        run_zypper
    else
        echo "Unsupported distro: please install build tools manually: cmake, ninja, clang-format, clang, gcc, g++"
        exit 3
    fi

    echo "Toolkit core installed."
    echo "Note: CUDA toolkit is not installed by this script. To add CUDA support, follow NVIDIA's official installation guide for your distribution."
}

main "$@"
