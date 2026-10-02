# Contributing to the ESP32 SDK

Work here on the reusable C++ SDK, portable utilities, C124 firmware, host regressions and hardware instructions. Gateway transport/authentication belongs in the [gateway](https://github.com/adidshaft/grok-gadgets-gateway); shared architecture and canonical contribution policies live in the [hub](https://github.com/adidshaft/grok-gadgets/blob/main/CONTRIBUTING.md).

You do not need a board to improve host tests, documentation or custom-capability examples. Physical testing requires the exact C124 and separate safe test preparation. Report the evidence level explicitly.

## Make one focused change

1. Choose a [ready issue](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues?q=is%3Aissue+is%3Aopen+label%3A%22help+wanted%22), or discuss an interface/protocol/large feature first. Typo fixes can go directly to a PR. Until publication, use [stable local IDs](planning/issues.json).
2. Fork the repository after activation, clone your fork, and create a short branch, for example `git switch -c docs/clarify-build`.
3. Follow the standalone [README setup](README.md#start-without-hardware). Change one behavior or document journey; preserve the pinned contract and dependency versions unless the issue calls for coordinated changes.
4. Run relevant checks from the repository root:

   ```sh
   sh tools/check.sh
   .venv/bin/python tools/check_contract.py
   .venv/bin/pio run -e atoms3-lite-usb
   ```

   Host checks cover formatting, lint and CTest; contract validation checks the SDK-generated wire frames. Firmware changes require the exact target compile. Documentation changes should rehearse the edited steps and check links. Record any unrun physical or Linux USB steps.
5. In the PR, link the issue and explain the behavior before/after, commands/results, docs updates and remaining limitations. Keep tokens, household data, device identifiers and raw account captures out of logs.
6. Respond to review with focused commits. The maintainer integrates the tested change into `main` and credits documentation, tests and code contributions. Do not force-push shared work or imply agent review is independent human reproduction.

## Integration and protocol changes

The [canonical protocol](https://github.com/adidshaft/grok-gadgets-gateway/tree/main/protocol/0.1.0) belongs to the gateway. Coordinate schema changes and consumer pins together; never edit this SDK's copy in isolation. The hub records the tested component combination.

Optional USB/gateway integration uses a separate gateway checkout as a sibling, its pinned environment installed, and `../grok-gadgets-gateway/.venv/bin/python tools/check_gateway.py`. This is a development workspace command, not a standalone build requirement. The host simulation reports simulated identity; a firmware ACK is not physical verification.

Original contributions use Apache-2.0 without an additional CLA or sign-off requirement. You remain responsible for AI-assisted code, citations and claimed test results. Follow the shared [Code of Conduct](https://github.com/adidshaft/grok-gadgets/blob/main/CODE_OF_CONDUCT.md) and [governance](https://github.com/adidshaft/grok-gadgets/blob/main/GOVERNANCE.md). For private conduct reports email [adidshaft@kyokasuigetsu.xyz](mailto:adidshaft@kyokasuigetsu.xyz); use [Security](SECURITY.md) for vulnerabilities.
