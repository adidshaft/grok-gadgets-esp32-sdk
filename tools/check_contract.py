"""Validate SDK-generated frames against pinned canonical gateway schema."""

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
for line in subprocess.check_output([root / "build/transcript"], text=True).splitlines():
    frame = json.loads(line)
    validator.validate(frame)
    assert len(line.encode()) + 1 <= 2048
print("contract: canonical fixture and SDK-generated hello/ack/event/poll/state frames valid")
