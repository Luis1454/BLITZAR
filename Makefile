EXECUTABLE := blitzar
HEADLESS_EXECUTABLE := blitzar-headless
SERVER_EXECUTABLE := blitzar-server
CLIENT_HOST_EXECUTABLE := blitzar-client

BUILD_DIR ?= build
BUILD_TYPE ?= Release
BUILD_TESTS ?= ON
PROFILE ?= dev
GENERATOR ?= Ninja
CUDA_ARCH ?= native
JOBS ?=
PROFILE_LOGS ?= OFF

INT_BUILD_DIR ?= build-integration
INT_BUILD_TYPE ?= Release
INT_TEST_REGEX ?=
INT_TEST_REGEX_NO_SERVER ?= ^(TST_UNT_CONF_|TST_INT_PROT_003_ServerClientConnectTimeoutIsBounded$$|TST_QLT_REPO_.*)
INT_TIMEOUT ?= 180
SERVER_EXE ?=

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
QUALITY_BUILD_DIR ?= build-quality
QUALITY_BUILD_TYPE ?= RelWithDebInfo
QUALITY_PROFILE ?= prod
QUALITY_TIMEOUT ?= 180
QUALITY_TIDY_JOBS ?= 0
QUALITY_TIDY_DIFF_BASE ?=
QUALITY_TIDY_DIFF_TARGET ?=
QUALITY_TIDY_FILE_TIMEOUT_SEC ?= 0
QUALITY_TIDY_TIMEOUT_FALLBACK_CHECKS ?=
# Fast, integration-safe subset: keep Python checks authoritative in the workflow (ruff/mypy/pytest),
# and keep CTest focused on deterministic C++/integration tests.
QUALITY_TEST_REGEX ?= TST_UNT_CONF_|TST_QLT_REPO_00(1|2|3|4|6|7)_

ifeq ($(OS),Windows_NT)
UNAME_S := Windows
else
UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)
endif

ifeq ($(OS),Windows_NT)
RUN_BIN := $(BUILD_DIR)/$(EXECUTABLE).exe
RUN_HEADLESS_BIN := $(BUILD_DIR)/$(HEADLESS_EXECUTABLE).exe
RUN_SERVER_BIN := $(BUILD_DIR)/$(SERVER_EXECUTABLE).exe
RUN_CLIENT_HOST_BIN := $(BUILD_DIR)/$(CLIENT_HOST_EXECUTABLE).exe
QT_MODULE_LIB := $(BUILD_DIR)/gravityClientModuleQtInProc.dll
else
RUN_BIN := $(BUILD_DIR)/$(EXECUTABLE)
RUN_HEADLESS_BIN := $(BUILD_DIR)/$(HEADLESS_EXECUTABLE)
RUN_SERVER_BIN := $(BUILD_DIR)/$(SERVER_EXECUTABLE)
RUN_CLIENT_HOST_BIN := $(BUILD_DIR)/$(CLIENT_HOST_EXECUTABLE)
ifeq ($(UNAME_S),Darwin)
QT_MODULE_LIB := $(BUILD_DIR)/gravityClientModuleQtInProc.dylib
else
QT_MODULE_LIB := $(BUILD_DIR)/gravityClientModuleQtInProc.so
endif
endif

ifeq ($(strip $(SERVER_EXE)),)
ifneq ($(wildcard $(RUN_SERVER_BIN)),)
SERVER_EXE := $(abspath $(RUN_SERVER_BIN))
endif
endif

CMAKE_FLAGS = \
	-G "$(GENERATOR)" \
	-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
	-DCMAKE_CUDA_ARCHITECTURES=$(CUDA_ARCH) \
	-DGRAVITY_PROFILE=$(PROFILE) \
	-DGRAVITY_BUILD_SERVER_DAEMON=ON \
	-DGRAVITY_BUILD_HEADLESS_BINARY=ON \
	-DGRAVITY_BUILD_CLIENT_HOST=ON \
	-DGRAVITY_BUILD_CLIENT_MODULES=ON \
	-DGRAVITY_BUILD_TESTS=$(BUILD_TESTS) \
	-DGRAVITY_PROFILE_LOGS=$(PROFILE_LOGS)

BUILD_CMD = cmake --build $(BUILD_DIR)
ifneq ($(strip $(JOBS)),)
BUILD_CMD += --parallel $(JOBS)
else
BUILD_CMD += --parallel
endif

INT_BUILD_CMD = cmake --build $(INT_BUILD_DIR)
ifneq ($(strip $(JOBS)),)
INT_BUILD_CMD += --parallel $(JOBS)
else
INT_BUILD_CMD += --parallel
endif

INT_CTEST_CMD = ctest --test-dir $(INT_BUILD_DIR) --output-on-failure --timeout $(INT_TIMEOUT)
ifneq ($(strip $(SERVER_EXE)),)
INT_CTEST_ENV = cmake -E env "GRAVITY_SERVER_EXE=$(SERVER_EXE)"
endif

include make/check.mk
include make/runtime.mk

all: configure build

configure:
	cmake -S . -B $(BUILD_DIR) $(CMAKE_FLAGS)

build:
	$(BUILD_CMD)

test: all
	ctest --test-dir $(BUILD_DIR) --output-on-failure

build-dev:
	$(MAKE) all BUILD_DIR=build-dev BUILD_TYPE=Debug BUILD_TESTS=ON PROFILE=dev

build-prod:
	$(MAKE) all BUILD_DIR=build-prod BUILD_TYPE=Release BUILD_TESTS=ON PROFILE=prod

build-run:
	$(MAKE) all BUILD_DIR=build-run BUILD_TYPE=Release BUILD_TESTS=OFF

build-ci:
	$(MAKE) all BUILD_DIR=build-ci BUILD_TYPE=Release BUILD_TESTS=ON PROFILE=prod

test-int: int-configure int-build int-run

int-configure:
	cmake -S tests -B $(INT_BUILD_DIR) -G "$(GENERATOR)" -DCMAKE_BUILD_TYPE=$(INT_BUILD_TYPE)

int-build:
	$(INT_BUILD_CMD)

int-run:
ifeq ($(strip $(SERVER_EXE)),)
	@echo "SERVER_EXE is empty and $(RUN_SERVER_BIN) is unavailable"
endif
ifeq ($(strip $(INT_TEST_REGEX)),)
ifeq ($(strip $(SERVER_EXE)),)
	@echo "Running safe integration subset only (set SERVER_EXE to run all integration_real tests)"
	$(INT_CTEST_CMD) -R "$(INT_TEST_REGEX_NO_SERVER)"
else
	$(INT_CTEST_ENV) $(INT_CTEST_CMD)
endif
else
	$(INT_CTEST_ENV) $(INT_CTEST_CMD) -R "$(INT_TEST_REGEX)"
endif
.PHONY: all configure build test build-dev build-prod build-run build-ci test-int int-configure int-build int-run
