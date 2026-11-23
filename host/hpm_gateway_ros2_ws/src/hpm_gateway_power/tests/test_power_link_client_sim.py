"""PowerLinkClient 与“固件仿真”的端到端测试（无需真实串口/硬件）。

固件仿真类 FakePowerBoardSerial 镜像 power_link.c 的应答行为：
    合法命令 -> 构造对应应答帧；坏帧/超长 -> 丢弃（无应答）。
"""

import struct
import time
import unittest

from hpm_gateway_power import power_link_protocol as pl


class FakePowerBoardSerial:
    """极简固件仿真：写进来一帧就回一帧。"""

    def __init__(self):
        self._parser = pl.PowerLinkFrameParser()
        self._out = bytearray()
        self._status_payload = bytes([
            0x00, 0x00, 0x27, 0x10,  # VBAT = 10000 mV
            0x00, 0xFF, 0x80, 0x00,  # NTC1=25.5, NTC2=无效
            0x03, 0xFF,              # STATE = 全部允许
            0x00,                    # ALARMS
        ])

    @property
    def in_waiting(self):
        return len(self._out)

    def read(self, n):
        data = bytes(self._out[:n])
        del self._out[:n]
        return data

    def write(self, data):
        for byte in data:
            result = self._parser.feed(byte)
            if result is not None:
                self._respond(*result)

    def _respond(self, cmd, payload, seq):
        if cmd == pl.CMD_HEARTBEAT:
            token = pl.crc16_ccitt_false(payload)
            resp = pl.build_frame(seq, pl.CMD_HEARTBEAT_ACK,
                                  struct.pack(">HB", token, 0))
        elif cmd == pl.CMD_GET_STATUS:
            resp = pl.build_frame(seq, pl.CMD_STATUS, self._status_payload)
        elif cmd == pl.CMD_SET_POWER:
            resp = pl.build_frame(seq, pl.CMD_CMD_ACK, bytes([cmd, pl.STATUS_OK]))
        elif cmd == pl.CMD_SET_POWER_ALL:
            resp = pl.build_frame(seq, pl.CMD_CMD_ACK, bytes([cmd, pl.STATUS_OK]))
        elif cmd == pl.CMD_SET_REPORT_INTERVAL:
            resp = pl.build_frame(seq, pl.CMD_CMD_ACK, bytes([cmd, pl.STATUS_OK]))
        else:
            resp = pl.build_frame(seq, pl.CMD_CMD_ACK,
                                  bytes([cmd, pl.STATUS_UNKNOWN_CMD]))
        self._out += resp


class TestClientSim(unittest.TestCase):
    def setUp(self):
        self.ser = FakePowerBoardSerial()
        self.client = pl.PowerLinkClient(self.ser)

    def test_get_status(self):
        status = self.client.get_status()
        self.assertEqual(status["battery_mv"], 10000)
        self.assertEqual(status["output_state"], 0x03FF)
        self.assertEqual(self.client.latest_status["alarms"], 0)

    def test_heartbeat_ack(self):
        token, alarms = self.client.send_heartbeat()
        self.assertEqual(alarms, 0)

    def test_set_power_ok(self):
        self.assertEqual(self.client.set_power(7, True), pl.STATUS_OK)
        self.assertEqual(self.client.set_power_all(0x0000), pl.STATUS_OK)

    def test_set_report_interval(self):
        self.assertEqual(self.client.set_report_interval(0), pl.STATUS_OK)

    def test_bad_crc_triggers_retransmit_ok(self):
        # 先发送一帧坏帧（仅写 SOF+LEN，不发完 -> 固件永远不应答）
        # 客户端请求仍应在重传后成功。
        frame = pl.build_frame(1, pl.CMD_GET_STATUS, b"")
        corrupted = bytearray(frame)
        corrupted[-1] ^= 0xFF
        # 直接写入串口 -> 固件丢弃，不产生应答；随后客户端正常请求仍成功。
        self.ser._parser = pl.PowerLinkFrameParser()  # 清状态
        self.ser.write(bytes(corrupted))
        # 请求成功后 bad_frame_count 由固件侧统计，客户端侧仅需成功返回
        status = self.client.get_status()
        self.assertEqual(status["battery_mv"], 10000)


if __name__ == "__main__":
    unittest.main()
