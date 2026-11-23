#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Author: BearKing
#
# topic_monitor.py - Read-only ROS2 topic observer.
#
# 目标：不改任何现有代码/消息/PDO，只订阅现有 ROS2 话题，统计每个话题的
# 频率（Hz）与内容摘要，周期写入 CSV 日志。用于排查“命令/反馈/手柄频率归零、
# 电机扭矩全零”等失力现象——一个话题频率骤降或扭矩全零，日志里立刻可见。
#
# 特性：
#   - 自动通过话题图解析消息类型并动态订阅（可监听任意话题，复用性强）
#   - 话题 QoS 用 name@best_effort / name@reliable 指定（feedback/imu 是 best_effort）
#   - 周期落盘 CSV：time, <topic>_hz, <topic>_last
#   - 话题从“有数据”变为“频率归零”时写入一行 !DROPPED 标记
#
# 用法（RDK，需先 source 工作区）:
#   python3 topic_monitor.py \
#     --topics /hpm_gateway/motor_feedback@best_effort /imu/data@best_effort \
#              /hpm_gateway/motor_commands /hpm_gateway/motor_command /gamepad/joy \
#     --log /var/log/hpm_gateway/topic_monitor.csv --interval 1
#
# 只读观测：不发布任何数据，不改变被观测节点行为。
import argparse
import importlib
import os
import time

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy


def resolve_type(type_str):
    """把 'pkg/msg/Type' 解析为可订阅的消息类。"""
    pkg = type_str.split('/')[0]
    msg_name = type_str.split('/')[-1]
    module = importlib.import_module('%s.msg' % pkg)
    return getattr(module, msg_name)


def summarize(topic, msg):
    """对已知话题按 topic 名提取关键内容摘要；未知话题退化为截断 repr。"""
    try:
        if topic == '/hpm_gateway/motor_feedback':
            pos = ' '.join('%.3f' % f.position_rad for f in msg.feedback)
            vel = ' '.join('%.3f' % f.velocity_rad_s for f in msg.feedback)
            tor = ' '.join('%.3f' % f.torque_nm for f in msg.feedback)
            return 'pos[%s] vel[%s] t[%s]' % (pos, vel, tor)
        if topic == '/imu/data':
            a, w = msg.linear_acceleration, msg.angular_velocity
            return 'acc=(%.2f,%.2f,%.2f) gyro=(%.2f,%.2f,%.2f)' % (
                a.x, a.y, a.z, w.x, w.y, w.z)
        if topic == '/gamepad/joy':
            return 'axes=%s' % [round(float(x), 2) for x in msg.axes]
        if topic == '/hpm_gateway/motor_command':
            return 'id=%d p=%.3f v=%.3f t=%.3f kp=%.2f kd=%.2f' % (
                msg.motor_id, msg.position_rad, msg.velocity_rad_s,
                msg.torque_nm, msg.kp, msg.kd)
        if topic == '/hpm_gateway/motor_commands':
            return 'valid=0x%08X seq=%d' % (msg.valid_mask, msg.sequence)
    except (AttributeError, TypeError):
        pass
    s = repr(msg)
    return s if len(s) <= 200 else s[:197] + '...'


class TopicMonitor(Node):
    def __init__(self, topics, log_path, interval):
        super().__init__('topic_monitor')
        parent = os.path.dirname(log_path)
        if parent:
            os.makedirs(parent, exist_ok=True)
        self.log_path = log_path
        self.interval = interval
        # name -> {count, window_start, last_summary, seen}
        self.state = {}
        self.columns = ['time']
        self._watched = []

        # rclpy 返回 [(name, [types]), ...]，转 dict 便于按名字查类型
        available = dict(self.get_topic_names_and_types())
        for spec in topics:
            name, _, qos = spec.partition('@')
            name = name.strip()
            types = available.get(name)
            if not types:
                self.get_logger().warn(
                    'topic %s has no publisher right now; skipped' % name)
                continue
            msg_cls = resolve_type(types[0])
            rel = (ReliabilityPolicy.BEST_EFFORT
                   if qos.strip() == 'best_effort'
                   else ReliabilityPolicy.RELIABLE)
            profile = QoSProfile(depth=10, reliability=rel)
            self.state[name] = {
                'count': 0, 'window_start': time.monotonic(),
                'last_summary': 'n/a', 'seen': False}
            self.columns.append(name + '_hz')
            self.columns.append(name + '_last')
            self._watched.append(
                self.create_subscription(msg_cls, name,
                                         self.make_cb(name), profile))
            self.get_logger().info(
                'subscribed %s (%s, %s)' %
                (name, types[0], 'best_effort' if rel ==
                 ReliabilityPolicy.BEST_EFFORT else 'reliable'))

        if not self._watched:
            self.get_logger().error('no topics subscribed; exit')
            raise SystemExit(1)

        self.create_timer(interval, self.flush)
        self.get_logger().info('monitoring %d topics -> %s every %ds' %
                               (len(self._watched), log_path, interval))

    def make_cb(self, name):
        def cb(msg):
            st = self.state[name]
            st['count'] += 1
            st['last_summary'] = summarize(name, msg)
            st['seen'] = True
        return cb

    def flush(self):
        now = time.time()
        row = [time.strftime('%Y-%m-%d %H:%M:%S')]
        events = []
        for name, st in self.state.items():
            elapsed = time.monotonic() - st['window_start']
            hz = st['count'] / elapsed if elapsed > 0 else 0.0
            row.append('%.1f' % hz)
            row.append(st['last_summary'])
            # 频率从“有数据”骤降到 0 -> 打标记
            if st['seen'] and hz < 0.01 and st['last_summary'] != 'n/a':
                events.append('!DROPPED %s' % name)
            st['count'] = 0
            st['window_start'] = time.monotonic()

        with open(self.log_path, 'a') as f:
            if f.tell() == 0:
                f.write(','.join(self.columns) + '\n')
            f.write(','.join(row) + '\n')
            for ev in events:
                f.write('!%s %s\n' % (time.strftime('%Y-%m-%d %H:%M:%S'), ev))
            # 强制落盘：突然断电（如 RDK 断电/电源故障）时，尽量少丢最近数据
            f.flush()
            os.fsync(f.fileno())


def main():
    ap = argparse.ArgumentParser(description='Read-only ROS2 topic observer')
    ap.add_argument('--topics', nargs='+', required=True,
                    help='topics, each as name[@best_effort|@reliable]')
    ap.add_argument('--log', default='/var/log/hpm_gateway/topic_monitor.csv',
                    help='output CSV path')
    ap.add_argument('--interval', type=float, default=1.0,
                    help='flush period in seconds')
    args = ap.parse_args()

    rclpy.init()
    try:
        node = TopicMonitor(args.topics, args.log, args.interval)
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        rclpy.shutdown()


if __name__ == '__main__':
    main()
