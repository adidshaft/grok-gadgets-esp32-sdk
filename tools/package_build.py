"""Package unpublished build output with hashes and toolchain provenance."""

import hashlib
import json
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path

root = Path(__file__).resolve().parents[1]
pio = root / ".venv/bin/pio"
if subprocess.check_output(["git", "status", "--porcelain"], cwd=root, text=True).strip():
    raise SystemExit("Commit source changes before packaging firmware provenance")
source_commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
# Always build the identified clean snapshot; never re-label stale output from a
# previous commit with the current HEAD.
subprocess.run([pio, "run", "-e", "atoms3-lite-usb"], cwd=root, check=True)
out = root / "artifacts/c124-usb"
out.mkdir(parents=True, exist_ok=True)
files = {}
for name in ("firmware.bin", "firmware.elf", "bootloader.bin", "partitions.bin"):
    source = root / ".pio/build/atoms3-lite-usb" / name
    shutil.copy2(source, out / name)
    files[name] = {
        "bytes": source.stat().st_size,
        "sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
    }
manifest = {
    "status": "build verified; hardware and Grok pending; unpublished",
    "source_commit": source_commit,
    "source_tree_clean": True,
    "built_at_utc": datetime.now(timezone.utc).isoformat(),
    "build_command": ".venv/bin/pio run -e atoms3-lite-usb",
    "toolchain": {
        "platformio_core": subprocess.check_output([pio, "--version"], text=True).strip(),
        "resolved_packages": subprocess.check_output(
            [pio, "pkg", "list", "-e", "atoms3-lite-usb"], cwd=root, text=True
        ).strip(),
    },
    "files": files,
}
(out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
print(json.dumps(manifest, indent=2))
