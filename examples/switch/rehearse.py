#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Rehearse the switch on the host: on, on again, a refused value, then off.

The 30-second safe-off after a lost relay link is covered by tests/switch_tests.cpp.
Host evidence only: no relay module, load or Grok Bot connection is involved.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from example_rehearsal import report, run  # noqa: E402

COMMANDS = [
    ("switch.set", {"on": True}),
    ("switch.set", {"on": True}),
    ("switch.set", {"on": "yes"}),
    ("switch.set", {"on": False}),
]


def rehearse():
    return run("switch", COMMANDS)


if __name__ == "__main__":
    report(rehearse())
