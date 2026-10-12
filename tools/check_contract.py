"""Validate frames produced by the SDK example sketches against the pinned gateway schema.

The transcript binaries compile the real sketches (src/main.cpp, examples/*) with the
simulated board in tests/stubs and print every frame they write. Run tools/check.sh first.
"""

import hashlib
import json
import subprocess
from pathlib import Path

from jsonschema import Draft202012Validator

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / "protocol/source.json").read_text())
for name, expected in manifest["files_sha256"].items():
    data = (root / "protocol/0.1.0" / name).read_bytes()
    assert hashlib.sha256(data).hexdigest() == expected, f"Pinned protocol changed: {name}"
validator = Draft202012Validator(
    json.loads((root / "protocol/0.1.0/device-request.schema.json").read_text())
)
for frame in json.loads((root / "protocol/0.1.0/fixtures/device-transcript.json").read_text()):
    validator.validate(frame)


def command(command_id, capability, arguments):
    return json.dumps({"command_id": command_id, "capability": capability, "arguments": arguments})


sketches = {
    "transcript_c124": [
        command("t-green", "rgb.set", {"r": 0, "g": 255, "b": 0, "on": True}),
        command("t-bad", "rgb.set", {"r": 256, "g": 0, "b": 0, "on": True}),
    ],
    "transcript_led_button": [
        command("t-on", "led.set", {"on": True}),
        command("t-bad", "led.set", {"on": 1}),
    ],
    "transcript_oled_kit": [
        command("t-text", "display.text", {"text": "Grok Bot ready"}),
        command("t-bad", "display.text", {"text": "tab\there"}),
        command("t-clear", "display.clear", {}),
    ],
}
# The Wi-Fi build sends the same frames over TLS, with the device token in its hello.
sketches["transcript_oled_kit_wifi"] = sketches["transcript_oled_kit"]
expected_acks = {
    "transcript_c124": {"t-green": "executed", "t-bad": "failed"},
    "transcript_led_button": {"t-on": "executed", "t-bad": "failed"},
    "transcript_oled_kit": {"t-text": "executed", "t-bad": "failed", "t-clear": "executed"},
}
expected_acks["transcript_oled_kit_wifi"] = expected_acks["transcript_oled_kit"]
for binary, commands in sketches.items():
    output = subprocess.run(
        [root / "build" / binary, *commands], check=True, capture_output=True, text=True, timeout=60
    ).stdout
    frames = []
    for line in output.splitlines():
        assert len(line.encode()) + 1 <= 2048, f"{binary}: frame exceeds 2048 bytes"
        frame = json.loads(line)
        validator.validate(frame)
        frames.append(frame)
    kinds = {frame["type"] for frame in frames}
    assert kinds == {"hello", "poll", "ack", "event", "state"}, (binary, kinds)
    assert frames[0]["type"] == "hello", binary
    wifi = binary.endswith("_wifi")
    assert (frames[0].get("token") == "transcript-device-token") == wifi, (binary, "hello token")
    hello = frames[0]["device"]
    names = hello["capabilities"]
    assert names[-2:] == ["state", "history_lost"] and "button" in names, (binary, names)
    events = {"button", "history_lost"}
    for name, schema in hello.get("capability_schemas", {}).items():
        assert name in names, (binary, name)
        if schema.get("x-grok-gadgets-kind") == "event":
            events.add(name)
    sent = [frame for frame in frames if frame["type"] == "event"]
    assert {frame["name"] for frame in sent} <= events, (binary, "undeclared event")
    assert len({frame["event_id"] for frame in sent}) == len(sent), (binary, "reused event_id")
    losses = [frame["data"] for frame in sent if frame["name"] == "history_lost"]
    assert losses == [{"dropped": 4}], (binary, losses)
    acks = {frame["command_id"]: frame["status"] for frame in frames if frame["type"] == "ack"}
    assert acks == expected_acks[binary], (binary, acks)
    if binary in ("transcript_oled_kit", "transcript_oled_kit_wifi"):
        shown = {f["command_id"]: f["state"]["display"] for f in frames if f["type"] == "ack"}
        assert shown["t-text"] == {
            "ready": True,
            "text": "Grok Bot ready",
            "sda": 8,
            "scl": 9,
            "address": "0x3c",
        }
        assert shown["t-bad"]["text"] == "Grok Bot ready", "rejected text changed the display"
        assert shown["t-clear"]["text"] == "", shown
    print(f"contract: {binary} produced {len(frames)} valid frames ({', '.join(sorted(kinds))})")
print("contract: pinned protocol hashes and canonical fixture valid")
