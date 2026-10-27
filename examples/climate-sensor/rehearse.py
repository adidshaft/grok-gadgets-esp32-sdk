#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Rehearse the climate sensor on the host: what Grok Bot's calls do before any part is wired.

Sets the threshold to 24 degrees, tries an out-of-range value, then lets the simulated module
swing from 22 to 28 degrees so that threshold.crossed fires. Host evidence only.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from example_rehearsal import report, run  # noqa: E402

COMMANDS = [("threshold.set", {"above_c": 24}), ("threshold.set", {"above_c": 100})]


def rehearse():
    return run("climate-sensor", COMMANDS)


if __name__ == "__main__":
    report(rehearse())
