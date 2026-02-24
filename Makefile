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

run: gui

run-ui: gui

gui: run-qt

qt: run-qt

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
	@powershell -NoProfile -Command "Write-Host 'Targets:'; Write-Host '  make run            Build + deploy Qt + launch GUI (default Qt module)'; Write-Host '  make gui            Same as run'; Write-Host '  make qt             Same as run'; Write-Host '  make all            Configure + build'; Write-Host '  make deploy-qt      Deploy Qt runtime/plugins for module host'; Write-Host '  make run-backend    Run backend mode via main binary'; Write-Host '  make run-headless   Run headless mode via main binary'; Write-Host '  make clean          Remove build directory'; Write-Host '  make re             Clean + full rebuild'; Write-Host ''; Write-Host 'Useful vars:'; Write-Host '  CONFIG=<file>       Config file passed to module host (default: simulation.ini)'; Write-Host '  GUI_MODULE=<name>   Frontend module name (default: qt)'; Write-Host '  BUILD_DIR=<dir>     Build directory (default: build)'; Write-Host '  QT_DIR=<path>       Qt root path used by deploy tools'; Write-Host '  ARGS=''...''          Extra args forwarded to runtime'"
else
help:
	@printf "Targets:\n"
	@printf "  make run            Build + deploy Qt + launch GUI (default Qt module)\n"
	@printf "  make gui            Same as run\n"
	@printf "  make qt             Same as run\n"
	@printf "  make all            Configure + build\n"
	@printf "  make deploy-qt      Deploy Qt runtime/plugins for module host\n"
	@printf "  make run-backend    Run backend mode via main binary\n"
	@printf "  make run-headless   Run headless mode via main binary\n"
	@printf "  make clean          Remove build directory\n"
	@printf "  make re             Clean + full rebuild\n"
	@printf "\nUseful vars:\n"
	@printf "  CONFIG=<file>       Config file passed to module host (default: simulation.ini)\n"
	@printf "  GUI_MODULE=<name>   Frontend module name (default: qt)\n"
	@printf "  BUILD_DIR=<dir>     Build directory (default: build)\n"
	@printf "  QT_DIR=<path>       Qt root path used by deploy tools\n"
	@printf "  ARGS='...'          Extra args forwarded to runtime\n"
endif

helper: help

.PHONY: all build-dev build-run build-ci run run-ui gui qt run-backend run-headless run-backend-direct run-headless-direct run-module-host deploy-qt run-qt deps-graphics deps-graphics-vcpkg clean fclean re help helper
