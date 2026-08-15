# @file Makefile
# @author Luis1454
# @project BLITZAR
# @brief Source artifact for the BLITZAR simulation project.

EXECUTABLE := blitzar
HEADLESS_EXECUTABLE := blitzar-headless
SERVER_EXECUTABLE := blitzar-server
CLIENT_HOST_EXECUTABLE := blitzar-client

JOBS ?=

INT_TEST_REGEX ?=
INT_TEST_REGEX_NO_SERVER ?= ^(TST_UNT_CONF_|TST_INT_PROT_003_ServerClientConnectTimeoutIsBounded$$|TST_QLT_REPO_.*)
INT_TIMEOUT ?= 180
SERVER_EXE ?=
INT_PRESET ?= integration-safe

QT_DIR ?= C:/Qt/6.8.2/msvc2022_64
WINDEPLOYQT ?= $(QT_DIR)/bin/windeployqt.exe
MACDEPLOYQT ?= $(QT_DIR)/bin/macdeployqt
LINUXDEPLOYQT ?= linuxdeployqt
QT_PLUGIN_PATH ?= $(QT_DIR)/plugins
QT_LIB_DIR ?= $(QT_DIR)/lib

VCPKG_TRIPLET ?= x64-windows
RUN_DOCTOR ?= 1
CONFIG ?= simulation.ini
CHECK ?= ini
CHECK_BUILD_TARGETS ?= 0
GUI_MODULE ?= qt
ARGS ?=
override QUALITY_BUILD_DIR := build-quality
QUALITY_TIMEOUT ?= 180
QUALITY_TIDY_JOBS ?= 0
QUALITY_TIDY_DIFF_BASE ?=
QUALITY_TIDY_DIFF_TARGET ?=
QUALITY_TIDY_FILE_TIMEOUT_SEC ?= 0
QUALITY_TIDY_TIMEOUT_FALLBACK_CHECKS ?=
QUALITY_PRESET ?= integration-quality
DOCKER ?= docker
DOCKER_IMAGE ?= blitzar-cpu:local

ifeq ($(OS),Windows_NT)
HOST_OS := Windows
else
HOST_OS := $(shell uname -s 2>/dev/null || echo Unknown)
endif

ifeq ($(HOST_OS),Windows)
DEFAULT_PRESET := windows-desktop
DEFAULT_BUILD_DIR := build-desktop
else ifeq ($(HOST_OS),Darwin)
DEFAULT_PRESET := macos-dev
DEFAULT_BUILD_DIR := build-macos
else
DEFAULT_PRESET := linux-dev
DEFAULT_BUILD_DIR := build
endif
PRESET ?= $(DEFAULT_PRESET)
BUILD_DIR ?= $(DEFAULT_BUILD_DIR)
# Fast, integration-safe subset: keep Python checks authoritative in the workflow (ruff/mypy/pytest),
# and keep CTest focused on deterministic C++/integration tests.
QUALITY_TEST_REGEX ?= TST_UNT_CONF_|TST_QLT_REPO_00(1|2|3|4|6|7)_

UNAME_S := $(HOST_OS)

ifeq ($(HOST_OS),Windows)
RUN_BIN := $(BUILD_DIR)/$(EXECUTABLE).exe
RUN_HEADLESS_BIN := $(BUILD_DIR)/$(HEADLESS_EXECUTABLE).exe
RUN_SERVER_BIN := $(BUILD_DIR)/$(SERVER_EXECUTABLE).exe
RUN_CLIENT_HOST_BIN := $(BUILD_DIR)/$(CLIENT_HOST_EXECUTABLE).exe
QT_MODULE_LIB := $(BUILD_DIR)/blitzarClientModuleQtInProc.dll
else
RUN_BIN := $(BUILD_DIR)/$(EXECUTABLE)
RUN_HEADLESS_BIN := $(BUILD_DIR)/$(HEADLESS_EXECUTABLE)
RUN_SERVER_BIN := $(BUILD_DIR)/$(SERVER_EXECUTABLE)
RUN_CLIENT_HOST_BIN := $(BUILD_DIR)/$(CLIENT_HOST_EXECUTABLE)
ifeq ($(UNAME_S),Darwin)
QT_MODULE_LIB := $(BUILD_DIR)/libblitzarClientModuleQtInProc.dylib
else
QT_MODULE_LIB := $(BUILD_DIR)/libblitzarClientModuleQtInProc.so
endif
endif

ifeq ($(strip $(SERVER_EXE)),)
ifneq ($(wildcard $(RUN_SERVER_BIN)),)
SERVER_EXE := $(abspath $(RUN_SERVER_BIN))
endif
endif

BUILD_CMD = cmake --build --preset $(PRESET)
ifneq ($(strip $(JOBS)),)
BUILD_CMD += --parallel $(JOBS)
else
BUILD_CMD += --parallel
endif

ifneq ($(strip $(SERVER_EXE)),)
INT_CTEST_ENV = cmake -E env "BLITZAR_SERVER_EXE=$(SERVER_EXE)"
endif

include make/check.mk
include make/runtime.mk

all: configure build

configure:
	cmake --preset $(PRESET)

build:
	$(BUILD_CMD)

test: all
	$(MAKE) test-int INT_PRESET=integration-safe

build-dev:
	$(MAKE) all PRESET=linux-dev BUILD_DIR=build

build-prod:
	$(MAKE) all PRESET=linux-prod BUILD_DIR=build-prod

build-run:
	$(MAKE) all PRESET=linux-prod BUILD_DIR=build-prod

build-ci:
	$(MAKE) all PRESET=release-prod BUILD_DIR=build

test-int: int-configure int-build int-run

int-configure:
	cmake --preset $(INT_PRESET) -S tests

int-build:
	cd tests && cmake --build --preset $(INT_PRESET) --parallel

int-run:
ifeq ($(strip $(SERVER_EXE)),)
	@echo "SERVER_EXE is empty and $(RUN_SERVER_BIN) is unavailable"
endif
ifeq ($(strip $(INT_TEST_REGEX)),)
ifeq ($(strip $(SERVER_EXE)),)
	@echo "Running safe integration subset only (set SERVER_EXE to run all integration_real tests)"
	cd tests && ctest --preset $(INT_PRESET) --output-on-failure --timeout $(INT_TIMEOUT) --no-tests=error -R "$(INT_TEST_REGEX_NO_SERVER)"
else
	cd tests && $(INT_CTEST_ENV) ctest --preset $(INT_PRESET) --output-on-failure --timeout $(INT_TIMEOUT) --no-tests=error
endif
else
	cd tests && $(INT_CTEST_ENV) ctest --preset $(INT_PRESET) --output-on-failure --timeout $(INT_TIMEOUT) --no-tests=error -R "$(INT_TEST_REGEX)"
endif

docker-build-cpu:
	$(DOCKER) build --file Dockerfile.cpu --tag $(DOCKER_IMAGE) .

docker-run-headless: docker-build-cpu
	$(DOCKER) run --rm $(DOCKER_IMAGE) --inspect --config /blitzar/simulation.ini

docker-shell: docker-build-cpu
	$(DOCKER) run --rm --interactive --tty --entrypoint /bin/bash $(DOCKER_IMAGE)

.PHONY: all configure build test build-dev build-prod build-run build-ci test-int int-configure int-build int-run docker-build-cpu docker-run-headless docker-shell
