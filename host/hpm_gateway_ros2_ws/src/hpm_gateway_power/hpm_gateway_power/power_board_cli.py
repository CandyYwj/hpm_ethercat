#!/usr/bin/env python3
"""PowerLink v1 电源板命令行工具（无需 ROS，直接 pyserial）。

用于电源板 bring-up 与手动验证。用法示例：

    power_board_cli.py --port /dev/ttyUSB0 status
    power_board_cli.py --port /dev/ttyUSB0 on M1
    power_board_cli.py --port /dev/ttyUSB0 off all
    power_board_cli.py --port /dev/ttyUSB0 monitor

通道名支持：12V_1, 12V_2, 12V_FAN, 12V_4, 5V, 12V_3, M1..M4, all
注意 48V 通道号与协议文档初稿不同（以硬件/固件为准）：
    M4=ch6(PA8), M1=ch7(PB13), M2=ch8(PB14), M3=ch9(PB15)
"""

import argparse
import sys
import time

import serial

from hpm_gateway_power.power_link_protocol import (
    CHANNEL_BY_NAME,
    CHANNEL_NAMES,
    MOTOR_TO_CHANNEL,
    LOW_VOLTAGE_MASK,
    FORTY_EIGHT_V_MASK,
    STATUS_NAMES,
    STATUS_OK,
    PowerLinkClient,
    PowerLinkError,
)


def resolve_target(text):
    """把 '12V_1'/'M1'/'all'/数字 解析为 (channel 或 None, mask or None)。"""
    text = text.strip().upper()
    if text == "ALL":
        return None, None
    if text in CHANNEL_BY_NAME:
        return CHANNEL_BY_NAME[text], None
    if text in MOTOR_TO_CHANNEL:
        return MOTOR_TO_CHANNEL[text], None
    try:
        ch = int(text)
        if 0 <= ch <= 9:
            return ch, None
    except ValueError:
        pass
    raise SystemExit(
        f"unknown target '{text}'; use {sorted(CHANNEL_NAMES.values())} or M1..M4 or all")


def cmd_status(client, _):
    status = client.get_status()
    alarms = status["alarms"]
    print(f"VBAT={status['battery_mv']} mV, "
          f"NTC1={status['ntc1_deci_c']} deci-C, "
          f"NTC2={status['ntc2_deci_c']} deci-C")
    state = status["output_state"]
    parts = [f"{name}={'ON' if (state >> ch) & 1 else 'off'}"
             for ch, name in sorted(CHANNEL_NAMES.items())]
    print("STATE: " + ", ".join(parts))
    print(f"ALARMS=0x{alarms:02X} "
          f"{'(LOW_VOLTAGE)' if alarms & 1 else ''}"
          f"{'(COMM_LOST)' if alarms & 2 else ''}")


def cmd_on_off(client, args):
    channel, _ = resolve_target(args.target)
    if channel is None:
        # all
        mask = LOW_VOLTAGE_MASK | FORTY_EIGHT_V_MASK
        status = client.set_power_all(mask if args.on else 0)
    else:
        status = client.set_power(channel, args.on)
    name = "all" if channel is None else CHANNEL_NAMES.get(channel, f"ch{channel}")
    if status == STATUS_OK:
        print(f"{name} -> {'ON' if args.on else 'off'} OK")
    else:
        print(f"{name} -> {'ON' if args.on else 'off'} {STATUS_NAMES.get(status, status)}")
        return 1
    return 0


def cmd_mask(client, args):
    status = client.set_power_all(args.mask)
    print(f"state_mask=0x{args.mask:04X} -> {STATUS_NAMES.get(status, status)}")
    return 0 if status == STATUS_OK else 1


def cmd_interval(client, args):
    status = client.set_report_interval(args.ms)
    print(f"report_interval={args.ms} ms -> {STATUS_NAMES.get(status, status)}")
    return 0 if status == STATUS_OK else 1


def cmd_monitor(client, args):
    print("monitoring (Ctrl-C to stop)...")
    last = None
    try:
        while True:
            try:
                status = client.get_status()
                state = status["output_state"]
                line = (f"VBAT={status['battery_mv']:5d} mV  "
                        f"NTC1={status['ntc1_deci_c']:5d}  "
                        f"NTC2={status['ntc2_deci_c']:5d}  "
                        f"ST=0x{state:04X}  AL=0x{status['alarms']:02X}")
                if line != last:
                    print(time.strftime("%H:%M:%S ") + line, flush=True)
                    last = line
            except PowerLinkError as exc:
                print(f"link: {exc}", flush=True)
            time.sleep(args.period / 1000.0)
    except KeyboardInterrupt:
        return 0


def main(argv=None):
    parser = argparse.ArgumentParser(description="PowerLink v1 power board CLI")
    parser.add_argument("--port", default="/dev/ttyUSB0", help="serial port")
    parser.add_argument("--baud", type=int, default=115200)
    sub = parser.add_subparsers(dest="command", required=True)

    p_status = sub.add_parser("status", help="query and print STATUS")
    p_status.set_defaults(func=cmd_status)

    for on in (True, False):
        p = sub.add_parser("on" if on else "off",
                           help=f"{'enable' if on else 'disable'} a channel or all")
        p.add_argument("target", help="channel name/number/Mx/all")
        p.set_defaults(func=cmd_on_off, on=on)

    p_mask = sub.add_parser("mask", help="set all channels via state bitmap")
    p_mask.add_argument("mask", type=lambda x: int(x, 0), help="u16 bitmap (hex ok)")
    p_mask.set_defaults(func=cmd_mask)

    p_int = sub.add_parser("interval", help="set periodic report interval (0=off)")
    p_int.add_argument("ms", type=int, help="interval in ms")
    p_int.set_defaults(func=cmd_interval)

    p_mon = sub.add_parser("monitor", help="poll status periodically")
    p_mon.add_argument("--period", type=int, default=1000, help="poll period ms")
    p_mon.set_defaults(func=cmd_monitor)

    args = parser.parse_args(argv)

    ser = serial.Serial(port=args.port, baudrate=args.baud,
                        timeout=0.05, write_timeout=0.1)
    client = PowerLinkClient(ser)
    try:
        return args.func(client, args)
    finally:
        ser.close()


if __name__ == "__main__":
    sys.exit(main())
