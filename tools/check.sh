#!/bin/sh
set -eu
.venv/bin/ruff check tools
.venv/bin/ruff format --check tools
.venv/bin/clang-format --dry-run --Werror lib/GrokGadgets/src/*.h src/*.cpp tests/*.cpp tests/stubs/*.h
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
