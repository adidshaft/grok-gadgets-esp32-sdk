# SPDX-License-Identifier: Apache-2.0
import unittest

from rehearse import rehearse


class ServoRehearsalTests(unittest.TestCase):
    def test_moves_and_refusal(self):
        acks = [f for f in rehearse() if f["type"] == "ack"]
        self.assertEqual([a["status"] for a in acks], ["executed", "failed", "executed"])
        self.assertEqual([a["state"]["servo"]["angle"] for a in acks], [0, 0, 120])
        self.assertEqual(acks[1]["error"]["code"], "invalid_arguments")


if __name__ == "__main__":
    unittest.main()
