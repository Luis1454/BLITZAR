#!/usr/bin/env python3
from __future__ import annotations

from argparse import Namespace

from checks.clang_tidy_check import ClangTidyCommand


def test_clang_tidy_parser_defaults_include_ignored_return_value_check() -> None:
    parser = ClangTidyCommand().build_parser()
    args = parser.parse_args(["--build-dir", "build-quality"])

    assert args.checks == "-*,clang-analyzer-*,bugprone-unused-return-value"


def test_clang_tidy_context_uses_default_ignored_return_value_check() -> None:
    command = ClangTidyCommand()
    args = Namespace(
        build_dir="build-quality",
        root=".",
        clang_tidy="clang-tidy",
        checks="-*,clang-analyzer-*,bugprone-unused-return-value",
        paths=[],
    )

    context = command.build_context(args)

    assert context.clang_tidy_checks == "-*,clang-analyzer-*,bugprone-unused-return-value"
