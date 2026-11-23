#!/usr/bin/env python3

import argparse

import rclpy
from hpm_gateway_msgs.msg import MotorMitCommandArray


ALL_MOTOR_IDS = tuple(range(1, 17))


def build_command(node, active, sequence, motor_ids, mode,
                  position_rad, velocity_rad_s, kp, kd):
    """Build a full 16-axis command frame in position or velocity mode.

    All 16 axes get explicit values. Selected motor_ids receive the active
    command; the rest stay at zero. When inactive the valid_mask is cleared
    so the firmware releases the axes (its 100 ms command timeout then sends
    the protocol stop/brake).
    """
    message = MotorMitCommandArray()
    message.stamp = node.get_clock().now().to_msg()
    message.sequence = sequence
    message.valid_mask = (
        sum(1 << (motor_id - 1) for motor_id in motor_ids)
        if active else 0
    )

    for motor_id in range(1, 17):
        command = message.commands[motor_id - 1]
        command.stamp = message.stamp
        command.motor_id = motor_id
        command.position_rad = 0.0
        command.velocity_rad_s = 0.0
        command.kp = 0.0
        command.kd = 0.0
        command.torque_nm = 0.0

    if active:
        for motor_id in motor_ids:
            command = message.commands[motor_id - 1]
            if mode == "velocity":
                # MIT velocity mode: kp=0, v_des set, kd is the velocity gain.
                command.velocity_rad_s = velocity_rad_s
                command.kd = kd
            else:  # position
                command.position_rad = position_rad
                command.kp = kp
                command.kd = kd

    return message


def parse_motor_ids(text):
    parts = [part.strip() for part in text.split(",") if part.strip()]
    motor_ids = [int(part) for part in parts]
    for motor_id in motor_ids:
        if motor_id < 1 or motor_id > 16:
            raise SystemExit("--motor-ids must contain values in 1..16")
    return tuple(sorted(set(motor_ids)))


def main():
    parser = argparse.ArgumentParser(
        description="Publish position or velocity reference commands for all axes.")
    parser.add_argument(
        "--topic",
        default="/hpm_gateway/motor_commands",
        help="MotorMitCommandArray topic.")
    parser.add_argument(
        "--rate-hz",
        type=float,
        default=200.0,
        help="Command publish rate.")
    parser.add_argument(
        "--duration-sec",
        type=float,
        default=2.0,
        help="Active command duration.")
    parser.add_argument(
        "--motor-ids",
        default=",".join(str(motor_id) for motor_id in ALL_MOTOR_IDS),
        help="Comma-separated motor IDs to command (1..16).")
    parser.add_argument(
        "--mode",
        choices=("position", "velocity"),
        default="position",
        help="Control mode: position (p_des + kp/kd) or velocity (v_des + kd).")
    parser.add_argument(
        "--position-rad",
        type=float,
        default=0.0,
        help="Target position in radians for position mode.")
    parser.add_argument(
        "--velocity-rad-s",
        type=float,
        default=0.3,
        help="Target velocity in rad/s for velocity mode (negative reverses).")
    parser.add_argument(
        "--kp",
        type=float,
        default=1.0,
        help="Position stiffness in Nm/rad.")
    parser.add_argument(
        "--kd",
        type=float,
        default=1.0,
        help="Damping in Nm/(rad/s).")
    args = parser.parse_args()

    if args.rate_hz <= 0.0:
        raise SystemExit("--rate-hz must be positive")
    if args.duration_sec < 0.0:
        raise SystemExit("--duration-sec must be non-negative")
    if args.kp < 0.0 or args.kd < 0.0:
        raise SystemExit("--kp and --kd must be non-negative")

    motor_ids = parse_motor_ids(args.motor_ids)

    rclpy.init()
    node = rclpy.create_node("hpm_gateway_reference_command_sender")
    publisher = node.create_publisher(MotorMitCommandArray, args.topic, 10)
    period_sec = 1.0 / args.rate_hz
    end_ns = node.get_clock().now().nanoseconds + int(args.duration_sec * 1e9)
    sequence = 1

    if args.mode == "velocity":
        node.get_logger().warn(
            f"Velocity control: motors {motor_ids} at {args.velocity_rad_s:.3f} "
            f"rad/s with kd={args.kd:.1f} for {args.duration_sec:.1f} s. "
            "Ensure mechanical restraint and safe limits.")
    else:
        node.get_logger().warn(
            f"Position control: motors {motor_ids} will actively hold "
            f"{args.position_rad:.3f} rad with kp={args.kp:.1f} kd={args.kd:.1f}. "
            "Ensure mechanical restraint and safe limits.")

    try:
        while rclpy.ok() and node.get_clock().now().nanoseconds < end_ns:
            publisher.publish(build_command(node, True, sequence, motor_ids,
                                            args.mode, args.position_rad,
                                            args.velocity_rad_s, args.kp, args.kd))
            sequence += 1
            rclpy.spin_once(node, timeout_sec=period_sec)
    finally:
        for _ in range(10):
            publisher.publish(build_command(node, False, sequence, motor_ids,
                                            args.mode, args.position_rad,
                                            args.velocity_rad_s, args.kp, args.kd))
            sequence += 1
            rclpy.spin_once(node, timeout_sec=0.02)
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
