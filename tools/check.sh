#!/bin/sh
set -eu
.venv/bin/ruff check tools examples/oled-kit
.venv/bin/ruff format --check tools examples/oled-kit
.venv/bin/clang-format --dry-run --Werror lib/GrokGadgets/src/*.h src/*.cpp examples/*/*.cpp examples/*/*.h tests/*.cpp tests/stubs/*.h
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure

.venv/bin/python -m unittest discover -s examples/oled-kit -p 'test_*.py' -v
