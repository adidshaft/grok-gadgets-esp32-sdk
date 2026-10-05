# USB framing and lifecycle

The gateway owns canonical protocol **0.1.0**. This repository copies it into `protocol/0.1.0`. The source manifest records SHA-256 hashes.

## Frame format

Frames contain UTF-8 JSON followed by LF. Each frame can contain at most 2048 bytes, including LF.

The firmware line buffer accepts 2047 bytes before LF. For oversized data, it drops bytes through the next LF and resets the session. A partial frame remains between loop iterations. Timeout clears it.

Send debug logs through a different interface.

## USB request sequence

1. Send hello.
2. Poll every 100 ms.
3. Send acknowledgements, events and state as needed.
4. Wait for one reply to each request.

The gateway does not send unsolicited commands. Each poll returns at most one command. Serial framing speed is 115200. C124 uses USB CDC; no separate UART adapter is needed.

The USB hello contains no credentials. The local bridge adds the host's per-device token to the loopback TCP hello. Physical USB access and host access affect security.

Run the bridge and gateway on the same host. The device protocol is not an HTTPS MCP endpoint.
Do not expose it through a tunnel. A tunnel does not add gateway authentication.
The gateway's separate `serve` command provides authenticated HTTP MCP on loopback.
Public HTTPS and Grok Bot compatibility are not verified; see `HARD-GROK-REMOTE-001` and the
[hosting FAQ](https://github.com/adidshaft/grok-gadgets/blob/main/docs/getting-started/hosting.md).

## Startup and identity

The LED starts off. Firmware publishes:

- A device ID of `c124-` plus the MAC bytes in esptool order.
- A random boot ID.
- Protocol and firmware versions.
- Capability names.
- Real-device identity: `simulated:false`.

The host test harness uses `simulated:true`. Physical firmware exposes no test-injection controls.

An RGB acknowledgement follows the return of the pixel driver's `show()` function. It does not prove that someone observed the light.

## Recover a session

A request times out after 13 seconds. That is longer than the USB bridge's 2-second connect wait plus its 10-second reply wait, so a late reply is not applied to the next request. Hello retries start at 500 ms and increase up to five seconds. The delay returns to 500 ms after a successful request other than hello.

A reply must be one JSON object whose fields match the outstanding request. Trailing bytes, a second object, an embedded NUL, or the wrong shape starts a new session. Firmware drops the old pending acknowledgement. The gateway records dispatched commands as uncertain when their results are unknown.

A permanently rejected queued event (`duplicate_conflict`, `invalid_event`, `invalid_request`, `unsupported_capability`, `invalid_state`) is dropped, counted in `history_lost`, and polling continues. Other event failures reconnect. After five failures the event is dropped.

Firmware never treats an acknowledgement as a command. It does not automatically retry an action. Correct credentials or protocol settings after authorization, revocation or version errors. Then reconnect. Firmware does not print secrets.

## Button events

The button debounce period is 30 ms. Press and release events use sequence IDs unique within a boot. The gateway retains events for a client to read; a button press does not start a Grok Bot task.

The queue holds 16 events. On overflow, it drops new events. After retained events drain, a `history_lost` event reports the dropped count. Retries of that report reuse one event ID and the same count, so a lost reply is not counted twice. A successful reply clears only the reported count. Losses during the report wait for the next ID.

Hello declares `history_lost`, `rgb.set`, `button`, and `state`. Custom event names also appear there, and their `capability_schemas` entries include `"x-grok-gadgets-kind": "event"`. `history_lost` is reserved and is not a sketch-registered name.

Current debounced button state appears in hello, acknowledgements and periodic state. Periodic state is sent every three seconds.

Retained events can be retried after reconnect with the same ID. The gateway retains 256 newly accepted event IDs per device and boot, for up to 64 devices. One busy device cannot evict another device's entries.

Same-boot reconnect keeps this cache. A new boot clears it. Retries do not extend retention. Gateway restart can cause a retried event to appear again.

Device reboot removes the RAM event queue and acknowledgement cache. USB firmware has no synchronized clock, so events do not claim an `observed_at` time. Use gateway `received_at` instead.

## Future Wi-Fi support

Wi-Fi transport is not implemented in this alpha. The gateway's device listener is loopback-only and unencrypted. Wi-Fi needs an authenticated, reachable transport and secure credential setup. This is tracked as ESP-005.

Wi-Fi device transport and cloud Bot MCP access are separate work.
Neither creates the other. Future customer-hosted and maker-hosted services are product
options, not shipped features. The hosting FAQ keeps their operation and security requirements together.

Before adding that transport:

1. Keep per-device credentials out of Git.
2. Support credential revocation.
3. Add TLS certificate validation.
4. Define credential setup and reset behavior.
5. Reuse the SDK's bounded command and event handling.

Do not expose plaintext credentials on a LAN or public network. C124 supports Wi-Fi, but this project has not verified its radio operation.
