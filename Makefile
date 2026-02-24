EXECUTABLE := myApp
HEADLESS_EXECUTABLE := myAppHeadless
BACKEND_EXECUTABLE := myAppBackend
MODULE_HOST_EXECUTABLE := myAppModuleHost
BUILD_DIR := build
GENERATOR ?= Ninja
CUDA_ARCH ?= native
BUILD_TYPE ?= Release
QT_DIR ?= C:/Qt/6.8.2/msvc2022_64
WINDEPLOYQT ?= $(QT_DIR)/bin/windeployqt.exe
MACDEPLOYQT ?= $(QT_DIR)/bin/macdeployqt
LINUXDEPLOYQT ?= linuxdeployqt
QT_PLUGIN_PATH ?= $(QT_DIR)/plugins
QT_LIB_DIR ?= $(QT_DIR)/lib
CONFIG ?= simulation.ini
GUI_MODULE ?= qt
RUN_DOCTOR ?= 1
ifeq ($(OS),Windows_NT)
UNAME_S := Windows
else
UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)
endif
ARGS ?=

ifeq ($(OS),Windows_NT)
RUN_BIN := $(BUILD_DIR)/$(EXECUTABLE).exe
RUN_HEADLESS_BIN := $(BUILD_DIR)/$(HEADLESS_EXECUTABLE).exe
RUN_BACKEND_BIN := $(BUILD_DIR)/$(BACKEND_EXECUTABLE).exe
RUN_MODULE_HOST_BIN := $(BUILD_DIR)/$(MODULE_HOST_EXECUTABLE).exe
QT_MODULE_LIB := $(BUILD_DIR)/gravityFrontendModuleQtInProc.dll
else
RUN_BIN := $(BUILD_DIR)/$(EXECUTABLE)
RUN_HEADLESS_BIN := $(BUILD_DIR)/$(HEADLESS_EXECUTABLE)
RUN_BACKEND_BIN := $(BUILD_DIR)/$(BACKEND_EXECUTABLE)
RUN_MODULE_HOST_BIN := $(BUILD_DIR)/$(MODULE_HOST_EXECUTABLE)
ifeq ($(UNAME_S),Darwin)
QT_MODULE_LIB := $(BUILD_DIR)/gravityFrontendModuleQtInProc.dylib
else
QT_MODULE_LIB := $(BUILD_DIR)/gravityFrontendModuleQtInProc.so
endif
endif

all:
	cmake -S . -B $(BUILD_DIR) -G "$(GENERATOR)" -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_CUDA_ARCHITECTURES=$(CUDA_ARCH)
	cmake --build $(BUILD_DIR)

build-dev:
	powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Profile dev -BuildDir build-dev -Generator "$(GENERATOR)" -CudaArch "$(CUDA_ARCH)"

build-run:
	powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Profile run -BuildDir build-run -Generator "$(GENERATOR)" -CudaArch "$(CUDA_ARCH)"

build-ci:
	powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Profile ci -BuildDir build-ci -Generator "$(GENERATOR)" -CudaArch "$(CUDA_ARCH)"

run: pre-run gui

run-ui: pre-run gui

gui: run-qt

qt: run-qt

run-mode-%: pre-run all
	$(RUN_BIN) --mode $* $(ARGS)

run-backend: run-mode-backend

run-headless: run-mode-headless

run-backend-direct: all
	$(RUN_BACKEND_BIN) $(ARGS)

run-headless-direct: all
	$(RUN_HEADLESS_BIN) $(ARGS)

run-module-host: all
	$(RUN_MODULE_HOST_BIN) $(ARGS)

doctor:
ifeq ($(OS),Windows_NT)
	powershell -ExecutionPolicy Bypass -File scripts/dev/doctor.ps1 -QtDir "$(QT_DIR)" -BuildDir "$(BUILD_DIR)"
else
	@echo "doctor target currently implemented for Windows."
endif

ifeq ($(RUN_DOCTOR),1)
pre-run: doctor
else
pre-run:
	@echo "pre-run doctor check skipped (RUN_DOCTOR=0)"
endif

deploy-qt:
ifeq ($(OS),Windows_NT)
	powershell -NoProfile -ExecutionPolicy Bypass -Command "if (!(Test-Path '$(WINDEPLOYQT)')) { throw 'windeployqt introuvable: $(WINDEPLOYQT)' }; if (!(Test-Path '$(QT_MODULE_LIB)')) { throw 'module Qt introuvable: $(QT_MODULE_LIB)' }; & '$(WINDEPLOYQT)' --dir '$(BUILD_DIR)' --release --compiler-runtime '$(QT_MODULE_LIB)'"
else
ifeq ($(UNAME_S),Darwin)
	@if [ ! -x "$(MACDEPLOYQT)" ]; then echo "macdeployqt introuvable: $(MACDEPLOYQT)"; exit 1; fi
	@if [ ! -f "$(QT_MODULE_LIB)" ]; then echo "module Qt introuvable: $(QT_MODULE_LIB)"; exit 1; fi
	"$(MACDEPLOYQT)" "$(QT_MODULE_LIB)" -verbose=0
