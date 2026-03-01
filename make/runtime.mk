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
	cmake -DQT_DIR="$(QT_DIR)" -DBUILD_DIR="$(BUILD_DIR)" -P scripts/doctor.cmake

ifeq ($(RUN_DOCTOR),1)
pre-run: doctor
else
pre-run:
	@echo "pre-run doctor check skipped (RUN_DOCTOR=0)"
endif

deploy-qt:
ifeq ($(OS),Windows_NT)
	"$(WINDEPLOYQT)" --dir "$(BUILD_DIR)" --release --compiler-runtime "$(QT_MODULE_LIB)"
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
	bash scripts/run_qt.sh --bin "$(RUN_MODULE_HOST_BIN)" --config "$(CONFIG)" --module "$(GUI_MODULE)" --qt-dir "$(QT_DIR)" -- $(ARGS)
endif

deps-graphics:
	cmake -DTRIPLET="$(VCPKG_TRIPLET)" -P scripts/deps_graphics.cmake

deps-graphics-vcpkg:
	cmake -DTRIPLET="$(VCPKG_TRIPLET)" -DBUILD_QT_WITH_VCPKG=ON -P scripts/deps_graphics.cmake

clean:
	cmake -DTARGETS="$(BUILD_DIR)" -DRETRIES=4 -DSLEEP_SECONDS=1 -P scripts/rm_paths.cmake

clean-all:
	cmake -DTARGETS="build;build-dev;build-run;build-ci;build-integration;build-integration-cov;build-refactor;build-verify;cmake-build-debug;cmake-build-release;exports;.qt;CMakeCache.txt" -DRETRIES=4 -DSLEEP_SECONDS=1 -P scripts/rm_paths.cmake

fclean: clean

re: clean all

help:
	@cmake -E cat scripts/help.txt

helper: help

.PHONY: run run-ui pre-run gui qt run-backend run-headless run-backend-direct run-headless-direct run-module-host doctor deploy-qt run-qt deps-graphics deps-graphics-vcpkg clean clean-all fclean re help helper
