"""Run the README Quickstart block exactly as written, from the repository root.

    python3 tools/check_readme.py

CI runs it in a fresh checkout, so the README cannot drift from what works.
It builds and tests software only; it never flashes or opens a board.
"""

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    readme = (ROOT / "README.md").read_text()
    section = readme.split("## Quickstart", 1)[1].split("\n## ", 1)[0]
    match = re.search(r"```sh\n(.*?)```", section, re.S)
    if not match:
        raise SystemExit("README Quickstart has no sh block")
    print(match[1], flush=True)
    subprocess.run(["bash", "-euo", "pipefail", "-c", match[1]], cwd=ROOT, check=True)
    print("README Quickstart passed (software build and host tests; no board).")


if __name__ == "__main__":
    sys.exit(main())
