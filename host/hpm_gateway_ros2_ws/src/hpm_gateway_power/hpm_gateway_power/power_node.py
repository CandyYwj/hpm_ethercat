#!/usr/bin/env python3
"""hpm_gateway 电源控制节点。

通过 USB-TTL / USART1 使用 PowerLink v1 协议控制 DOG 电源板（STM32F103C8T6）：
- 周期心跳（默认 500 ms），维持链路并清除板卡 COMM_LOST 告警；
- 周期 GET_STATUS 轮询（默认 1000 ms），发布 `PowerStatus` 遥测；
- 订阅 `PowerCommand`，执行单通道 SET_POWER 或批量 SET_POWER_ALL；
- 主机侧链路看门狗：>3 s 无合法帧则置 comm_lost。

依赖：rclpy、hpm_gateway_msgs、pyserial。
"""

import serial
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy

from hpm_gateway_msgs.msg import PowerCommand, PowerStatus
from hpm_gateway_power.power_link_protocol import (
    CHANNEL_NAMES,
    CHANNEL_ALL,
    STATUS_NAMES,
    STATUS_OK,
    PowerLinkClient,
    PowerLinkError,
)

# 与固件一致：超过该时长未收到合法帧，判定链路中断。
WATCHDOG_COMM_LOST_MS = 3000


class PowerNode(Node):
    def __init__(self):
        super().__init__("hpm_gateway_power_node")

        self.declare_parameter("serial_port", "/dev/ttyUSB0")
        self.declare_parameter("baud_rate", 115200)
        self.declare_parameter("heartbeat_period_ms", 500)
        self.declare_parameter("status_period_ms", 1000)
        self.declare_parameter("cmd_timeout_ms", 100)
        self.declare_parameter("cmd_retries", 2)
        self.declare_parameter("report_interval_ms", 0)
        self.declare_parameter("status_topic", "/hpm_gateway/power_status")
        self.declare_parameter("command_topic", "/hpm_gateway/power_command")

        serial_port = self.get_parameter("serial_port").value
        baud_rate = self.get_parameter("baud_rate").value
        heartbeat_ms = self.get_parameter("heartbeat_period_ms").value
        status_ms = self.get_parameter("status_period_ms").value
        self._cmd_timeout_s = self.get_parameter("cmd_timeout_ms").value / 1000.0
        self._cmd_retries = int(self.get_parameter("cmd_retries").value)
        report_ms = int(self.get_parameter("report_interval_ms").value)

        self._client = None
        self._last_rx_monotonic = None
        self._last_ack_cmd = 0
        self._last_ack_status = STATUS_OK
        self._last_ack_ok = True
        self._last_error = ""

        # 话题
        reliable = QoSProfile(
            depth=10,
            reliability=ReliabilityPolicy.RELIABLE,
            history=HistoryPolicy.KEEP_LAST,
        )
        self._status_pub = self.create_publisher(
            PowerStatus, self.get_parameter("status_topic").value, reliable)
        self._cmd_sub = self.create_subscription(
            PowerCommand, self.get_parameter("command_topic").value,
            self._on_command, reliable)

        # 打开串口并初始化协议
        try:
            ser = serial.Serial(port=serial_port, baudrate=baud_rate,
                                timeout=0.05, write_timeout=0.1)
        except serial.SerialException as exc:
            self.get_logger().fatal(f"cannot open {serial_port}: {exc}")
            raise
        self._client = PowerLinkClient(ser)
        self.get_logger().info(
            f"power link open: {serial_port} @ {baud_rate}")

        # 关闭板卡周期推送，改由主机统一轮询（失败不致命，解析器仍可处理推送帧）
        if report_ms == 0:
            try:
                status = self._client.set_report_interval(0)
                if status != STATUS_OK:
                    self.get_logger().warn(
                        f"SET_REPORT_INTERVAL(0) -> {STATUS_NAMES.get(status, status)}")
            except PowerLinkError as exc:
                self.get_logger().warn(f"SET_REPORT_INTERVAL failed: {exc}")

        # 定时器
        self.create_timer(heartbeat_ms / 1000.0, self._on_heartbeat_timer)
        self.create_timer(status_ms / 1000.0, self._on_status_timer)

    # ---- 链路看门狗 ----

    def _note_rx(self):
        self._last_rx_monotonic = self.get_clock().now().nanoseconds

    @property
    def _comm_lost(self):
        if self._last_rx_monotonic is None:
            return True
        age_ms = (self.get_clock().now().nanoseconds - self._last_rx_monotonic) / 1e6
        return age_ms > WATCHDOG_COMM_LOST_MS

    # ---- 周期任务 ----

    def _on_heartbeat_timer(self):
        if self._client is None:
            return
        try:
            self._client.send_heartbeat()
            self._note_rx()
        except PowerLinkError as exc:
            self._last_error = f"heartbeat: {exc}"
            self.get_logger().warning(self._last_error)

    def _on_status_timer(self):
        if self._client is None:
            return
        try:
            status = self._client.get_status()
            self._note_rx()
            self._publish_status()
        except PowerLinkError as exc:
            self._last_error = f"get_status: {exc}"
            self.get_logger().warning(self._last_error)

    # ---- 命令订阅 ----

    def _on_command(self, msg: PowerCommand):
        if self._client is None:
            self.get_logger().warn("command ignored: serial not open")
            return
        try:
            if msg.channel == CHANNEL_ALL:
                self._last_ack_cmd = 0x05
                self._last_ack_status = self._client.set_power_all(msg.state_mask)
            else:
                self._last_ack_cmd = 0x02
                self._last_ack_status = self._client.set_power(
                    msg.channel, msg.enable)
            self._last_ack_ok = (self._last_ack_status == STATUS_OK)
            self._note_rx()
            name = CHANNEL_NAMES.get(msg.channel, f"ch{msg.channel}")
            action = "on" if msg.enable else "off"
            self.get_logger().info(
                f"set {name} {action} -> "
                f"{STATUS_NAMES.get(self._last_ack_status, self._last_ack_status)}")
        except PowerLinkError as exc:
            self._last_ack_ok = False
            self._last_error = f"command: {exc}"
            self.get_logger().error(self._last_error)
        self._publish_status()

    # ---- 发布 ----

    def _publish_status(self):
        status = PowerStatus()
        status.stamp = self.get_clock().now().to_msg()
        status.comm_lost = self._comm_lost
        status.last_ack_cmd = self._last_ack_cmd
        status.last_ack_status = self._last_ack_status
        status.last_ack_ok = self._last_ack_ok
        if self._client is not None and self._client.latest_status is not None:
            s = self._client.latest_status
            status.battery_mv = s["battery_mv"]
            status.ntc1_deci_c = s["ntc1_deci_c"]
            status.ntc2_deci_c = s["ntc2_deci_c"]
            status.output_state = s["output_state"]
            status.alarms = s["alarms"]
        else:
            status.battery_mv = 0
            status.ntc1_deci_c = 0
            status.ntc2_deci_c = 0
            status.output_state = 0
            status.alarms = 0
        self._status_pub.publish(status)


def main(args=None):
    rclpy.init(args=args)
    node = PowerNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        if node._client is not None:
            try:
                node._client._serial.close()
            except Exception:
                pass
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == "__main__":
    main()
