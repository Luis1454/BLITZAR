#!/usr/bin/env python3
from __future__ import annotations

from typing import Any

JsonValue = None | bool | int | float | str | list["JsonValue"] | dict[str, "JsonValue"]
OptionsMap = dict[str, Any]

