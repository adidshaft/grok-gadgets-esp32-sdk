"""Package unpublished build output with hashes and toolchain provenance."""

import hashlib
import json
import platform
import shutil
import struct
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
build = root / ".pio/build/atoms3-lite-usb"
boot_app0 = (
    Path.home() / ".platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"
)
if not boot_app0.is_file():
    raise SystemExit(f"Missing {boot_app0}")
# PlatformIO rewrites this board's qio mode to dio for Arduino uploads.
flash_args = """\
--chip esp32s3 --flash_mode dio --flash_freq 80m --flash_size 8MB
0x0 bootloader.bin
0x8000 partitions.bin
0xe000 boot_app0.bin
0x10000 firmware.bin
"""
(out / "flash_args").write_text(flash_args)
shutil.copy2(boot_app0, out / "boot_app0.bin")
files = {}
for name in (
    "firmware.bin",
    "firmware.elf",
    "bootloader.bin",
    "partitions.bin",
    "boot_app0.bin",
    "flash_args",
):
    source = out / name if name in ("boot_app0.bin", "flash_args") else build / name
    if name not in ("boot_app0.bin", "flash_args"):
        shutil.copy2(source, out / name)
    data = (out / name).read_bytes()
    files[name] = {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
toolchain_package = Path.home() / ".platformio/packages/toolchain-xtensa-esp32s3/package.json"
toolchain_system = "unknown"
if toolchain_package.is_file():
    toolchain_system = json.loads(toolchain_package.read_text()).get("system", "unknown")
manifest = {
    "status": "build verified; hardware and Grok pending; unpublished",
    "source_commit": source_commit,
    "source_tree_clean": True,
    "built_at_utc": datetime.now(timezone.utc).isoformat(),
    "build_command": ".venv/bin/pio run -e atoms3-lite-usb",
    "hash_depends_on": (
        "firmware.bin and firmware.elf follow the Xtensa toolchain package "
        "system (darwin_arm64, darwin_x86_64, or linux), not only version "
        "8.4.0+2021r2-patch5. -ffile-prefix-map removes the checkout path and "
        "PlatformIO home. bootloader.bin and partitions.bin omit those paths."
    ),
    "host": {
        "system": platform.system(),
        "machine": platform.machine(),
        "python": platform.python_version(),
        "python_bits": struct.calcsize("P") * 8,
    },
    "toolchain": {
        "platformio_core": subprocess.check_output([pio, "--version"], text=True).strip(),
        "xtensa_esp32s3_system": toolchain_system,
        "resolved_packages": subprocess.check_output(
            [pio, "pkg", "list", "-e", "atoms3-lite-usb"], cwd=root, text=True
        ).strip(),
    },
    "flash": {
        "supported_command": ".venv/bin/pio run -e atoms3-lite-usb -t upload",
        "mode": "dio",
        "freq": "80m",
        "size": "8MB",
        "offsets": {
            "bootloader.bin": "0x0",
            "partitions.bin": "0x8000",
            "boot_app0.bin": "0xe000",
            "firmware.bin": "0x10000",
        },
    },
    "files": files,
}
(out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
print(json.dumps(manifest, indent=2))
