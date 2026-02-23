EXECUTABLE := myApp
HEADLESS_EXECUTABLE := myAppHeadless
BACKEND_EXECUTABLE := myAppBackend
MODULE_HOST_EXECUTABLE := myAppModuleHost
BUILD_DIR := build
GENERATOR ?= Ninja
CUDA_ARCH ?= native
BUILD_TYPE ?= Release
ARGS ?=

ifeq ($(OS),Windows_NT)
RUN_BIN := $(BUILD_DIR)/$(EXECUTABLE).exe
RUN_HEADLESS_BIN := $(BUILD_DIR)/$(HEADLESS_EXECUTABLE).exe
RUN_BACKEND_BIN := $(BUILD_DIR)/$(BACKEND_EXECUTABLE).exe
RUN_MODULE_HOST_BIN := $(BUILD_DIR)/$(MODULE_HOST_EXECUTABLE).exe
else
RUN_BIN := $(BUILD_DIR)/$(EXECUTABLE)
RUN_HEADLESS_BIN := $(BUILD_DIR)/$(HEADLESS_EXECUTABLE)
RUN_BACKEND_BIN := $(BUILD_DIR)/$(BACKEND_EXECUTABLE)
RUN_MODULE_HOST_BIN := $(BUILD_DIR)/$(MODULE_HOST_EXECUTABLE)
endif

all:
	cmake -S . -B $(BUILD_DIR) -G "$(GENERATOR)" -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_CUDA_ARCHITECTURES=$(CUDA_ARCH)
	cmake --build $(BUILD_DIR)

run: all
	$(RUN_BIN) --mode ui $(ARGS)

run-ui: all
	$(RUN_BIN) --mode ui $(ARGS)

run-backend: all
	$(RUN_BIN) --mode backend $(ARGS)

run-headless: all
	$(RUN_BIN) --mode headless $(ARGS)

run-backend-direct: all
	$(RUN_BACKEND_BIN) $(ARGS)

run-headless-direct: all
	$(RUN_HEADLESS_BIN) $(ARGS)

run-module-host: all
	$(RUN_MODULE_HOST_BIN) $(ARGS)

deps-graphics:
	powershell -ExecutionPolicy Bypass -File scripts/install_graphics_deps.ps1

deps-graphics-vcpkg:
	powershell -ExecutionPolicy Bypass -File scripts/install_graphics_deps.ps1 -BuildQtWithVcpkg

clean:
	cmake -E rm -rf $(BUILD_DIR)

fclean: clean

re: clean all

.PHONY: all run run-ui run-backend run-headless run-backend-direct run-headless-direct run-module-host deps-graphics deps-graphics-vcpkg clean fclean re
