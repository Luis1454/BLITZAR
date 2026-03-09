#!/usr/bin/env python3
from __future__ import annotations

from tests.checks.check import build_context, build_parser


def test_clang_tidy_parser_defaults_include_ignored_return_value_check() -> None:
    parser = build_parser()
    args = parser.parse_args(["clang_tidy", "--build-dir", "build-quality"])

    assert args.checks == "-*,clang-analyzer-*,bugprone-unused-return-value"


def test_clang_tidy_context_uses_default_ignored_return_value_check() -> None:
    args = build_parser().parse_args(["clang_tidy", "--build-dir", "build-quality"])
    context = build_context(args)

    assert context.clang_tidy_checks == "-*,clang-analyzer-*,bugprone-unused-return-value"
