# SPDX-License-Identifier: Apache-2.0
"""Run a builder example's host build against the scripted relay (hub ADR 013 §6).

The transcript build (`build/transcript_<example>_wifi`, made by `sh tools/check.sh`) runs the
real sketch with a simulated part: each argument is one command, handed over on one poll.
This is host evidence only. It proves no wiring, no part and no Grok Bot connection.
"""

import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def run(example, commands):
    """Return the frames the sketch sent, as dictionaries, for these commands."""
    binary = ROOT / "build" / f"transcript_{example.replace('-', '_')}_wifi"
    if not binary.exists():
        raise SystemExit(f"{binary.relative_to(ROOT)} is missing; run `sh tools/check.sh` first")
    arguments = [
        json.dumps({"command_id": f"r{index}", "capability": name, "arguments": args})
        for index, (name, args) in enumerate(commands, 1)
    ]
    output = subprocess.run([binary, *arguments], check=True, capture_output=True, text=True)
    return [json.loads(line) for line in output.stdout.splitlines() if line.startswith("{")]


def report(frames):
    """Print the acknowledgements and events that matter, one line each."""
    for frame in frames:
        if frame["type"] == "ack":
            error = frame.get("error", {}).get("code", "")
            print(
                json.dumps(
                    {
                        "ack": frame["command_id"],
                        "status": frame["status"],
                        "error": error,
                        "state": frame["state"],
                    }
                )
            )
        # The transcript presses the button and floods the event queue on purpose
        # (`history_lost`); those frames test the SDK, not the example.
        elif frame["type"] == "event" and frame["name"] not in ("button", "history_lost"):
            print(json.dumps({"event": frame["name"], "data": frame["data"]}))
