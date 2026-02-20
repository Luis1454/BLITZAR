EXECUTABLE := myApp
FRONTEND_EXECUTABLE := myAppFrontend
QT_FRONTEND_EXECUTABLE := myAppQt
BUILD_DIR := build
GENERATOR ?= Ninja
CUDA_ARCH ?= native
BUILD_TYPE ?= Release

ifeq ($(OS),Windows_NT)
RUN_BIN := $(BUILD_DIR)/$(EXECUTABLE).exe
RUN_FRONTEND_BIN := $(BUILD_DIR)/$(FRONTEND_EXECUTABLE).exe
RUN_QT_FRONTEND_BIN := $(BUILD_DIR)/$(QT_FRONTEND_EXECUTABLE).exe
else
RUN_BIN := $(BUILD_DIR)/$(EXECUTABLE)
RUN_FRONTEND_BIN := $(BUILD_DIR)/$(FRONTEND_EXECUTABLE)
RUN_QT_FRONTEND_BIN := $(BUILD_DIR)/$(QT_FRONTEND_EXECUTABLE)
endif

all:
	cmake -S . -B $(BUILD_DIR) -G "$(GENERATOR)" -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_CUDA_ARCHITECTURES=$(CUDA_ARCH)
	cmake --build $(BUILD_DIR)

run: all
	$(RUN_BIN)

run-frontend: all
	$(RUN_FRONTEND_BIN)

run-qt: all
ifeq ($(OS),Windows_NT)
	powershell -NoProfile -ExecutionPolicy Bypass -Command "$$env:QT_PLUGIN_PATH='$(abspath $(BUILD_DIR))'; $$env:QT_QPA_PLATFORM_PLUGIN_PATH='$(abspath $(BUILD_DIR))/platforms'; & '$(RUN_QT_FRONTEND_BIN)'"
else
	$(RUN_QT_FRONTEND_BIN)
endif

deps-graphics:
	powershell -ExecutionPolicy Bypass -File scripts/install_graphics_deps.ps1

deps-graphics-vcpkg:
	powershell -ExecutionPolicy Bypass -File scripts/install_graphics_deps.ps1 -BuildQtWithVcpkg

clean:
	cmake -E rm -rf $(BUILD_DIR)

fclean: clean

re: clean all

.PHONY: all run run-frontend run-qt deps-graphics deps-graphics-vcpkg clean fclean re
