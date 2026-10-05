# Contributing to the ESP32 SDK

Contribute to the reusable C++ SDK, portable utilities, C124 firmware, tests and hardware instructions. The [gateway](https://github.com/adidshaft/grok-gadgets-gateway) owns transport and authentication. The [hub](https://github.com/adidshaft/grok-gadgets/blob/main/CONTRIBUTING.md) owns shared architecture and contribution policy.

Use the [writing guide](https://github.com/adidshaft/grok-gadgets/blob/main/docs/contributing/writing-guide.md) for documentation.

You do not need a board to improve host tests, documentation or custom-capability examples. Physical testing requires the exact C124 and separate safe test preparation. Report the evidence level explicitly.

## Make one focused change

1. Choose a [ready issue](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues?q=is%3Aissue+is%3Aopen+label%3A%22help+wanted%22), or discuss an interface/protocol/large feature first. Typo fixes can go directly to a PR. The [local ledger](planning/issues.json) records preparation work.
2. Fork the repository. Clone your fork and create a short branch. For example: `git switch -c docs/clarify-build`.
3. Follow the standalone [README setup](README.md#start-without-hardware). Change one behavior or procedure. Keep protocol and dependency pins unless the issue requires coordinated changes.
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

## Ignore rules and publication privacy

Update `.gitignore` for new caches, build output, local device configuration, logs and credentials. Keep reviewed sample configuration and the hub’s public simulator download.

Check new patterns with `git check-ignore`. Review staged files before each commit. Ignore rules do not remove tracked files or history. Never merge private pre-publication history into a public branch. Use the sanitized public checkout and a public or GitHub noreply commit email.
