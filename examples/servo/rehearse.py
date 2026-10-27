#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Rehearse the servo on the host: two moves and a refused angle.

Each acknowledgement comes after the move ends, which is what Grok Bot's
gadgets_command_status reports. Host evidence only: no servo or Grok Bot connection.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from example_rehearsal import report, run  # noqa: E402

COMMANDS = [
    ("servo.move", {"angle": 0}),
    ("servo.move", {"angle": 181}),
    ("servo.move", {"angle": 120}),
]


def rehearse():
    return run("servo", COMMANDS)


if __name__ == "__main__":
    report(rehearse())
