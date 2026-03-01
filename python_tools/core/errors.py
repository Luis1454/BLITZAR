#!/usr/bin/env python3
from __future__ import annotations


class CheckExecutionError(RuntimeError):
    pass


class ConfigurationError(CheckExecutionError):
    pass

