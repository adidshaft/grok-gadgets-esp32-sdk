# SPDX-License-Identifier: Apache-2.0
import unittest

from rehearse import KitModel


class KitModelTests(unittest.TestCase):
    def test_text_and_clear(self):
        kit = KitModel()
        self.assertEqual(kit.set_text("Grok Bot ready")["display"]["text"], "Grok Bot ready")
        self.assertEqual(kit.clear()["display"]["text"], "")
        self.assertTrue(kit.status()["simulated"])

    def test_invalid_input_preserves_state(self):
        kit = KitModel()
        kit.set_text("Previous")
        for invalid in (None, 1, True, "x" * 33, "line\nbreak", "\x7f", "é"):
            with self.subTest(invalid=invalid):
                with self.assertRaises(ValueError):
                    kit.set_text(invalid)
                self.assertEqual(kit.text, "Previous")
        kit.set_text("x" * 32)
        self.assertEqual(len(kit.text), 32)

    def test_ordered_button_edges(self):
        kit = KitModel()
        kit.button_edge(True)
        kit.button_edge(True)
        kit.button_edge(False)
        self.assertEqual(
            kit.events, [{"sequence": 1, "pressed": True}, {"sequence": 2, "pressed": False}]
        )
        with self.assertRaises(ValueError):
            kit.button_edge(1)
        self.assertFalse(kit.pressed)

    def test_disconnect_and_recovery(self):
        kit = KitModel()
        kit.set_text("Before")
        kit.disconnect()
        for action in (lambda: kit.set_text("After"), kit.clear, lambda: kit.button_edge(True)):
            with self.assertRaises(ConnectionError):
                action()
        self.assertFalse(kit.status()["online"])
        self.assertEqual(kit.text, "Before")
        self.assertEqual(kit.reconnect()["display"]["text"], "Before")
        kit.button_edge(True)
        fresh = kit.reboot()
        self.assertEqual(fresh["boot"], 2)
        self.assertEqual(fresh["display"]["text"], "")
        self.assertFalse(fresh["button"]["pressed"])
        self.assertEqual(kit.events, [])


if __name__ == "__main__":
    unittest.main()
