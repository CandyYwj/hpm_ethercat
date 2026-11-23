"""PowerLink v1 协议客户端（纯 Python，无 ROS 依赖）。

与电源板固件 `App/Src/power_link.c` 逐字段对齐：

- 帧格式：SOF(0xA5) + LEN + SEQ + CMD + PAYLOAD + CRC16(低字节在前)
- CRC16 = CRC-16/CCITT-FALSE（poly 0x1021, init 0xFFFF），覆盖 LEN+SEQ+CMD+PAYLOAD
- 多字节字段一律大端（MSB first）
- 应答：HEARTBEAT_ACK(0x81) / CMD_ACK(0x82) / STATUS(0x83)

本模块只负责协议编解码与串口收发，不依赖 ROS，便于独立测试与复用。
"""

import struct
import threading
import time

# ---- 帧常量 ----
SOF = 0xA5
MAX_PAYLOAD = 250

# ---- 命令码 ----
CMD_HEARTBEAT = 0x01
CMD_SET_POWER = 0x02
CMD_GET_STATUS = 0x03
CMD_SET_REPORT_INTERVAL = 0x04
CMD_SET_POWER_ALL = 0x05
CMD_HEARTBEAT_ACK = 0x81
CMD_CMD_ACK = 0x82
CMD_STATUS = 0x83

# ---- 状态码（CMD_ACK 的 STATUS 字段） ----
STATUS_OK = 0x00
STATUS_UNKNOWN_CMD = 0x01
STATUS_BAD_FRAME = 0x02
STATUS_BAD_CHANNEL = 0x03
STATUS_BAD_PARAM = 0x04
STATUS_BUSY = 0x05
STATUS_NOT_SUPPORTED = 0x06

STATUS_NAMES = {
    STATUS_OK: "OK",
    STATUS_UNKNOWN_CMD: "UNKNOWN_CMD",
    STATUS_BAD_FRAME: "BAD_FRAME",
    STATUS_BAD_CHANNEL: "BAD_CHANNEL",
    STATUS_BAD_PARAM: "BAD_PARAM",
    STATUS_BUSY: "BUSY",
    STATUS_NOT_SUPPORTED: "NOT_SUPPORTED",
}

# ---- 告警位 ----
ALARM_LOW_VOLTAGE = 1 << 0
ALARM_COMM_LOST = 1 << 1

# ---- 通道映射（与固件 power_board_app.h / 硬件基线一致） ----
# 注意：协议文档初稿把 48V 标成了 M1..M4 顺排，但硬件/固件实际为：
#   PA8 = M4, PB13 = M1, PB14 = M2, PB15 = M3
# 因此 CH6..9 = 48V_M4, M1, M2, M3。主机侧以本表为准。
CHANNEL_NAMES = {
    0: "12V_1",
    1: "12V_2",
    2: "12V_FAN",
    3: "12V_4",
    4: "5V",
    5: "12V_3",
    6: "48V_M4",
    7: "48V_M1",
    8: "48V_M2",
    9: "48V_M3",
}
CHANNEL_BY_NAME = {name: ch for ch, name in CHANNEL_NAMES.items()}
# 电机名（按物理电机号）到协议通道号的映射。
MOTOR_TO_CHANNEL = {"M1": 7, "M2": 8, "M3": 9, "M4": 6}

LOW_VOLTAGE_MASK = 0x003F
FORTY_EIGHT_V_MASK = 0x03C0
VALID_CHANNEL_MASK = 0x03FF

CHANNEL_ALL = 0xFF  # ROS PowerCommand 中表示“批量”的哨兵值


def crc16_ccitt_false(data):
    """CRC-16/CCITT-FALSE：poly 0x1021，init 0xFFFF，按位计算（与固件一致）。"""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def build_frame(seq, cmd, payload=b""):
    """构造一帧：SOF + LEN + SEQ + CMD + PAYLOAD + CRC16(低字节在前)。"""
    payload = bytes(payload)
    if len(payload) > MAX_PAYLOAD:
        raise ValueError("payload too long: %d" % len(payload))
    head = bytes((SOF, len(payload), seq & 0xFF, cmd))
    crc = crc16_ccitt_false(head[1:] + payload)
    return head + payload + bytes((crc & 0xFF, (crc >> 8) & 0xFF))


def parse_status(payload):
    """解析 STATUS(0x83) 载荷（11 字节大端）为字典。"""
    if len(payload) < 11:
        raise ValueError("status payload too short: %d" % len(payload))
    battery_mv, ntc1, ntc2, state, alarms = struct.unpack(">IhhHB", payload[:11])
    return {
        "battery_mv": battery_mv,
        "ntc1_deci_c": ntc1,
        "ntc2_deci_c": ntc2,
        "output_state": state,
        "alarms": alarms,
    }


class PowerLinkError(Exception):
    """协议/链路错误（坏帧、超时等）。"""


