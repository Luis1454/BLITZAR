check:
ifeq ($(CHECK),ini)
	python tests/checks/check.py ini --config $(CONFIG)
else ifeq ($(CHECK),mirror)
	python tests/checks/check.py mirror --root .
else ifeq ($(CHECK),no_legacy)
ifeq ($(CHECK_BUILD_TARGETS),1)
	python tests/checks/check.py no_legacy --root . --build-dir $(BUILD_DIR) --check-build-targets
else
	python tests/checks/check.py no_legacy --root .
endif
else ifeq ($(CHECK),launcher)
	python tests/checks/check.py launcher --build-dir $(BUILD_DIR)
else ifeq ($(CHECK),quality)
	python tests/checks/check.py quality --root .
else ifeq ($(CHECK),repo)
	python tests/checks/check.py repo --root .
else ifeq ($(CHECK),all)
	python tests/checks/check.py all --root . --config $(CONFIG)
else
	@echo "Unknown CHECK=$(CHECK). Supported: ini mirror no_legacy launcher quality repo all"
	@exit 2
endif

check-all:
	$(MAKE) check CHECK=all CONFIG=$(CONFIG)

.PHONY: check check-all
