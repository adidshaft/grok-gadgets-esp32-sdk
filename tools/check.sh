#!/bin/sh
set -eu
.venv/bin/ruff check tools examples
.venv/bin/ruff format --check tools examples
.venv/bin/clang-format --dry-run --Werror lib/GrokGadgets/src/*.h src/*.cpp examples/*/*.cpp examples/*/*.h tests/*.cpp tests/stubs/*.h
# Host update tests link OpenSSL; Homebrew keeps it outside the default search path.
for prefix in /opt/homebrew/opt/openssl@3 /usr/local/opt/openssl@3; do
  if [ -z "${OPENSSL_ROOT_DIR:-}" ] && [ -d "$prefix" ]; then OPENSSL_ROOT_DIR=$prefix; fi
done
cmake -S . -B build ${OPENSSL_ROOT_DIR:+-DOPENSSL_ROOT_DIR="$OPENSSL_ROOT_DIR"}
cmake --build build
ctest --test-dir build --output-on-failure

.venv/bin/python -m unittest discover -s examples/oled-kit -p 'test_*.py' -v
# Builder examples (hub ADR 013 §6): rehearse each sketch's host build.
for example in climate-sensor switch servo; do
  .venv/bin/python -m unittest discover -s examples/$example -p 'test_*.py' -v
done
