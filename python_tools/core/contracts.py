#!/usr/bin/env python3
from __future__ import annotations

from typing import Protocol

from .models import CheckContext, CheckResult


class CheckContract(Protocol):
    def run(self, context: CheckContext) -> CheckResult:
        ...

