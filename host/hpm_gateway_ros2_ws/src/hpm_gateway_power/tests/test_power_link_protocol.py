"""PowerLink v1 协议模块单元测试（纯 Python，无需 ROS/串口/硬件）。

运行：
    python3 -m unittest tests.test_power_link_protocol
    # 或在包目录内： python3 -m unittest -v
"""

import unittest

from hpm_gateway_power import power_link_protocol as pl


class TestCrc16(unittest.TestCase):
    def test_known_vector(self):
        # CRC-16/CCITT-FALSE 标准校验向量："123456789" -> 0x29B1
        self.assertEqual(pl.crc16_ccitt_false(b"123456789"), 0x29B1)

    def test_empty(self):
        self.assertEqual(pl.crc16_ccitt_false(b""), 0xFFFF)


class TestFrameBuildParse(unittest.TestCase):
    def _round_trip(self, cmd, payload, seq):
        frame = pl.build_frame(seq, cmd, payload)
        # 帧长 = 1(SOF) + 1(LEN) + 1(SEQ) + 1(CMD) + len + 2(CRC)
        self.assertEqual(len(frame), 6 + len(payload))
        parser = pl.PowerLinkFrameParser()
        result = None
        for byte in frame:
            result = parser.feed(byte)
        self.assertIsNotNone(result)
        fcmd, fpayload, fseq = result
        self.assertEqual(fcmd, cmd)
        self.assertEqual(fpayload, payload)
        self.assertEqual(fseq, seq)
        self.assertEqual(parser.bad_frame_count, 0)

    def test_round_trip_commands(self):
        self._round_trip(pl.CMD_HEARTBEAT, b"\x00\x01", 0x12)
        self._round_trip(pl.CMD_SET_POWER, b"\x07\x01", 0x34)
        self._round_trip(pl.CMD_GET_STATUS, b"", 0x56)
        self._round_trip(pl.CMD_SET_POWER_ALL, b"\x03\xFF", 0x78)
        self._round_trip(pl.CMD_STATUS, b"\x00\x00\x27\x10\x80\x00\x80\x00\x03\xFF\x03", 0x00)

    def test_bad_crc_rejected(self):
        frame = bytearray(pl.build_frame(0x01, pl.CMD_SET_POWER, b"\x07\x01"))
        frame[-1] ^= 0xFF  # 翻转 CRC 高字节
        parser = pl.PowerLinkFrameParser()
        result = None
        for byte in frame:
            result = parser.feed(byte)
        self.assertIsNone(result)
        self.assertEqual(parser.bad_frame_count, 1)

    def test_len_too_large_resync(self):
        parser = pl.PowerLinkFrameParser()
        parser.feed(pl.SOF)
        parser.feed(0xFF)  # LEN > 250 -> 坏帧并回到搜 SOF
        self.assertEqual(parser.bad_frame_count, 1)
        # 之后仍能正常解析一帧
        frame = pl.build_frame(1, pl.CMD_GET_STATUS, b"")
        result = None
        for byte in frame:
            result = parser.feed(byte)
        self.assertIsNotNone(result)

    def test_sof_in_payload_no_stuffing(self):
        # 载荷中出现 0xA5 不应导致误判
        self._round_trip(pl.CMD_SET_POWER, bytes([0xA5, 0xA5]), 1)


class TestStatusDecode(unittest.TestCase):
    def test_decode(self):
        # VBAT=10000mV, NTC1=25.5°C(255), NTC2=-32768(无效), STATE=0x03FF, AL=0x01
        payload = bytes([0x00, 0x00, 0x27, 0x10,
                         0x00, 0xFF, 0x80, 0x00,
                         0x03, 0xFF, 0x01])
        s = pl.parse_status(payload)
        self.assertEqual(s["battery_mv"], 10000)
        self.assertEqual(s["ntc1_deci_c"], 255)
        self.assertEqual(s["ntc2_deci_c"], -32768)
        self.assertEqual(s["output_state"], 0x03FF)
        self.assertEqual(s["alarms"], 0x01)


class TestChannelMap(unittest.TestCase):
    def test_names(self):
        self.assertEqual(pl.CHANNEL_NAMES[0], "12V_1")
        self.assertEqual(pl.CHANNEL_NAMES[4], "5V")
        # 48V 顺序以硬件为准：M4=ch6, M1=ch7, M2=ch8, M3=ch9
        self.assertEqual(pl.MOTOR_TO_CHANNEL, {"M1": 7, "M2": 8, "M3": 9, "M4": 6})
        self.assertEqual(pl.CHANNEL_NAMES[6], "48V_M4")
        self.assertEqual(pl.CHANNEL_NAMES[9], "48V_M3")


if __name__ == "__main__":
    unittest.main()
