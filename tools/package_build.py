"""Package unpublished build output with hashes and toolchain provenance."""

import hashlib
import json
import shutil
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]
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
    "source_commit": subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=root, text=True
    ).strip(),
    "source_tree_clean": not subprocess.check_output(
        ["git", "status", "--porcelain"], cwd=root, text=True
    ).strip(),
    "files": files,
}
(out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
print(json.dumps(manifest, indent=2))
