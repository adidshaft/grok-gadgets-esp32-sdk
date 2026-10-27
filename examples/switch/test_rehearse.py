# SPDX-License-Identifier: Apache-2.0
import unittest

from rehearse import rehearse


class SwitchRehearsalTests(unittest.TestCase):
    def test_on_again_refused_off(self):
        acks = [f for f in rehearse() if f["type"] == "ack"]
        self.assertEqual(
            [a["status"] for a in acks], ["executed", "executed", "failed", "executed"]
        )
        self.assertEqual([a["state"]["switch"]["on"] for a in acks], [True, True, True, False])
        self.assertEqual(acks[2]["error"]["code"], "invalid_arguments")


if __name__ == "__main__":
    unittest.main()