else
	@if [ ! -f "$(QT_MODULE_LIB)" ]; then echo "module Qt introuvable: $(QT_MODULE_LIB)"; exit 1; fi
	@if command -v "$(LINUXDEPLOYQT)" >/dev/null 2>&1; then \
		"$(LINUXDEPLOYQT)" "$(QT_MODULE_LIB)" -bundle-non-qt-libs; \
	else \
		echo "linuxdeployqt introuvable: deploy skip (utilise QT_PLUGIN_PATH/LD_LIBRARY_PATH pour run)"; \
	fi
endif
endif

run-qt: all deploy-qt
ifeq ($(OS),Windows_NT)
	$(RUN_MODULE_HOST_BIN) --config $(CONFIG) --module $(GUI_MODULE) $(ARGS)
else
ifeq ($(UNAME_S),Darwin)
	QT_PLUGIN_PATH="$(QT_PLUGIN_PATH)" DYLD_LIBRARY_PATH="$(QT_LIB_DIR):$$DYLD_LIBRARY_PATH" $(RUN_MODULE_HOST_BIN) --config $(CONFIG) --module $(GUI_MODULE) $(ARGS)
else
	QT_PLUGIN_PATH="$(QT_PLUGIN_PATH)" LD_LIBRARY_PATH="$(QT_LIB_DIR):$$LD_LIBRARY_PATH" $(RUN_MODULE_HOST_BIN) --config $(CONFIG) --module $(GUI_MODULE) $(ARGS)
endif
endif

deps-graphics:
	powershell -ExecutionPolicy Bypass -File scripts/install_graphics_deps.ps1

deps-graphics-vcpkg:
	powershell -ExecutionPolicy Bypass -File scripts/install_graphics_deps.ps1 -BuildQtWithVcpkg

clean:
	cmake -E rm -rf $(BUILD_DIR)

fclean: clean

re: clean all

ifeq ($(OS),Windows_NT)
help:
	@powershell -NoProfile -Command "Write-Host 'Targets:'; Write-Host '  make run            Doctor check + build + deploy Qt + launch GUI'; Write-Host '  make gui            Build + deploy Qt + launch GUI (no pre-check)'; Write-Host '  make qt             Same as gui'; Write-Host '  make all            Configure + build'; Write-Host '  make doctor         Validate local toolchain (CMake/CUDA/MSVC/Qt)'; Write-Host '  make deploy-qt      Deploy Qt runtime/plugins for module host'; Write-Host '  make run-backend    Doctor check + run backend mode via main binary'; Write-Host '  make run-headless   Doctor check + run headless mode via main binary'; Write-Host '  make clean          Remove build directory'; Write-Host '  make re             Clean + full rebuild'; Write-Host ''; Write-Host 'Useful vars:'; Write-Host '  CONFIG=<file>       Config file passed to module host (default: simulation.ini)'; Write-Host '  GUI_MODULE=<name>   Frontend module name (default: qt)'; Write-Host '  RUN_DOCTOR=0|1      Enable/disable doctor before run targets (default: 1)'; Write-Host '  BUILD_DIR=<dir>     Build directory (default: build)'; Write-Host '  QT_DIR=<path>       Qt root path used by deploy tools'; Write-Host '  ARGS=''...''          Extra args forwarded to runtime'"
else
help:
	@printf "Targets:\n"
	@printf "  make run            Doctor check + build + deploy Qt + launch GUI\n"
	@printf "  make gui            Build + deploy Qt + launch GUI (no pre-check)\n"
	@printf "  make qt             Same as gui\n"
	@printf "  make all            Configure + build\n"
	@printf "  make doctor         Validate local toolchain (CMake/CUDA/MSVC/Qt)\n"
	@printf "  make deploy-qt      Deploy Qt runtime/plugins for module host\n"
	@printf "  make run-backend    Doctor check + run backend mode via main binary\n"
	@printf "  make run-headless   Doctor check + run headless mode via main binary\n"
	@printf "  make clean          Remove build directory\n"
	@printf "  make re             Clean + full rebuild\n"
	@printf "\nUseful vars:\n"
	@printf "  CONFIG=<file>       Config file passed to module host (default: simulation.ini)\n"
	@printf "  GUI_MODULE=<name>   Frontend module name (default: qt)\n"
	@printf "  RUN_DOCTOR=0|1      Enable/disable doctor before run targets (default: 1)\n"
	@printf "  BUILD_DIR=<dir>     Build directory (default: build)\n"
	@printf "  QT_DIR=<path>       Qt root path used by deploy tools\n"
	@printf "  ARGS='...'          Extra args forwarded to runtime\n"
endif

helper: help

.PHONY: all build-dev build-run build-ci run run-ui pre-run gui qt run-backend run-headless run-backend-direct run-headless-direct run-module-host doctor deploy-qt run-qt deps-graphics deps-graphics-vcpkg clean fclean re help helper
