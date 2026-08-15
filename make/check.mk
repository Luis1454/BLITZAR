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
	cd tests && cmake --preset $(QUALITY_PRESET)

quality-build:
	cd tests && cmake --build --preset $(QUALITY_PRESET) --parallel

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
	cd tests && cmake -E env ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
		ctest --preset $(QUALITY_PRESET) --output-on-failure --timeout $(QUALITY_TIMEOUT) --no-tests=error -R "$(QUALITY_TEST_REGEX)"

quality-strict: quality-local quality-python quality-rust quality-configure quality-build quality-analyze quality-test

.PHONY: check check-all quality-local quality-python quality-rust quality-configure quality-build quality-analyze quality-analyze-fast quality-test quality-strict
