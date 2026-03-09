check:
	python tests/checks/check.py $(CHECK) --root . --config $(CONFIG) $(if $(strip $(BUILD_DIR)),--build-dir $(BUILD_DIR),) $(if $(filter 1,$(CHECK_BUILD_TARGETS)),--check-build-targets,)

check-all:
	$(MAKE) check CHECK=all CONFIG=$(CONFIG)

quality-local:
	$(MAKE) check CHECK=all CONFIG=$(CONFIG) BUILD_DIR=

quality-python:
	python -m ruff check tests/checks scripts/ci python_tools
	python -m mypy tests/checks scripts/ci python_tools

quality-configure:
	cmake -S tests -B $(QUALITY_BUILD_DIR) -G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=$(QUALITY_BUILD_TYPE) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DGRAVITY_PROFILE=$(QUALITY_PROFILE) \
		-DGRAVITY_INTEGRATION_STRICT_WARNINGS=ON \
		-DGRAVITY_INTEGRATION_ENABLE_SANITIZERS=ON

quality-build:
	cmake --build $(QUALITY_BUILD_DIR) --parallel

quality-analyze:
	python tests/checks/run.py clang_tidy --root . --build-dir $(QUALITY_BUILD_DIR)

quality-test:
	cmake -E env ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
		ctest --test-dir $(QUALITY_BUILD_DIR) --output-on-failure --timeout $(QUALITY_TIMEOUT) --no-tests=error -R "$(QUALITY_TEST_REGEX)"

quality-strict: quality-local quality-python quality-configure quality-build quality-analyze quality-test

.PHONY: check check-all quality-local quality-python quality-configure quality-build quality-analyze quality-test quality-strict
