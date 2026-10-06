"""Check the library manifests agree and that PlatformIO can pack the library.

    .venv/bin/python tools/check_package.py

Packing writes a tarball to a temporary folder only; nothing is published.
"""

import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LIBRARY = ROOT / "lib/GrokGadgets"


def main():
    platformio = json.loads((LIBRARY / "library.json").read_text())
    arduino = dict(
        line.split("=", 1)
        for line in (LIBRARY / "library.properties").read_text().splitlines()
        if "=" in line
    )
    if (platformio["name"], platformio["version"]) != (arduino["name"], arduino["version"]):
        raise SystemExit("library.json and library.properties disagree on name or version")
    json_dependency = platformio["dependencies"]["bblanchon/ArduinoJson"]
    if f"ArduinoJson (={json_dependency})" != arduino["depends"]:
        raise SystemExit("ArduinoJson versions differ between the two manifests")
    for name in ("LICENSE", "NOTICE"):
        if (LIBRARY / name).read_bytes() != (ROOT / name).read_bytes():
            raise SystemExit(f"lib/GrokGadgets/{name} must match the repository {name}")
    with tempfile.TemporaryDirectory() as folder:
        subprocess.run(
            [str(ROOT / ".venv/bin/pio"), "pkg", "pack", str(LIBRARY), "-o", folder],
            check=True,
        )
    print(f"package: GrokGadgets {platformio['version']} manifests agree and pack (not published)")


if __name__ == "__main__":
    sys.exit(main())
