# SPDX-License-Identifier: Apache-2.0
import unittest

from rehearse import rehearse


class ClimateRehearsalTests(unittest.TestCase):
    def test_threshold_and_crossing(self):
        frames = rehearse()
        acks = {f["command_id"]: f for f in frames if f["type"] == "ack"}
        self.assertEqual(acks["r1"]["status"], "executed")
        self.assertEqual(acks["r1"]["state"]["climate"]["above_c"], 24)
        self.assertEqual(acks["r2"]["error"]["code"], "invalid_arguments")
        self.assertEqual(acks["r2"]["state"]["climate"]["above_c"], 24)
        crossed = [f["data"] for f in frames if f.get("name") == "threshold.crossed"]
        self.assertEqual(crossed[0], {"reading": "temperature_c", "value": 25, "above": True})


if __name__ == "__main__":
    unittest.main()
