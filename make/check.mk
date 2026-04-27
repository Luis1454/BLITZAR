# @file make/check.mk
# @author Luis1454
# @project BLITZAR
# @brief Makefile fragments for local quality and runtime workflows.

check:
	python tests/checks/check.py $(CHECK) --root . --config $(CONFIG) $(if $(strip $(BUILD_DIR)),--build-dir $(BUILD_DIR),) $(if $(filter 1,$(CHECK_BUILD_TARGETS)),--check-build-targets,)

check-all:
	$(MAKE) check CHECK=all CONFIG=$(CONFIG)

quality-local:
	$(MAKE) check CHECK=all CONFIG=$(CONFIG) BUILD_DIR=

quality-python:
	python -m ruff check .
	python -m mypy tests/checks scripts/ci python_tools
	python -m pytest -q tests/checks/suites

quality-rust:
	cargo fmt --all --check --manifest-path rust/Cargo.toml
	cargo test --manifest-path rust/Cargo.toml

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
	python tests/checks/run.py clang_tidy --root . --build-dir $(QUALITY_BUILD_DIR) \
		--jobs $(QUALITY_TIDY_JOBS) \
		$(if $(filter-out 0,$(QUALITY_TIDY_FILE_TIMEOUT_SEC)),--file-timeout-sec $(QUALITY_TIDY_FILE_TIMEOUT_SEC),) \
		$(if $(strip $(QUALITY_TIDY_TIMEOUT_FALLBACK_CHECKS)),--timeout-fallback-checks "$(QUALITY_TIDY_TIMEOUT_FALLBACK_CHECKS)",) \
		$(if $(strip $(QUALITY_TIDY_DIFF_BASE)),--diff-base $(QUALITY_TIDY_DIFF_BASE),) \
		$(if $(strip $(QUALITY_TIDY_DIFF_TARGET)),--diff-target $(QUALITY_TIDY_DIFF_TARGET),)

quality-analyze-fast:
	$(MAKE) quality-analyze \
		QUALITY_TIDY_FILE_TIMEOUT_SEC=120 \
		QUALITY_TIDY_TIMEOUT_FALLBACK_CHECKS=-*,bugprone-unused-return-value

quality-test:
	cmake -E env ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
		ctest --test-dir $(QUALITY_BUILD_DIR) --output-on-failure --timeout $(QUALITY_TIMEOUT) --no-tests=error -R "$(QUALITY_TEST_REGEX)"

quality-strict: quality-local quality-python quality-rust quality-configure quality-build quality-analyze quality-test

.PHONY: check check-all quality-local quality-python quality-rust quality-configure quality-build quality-analyze quality-analyze-fast quality-test quality-strict
