#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Controller-independent first-kit model. No GPIO, USB or display driver."""

import json


class KitModel:
    """Logical behavior only; these operations are not gateway capabilities."""

    def __init__(self):
        self.text = ""
        self.online = True
        self.boot = 1
        self.pressed = False
        self.events = []

    def status(self):
        return {
            "simulated": True,
            "online": self.online,
            "boot": self.boot,
            "display": {"text": self.text},
            "button": {"pressed": self.pressed},
        }

    def _connected(self):
        if not self.online:
            raise ConnectionError("Model is offline; no command was applied")

    def set_text(self, text):
        self._connected()
        if not isinstance(text, str) or len(text) > 32:
            raise ValueError("Expected at most 32 printable ASCII characters")
        if any(ord(char) < 32 or ord(char) > 126 for char in text):
            raise ValueError("Expected at most 32 printable ASCII characters")
        self.text = text
        return self.status()

    def clear(self):
        return self.set_text("")

    def button_edge(self, pressed):
        """Inject a simulated input; this does not command a physical button."""
        self._connected()
        if type(pressed) is not bool:
            raise ValueError("Expected boolean button input")
        if pressed != self.pressed:
            self.pressed = pressed
            self.events.append({"sequence": len(self.events) + 1, "pressed": pressed})
        return self.status()

    def disconnect(self):
        self.online = False

    def reconnect(self):
        """Reopen the model transport without asserting a physical USB outcome."""
        self.online = True
        return self.status()

    def reboot(self):
        self.boot += 1
        self.online = True
        self.text = ""
        self.pressed = False
        self.events = []
        return self.status()


def main():
    kit = KitModel()
    print(json.dumps({"step": "text", "state": kit.set_text("Grok Bot ready")}))
    kit.button_edge(True)
    kit.button_edge(False)
    print(json.dumps({"step": "button", "simulated": True, "events": kit.events}))
    print(json.dumps({"step": "clear", "state": kit.clear()}))
    kit.disconnect()
    print(json.dumps({"step": "disconnect", "state": kit.status()}))
    print(json.dumps({"step": "reconnect", "state": kit.reconnect()}))
    print(json.dumps({"step": "reboot", "state": kit.reboot()}))


if __name__ == "__main__":
    main()
