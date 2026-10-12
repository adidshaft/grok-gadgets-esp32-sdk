# USB framing and lifecycle

The gateway owns canonical protocol **0.1.0**. This repository copies it into `protocol/0.1.0`. The source manifest records SHA-256 hashes. The copied protocol README records its original transport scope. Use this guide for current USB and Wi-Fi support.

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
Public HTTPS and native Grok Bot MCP calls remain unverified; see `HARD-GROK-REMOTE-001` and the
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

A request times out after 13 seconds. The USB bridge waits 2 seconds to connect and 10 seconds for a reply. The firmware does not apply late replies to another request. Hello retries start at 500 ms and increase up to five seconds. The delay returns to 500 ms after a successful request other than hello.

A reply must be one JSON object whose fields match the outstanding request. Trailing bytes, a second object, an embedded NUL, or the wrong shape starts a new session. Firmware drops the old pending acknowledgement. The gateway records dispatched commands as uncertain when their results are unknown.

A permanently rejected queued event (`duplicate_conflict`, `invalid_event`, `invalid_request`, `unsupported_capability`, `invalid_state`) is dropped, counted in `history_lost`, and polling continues. Other event failures reconnect. After five failures the event is dropped.

The gateway can answer an acknowledgement with `late_ack` or `unknown_command`. The firmware drops that acknowledgement and keeps polling on the same session. It does not resend it or reconnect. `gadget.droppedAcks()` counts these. See the [canonical protocol](../protocol/0.1.0/README.md#late-acknowledgements-late_ack).

Firmware never treats an acknowledgement as a command. It does not automatically retry an action. Correct credentials or protocol settings after authorization, revocation or version errors. Then reconnect. Firmware does not print secrets.

## Button events

The button debounce period is 30 ms. Press and release events use sequence IDs unique within a boot. The gateway retains events for a client to read; a button press does not start a Grok Bot task.

The queue holds 16 events. On overflow, it drops new events. After retained events drain, a `history_lost` event reports the dropped count. Retries of that report reuse one event ID and the same count, so a lost reply is not counted twice. A successful reply clears only the reported count. Losses during the report wait for the next ID.

Hello declares `history_lost`, `rgb.set`, `button`, and `state`. Custom event names also appear there, and their `capability_schemas` entries include `"x-grok-gadgets-kind": "event"`. `history_lost` is reserved and is not a sketch-registered name.

Current debounced button state appears in hello, acknowledgements and periodic state. Periodic state is sent every three seconds.

Retained events can be retried after reconnect with the same ID. The gateway retains 256 newly accepted event IDs per device and boot, for up to 64 devices. One busy device cannot evict another device's entries.

Same-boot reconnect keeps this cache. A new boot clears it. Retries do not extend retention. Gateway restart can cause a retried event to appear again.

Device reboot removes the RAM event queue and acknowledgement cache. USB firmware has no synchronized clock, so events do not claim an `observed_at` time. Use gateway `received_at` instead.

## Wi-Fi transport

The OLED kit supports Wi-Fi with TLS and a pinned gateway CA. USB provisioning stores the device token and network settings.
See the [Wi-Fi example](../examples/oled-kit/README.md#wi-fi-build) and [physical acceptance record](verification/oled-kit-wifi-2026-10-12.md).
Keep the Mac gateway running while the board uses a USB charger.

Wi-Fi transport does not prove native Grok Bot MCP access. The tested Grok Bot route runs an approved local command.
A button event does not wake Grok Bot. C124 Wi-Fi operation remains untested.

Keep credentials out of Git. The board stores secrets without flash encryption. Revoke its token and change Wi-Fi credentials if you lose it.
Do not expose the unencrypted loopback device port to a network. Use the gateway's separate LAN TLS listener for Wi-Fi.