class PowerLinkFrameParser:
    """逐字节状态机接收解析器，逻辑镜像固件 power_link.c 的解析器。

    帧内可能包含任意 0xA5，由于以 LEN 定界，无需字节填充。
    """

    WAIT_SOF = 0
    LENGTH = 1
    DATA = 2

    def __init__(self):
        self._state = self.WAIT_SOF
        self._frame_length = 0  # 待收的 SEQ+CMD+PAYLOAD+CRC 字节数
        self._frame = bytearray()
        self._len_byte = 0
        self.bad_frame_count = 0

    def feed(self, byte):
        """送入一个字节，若拼完一帧则返回 (cmd, payload, seq)，否则返回 None。"""
        if self._state == self.WAIT_SOF:
            if byte == SOF:
                self._state = self.LENGTH
            return None

        if self._state == self.LENGTH:
            if byte > MAX_PAYLOAD:
                self.bad_frame_count += 1
                self._state = self.WAIT_SOF
                return None
            self._len_byte = byte
            self._frame_length = byte + 4  # SEQ + CMD + PAYLOAD + CRC
            self._frame = bytearray()
            self._state = self.DATA
            return None

        # DATA
        self._frame.append(byte)
        if len(self._frame) < self._frame_length:
            return None

        received_crc = (self._frame[self._frame_length - 2]
                        | (self._frame[self._frame_length - 1] << 8))
        crc_data = bytes([self._len_byte]) + bytes(self._frame[:self._frame_length - 2])
        if crc16_ccitt_false(crc_data) != received_crc:
            self.bad_frame_count += 1
            self._state = self.WAIT_SOF
            return None

        seq = self._frame[0]
        cmd = self._frame[1]
        payload = bytes(self._frame[2:self._frame_length - 2])
        self._state = self.WAIT_SOF
        return cmd, payload, seq


class PowerLinkClient:
    """PowerLink 主机端客户端：串口收发 + 序号 + 应答匹配 + 重传。

    用法：
        client = PowerLinkClient(serial)
        client.set_report_interval(0)          # 关闭板卡周期推送，改由主机轮询
        client.send_heartbeat()
        status = client.get_status()           # -> dict，失败抛 PowerLinkError
        client.set_power(7, True)              # 使能 48V_M1（PB13）
        client.set_power_all(0x03FF)           # 全部允许
    """

    def __init__(self, serial, heartbeat_token=0):
        self._serial = serial
        self._lock = threading.Lock()
        self._parser = PowerLinkFrameParser()
        self._seq = 0
        self._token = heartbeat_token & 0xFFFF
        self.latest_status = None      # 最近收到的 STATUS（dict）
        self.last_rx_ms = 0.0
        self.bad_frame_count = 0

    # ---- 底层收发 ----

    def _next_seq(self):
        self._seq = (self._seq + 1) & 0xFF
        return self._seq

    def _drain_and_parse(self, timeout_s):
        """读串口直到出现整帧；返回解析出的帧列表。非阻塞式收集。"""
        frames = []
        deadline = time.monotonic() + timeout_s
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                break
            available = self._serial.in_waiting
            if available <= 0:
                time.sleep(0.001)
                continue
            chunk = self._serial.read(available)
            for byte in chunk:
                result = self._parser.feed(byte)
                if result is not None:
                    frames.append(result)
            if frames:
                break
        return frames

    def _handle_frame(self, cmd, payload, seq):
        if cmd == CMD_STATUS:
            self.latest_status = parse_status(payload)
        return cmd, payload, seq

    def _transact(self, cmd, payload, expect_cmd, timeout_s=0.15, retries=3):
        """发送一帧并等待期望的应答帧，带重传。返回 (cmd, payload, seq)。"""
        with self._lock:
            last_error = None
            for _ in range(retries + 1):
                seq = self._next_seq()
                self._serial.write(build_frame(seq, cmd, payload))
                deadline = time.monotonic() + timeout_s
                while time.monotonic() < deadline:
                    for frame in self._drain_and_parse(0.02):
                        fcmd, fpayload, fseq = self._handle_frame(*frame)
                        if fcmd == expect_cmd:
                            if fcmd == CMD_CMD_ACK:
                                # CMD_ACK 载荷首字节回显原命令码
                                if len(fpayload) < 1 or fpayload[0] != cmd:
                                    continue
                            return fcmd, fpayload, fseq
                        last_error = "unexpected cmd 0x%02X" % fcmd
                last_error = "timeout waiting for 0x%02X" % expect_cmd
            raise PowerLinkError(last_error or "no response")

    # ---- 对外命令 ----

    def send_heartbeat(self):
        """HEARTBEAT(0x01)，返回 (alarms, )。"""
        self._token = (self._token + 1) & 0xFFFF
        payload = struct.pack(">H", self._token)
        cmd, payload, _ = self._transact(CMD_HEARTBEAT, payload, CMD_HEARTBEAT_ACK)
        token, alarms = struct.unpack(">HB", payload[:3])
        return token, alarms

    def set_power(self, channel, enable):
        """SET_POWER(0x02)，返回状态码（0=OK）。"""
        payload = bytes((channel & 0xFF, 1 if enable else 0))
        cmd, payload, _ = self._transact(CMD_SET_POWER, payload, CMD_CMD_ACK)
        return payload[1] if len(payload) >= 2 else STATUS_BAD_FRAME

    def set_power_all(self, state_mask):
        """SET_POWER_ALL(0x05)，返回状态码（0=OK）。"""
        payload = struct.pack(">H", state_mask & 0xFFFF)
        cmd, payload, _ = self._transact(CMD_SET_POWER_ALL, payload, CMD_CMD_ACK)
        return payload[1] if len(payload) >= 2 else STATUS_BAD_FRAME

    def get_status(self):
        """GET_STATUS(0x03)，返回 STATUS 字典并缓存到 latest_status。"""
        cmd, payload, _ = self._transact(CMD_GET_STATUS, b"", CMD_STATUS)
        status = parse_status(payload)
        self.latest_status = status
        return status

    def set_report_interval(self, interval_ms):
        """SET_REPORT_INTERVAL(0x04)；0 = 关闭周期推送。返回状态码。"""
        payload = struct.pack(">H", interval_ms)
        cmd, payload, _ = self._transact(CMD_SET_REPORT_INTERVAL, payload, CMD_CMD_ACK)
        return payload[1] if len(payload) >= 2 else STATUS_BAD_FRAME
