#!/usr/bin/env python3
"""Record and render the firmware's 1 Hz MT/MD motor telemetry stream."""

import argparse
import json
from pathlib import Path
import sys
import time
from typing import Callable, Iterable, List, Optional


MOTOR_IDS = range(1, 17)
METRICS = ("position_rad", "velocity_rad_s", "torque_nm", "temperature_c")
METRIC_LABELS = {
    "position_rad": ("Position", "rad"),
    "velocity_rad_s": ("Velocity", "rad/s"),
    "torque_nm": ("Torque", "Nm"),
    "temperature_c": ("Temperature", "C"),
}


def parse_line(line: str, snapshot: Optional[dict],
               on_snapshot: Optional[Callable[[dict], None]] = None) -> Optional[dict]:
    """Consume one line and return the in-progress snapshot, if any."""
    fields = line.strip().split(",")
    try:
        if fields[:2] == ["MT", "1"] and len(fields) == 4:
            return {"version": 1, "uptime_ms": int(fields[2]),
                    "command_sequence": int(fields[3]), "motors": {}}
        if fields[:2] != ["MD", "1"] or len(fields) != 10 or snapshot is None:
            return snapshot
        motor_id = int(fields[2])
        if motor_id not in MOTOR_IDS:
            return snapshot
        snapshot["motors"][str(motor_id)] = {
            "online": bool(int(fields[3])),
            "enabled": bool(int(fields[4])),
            "fault": int(fields[5], 0),
            "position_rad": float(fields[6]),
            "velocity_rad_s": float(fields[7]),
            "torque_nm": float(fields[8]),
            "temperature_c": float(fields[9]),
        }
    except ValueError:
        return snapshot
    if len(snapshot["motors"]) == len(MOTOR_IDS):
        if on_snapshot is not None:
            on_snapshot(snapshot)
        return None
    return snapshot


def lines_from_input(port: Optional[str], baud: int) -> Iterable[str]:
    if port:
        try:
            import serial
        except ImportError as exc:
            raise SystemExit("pyserial is required for --port") from exc
        with serial.Serial(port, baud, timeout=1.0) as stream:
            while True:
                yield stream.readline().decode("ascii", errors="replace")
    else:
        yield from sys.stdin


def record_stream(port: Optional[str], baud: int, output: Path,
                  duration_seconds: Optional[float]) -> int:
    """Record one JSON object per complete 16-axis snapshot."""
    output.parent.mkdir(parents=True, exist_ok=True)
    output.unlink(missing_ok=True)
    count = 0
    started = time.monotonic()

    def write_snapshot(snapshot: dict) -> None:
        nonlocal count
        with output.open("a", encoding="utf-8") as stream:
            stream.write(json.dumps(snapshot, sort_keys=True) + "\n")
        count += 1
        print(f"recorded snapshot {count}: uptime={snapshot['uptime_ms']} ms",
              flush=True)

    try:
        snapshot = None
        for line in lines_from_input(port, baud):
            snapshot = parse_line(line, snapshot, write_snapshot)
            if (duration_seconds is not None
                    and time.monotonic() - started >= duration_seconds):
                break
    except KeyboardInterrupt:
        print("recording stopped", file=sys.stderr)
    return count


def load_snapshots(input_path: Path) -> List[dict]:
    snapshots = []
    with input_path.open("r", encoding="utf-8") as stream:
        for line_number, line in enumerate(stream, 1):
            try:
                snapshot = json.loads(line)
                if (snapshot.get("version") == 1
                        and all(str(motor_id) in snapshot.get("motors", {})
                                for motor_id in MOTOR_IDS)):
                    snapshots.append(snapshot)
            except json.JSONDecodeError:
                print(f"warning: skipping invalid JSON line {line_number}",
                      file=sys.stderr)
    return snapshots


def render_snapshots(snapshots: List[dict], output_dir: Path) -> None:
    """Render one PNG per motor, with four metric subplots per PNG."""
    try:
        import matplotlib.pyplot as plt
    except ImportError as exc:
        raise SystemExit("matplotlib is required for render") from exc
    if not snapshots:
        raise SystemExit("record file contains no complete snapshots")
    output_dir.mkdir(parents=True, exist_ok=True)
    times = [snapshot["uptime_ms"] / 1000.0 for snapshot in snapshots]
    for motor_id in MOTOR_IDS:
        figure, axes = plt.subplots(4, 1, figsize=(12, 10), sharex=True)
        figure.suptitle(f"Motor {motor_id} telemetry")
        for axis, metric in zip(axes, METRICS):
            title, unit = METRIC_LABELS[metric]
            values = [snapshot["motors"][str(motor_id)][metric]
                      for snapshot in snapshots]
            axis.plot(times, values, linewidth=1.0)
            axis.set_ylabel(unit)
            axis.set_title(title, loc="left", fontsize=10)
            axis.grid(True, alpha=0.25)
        axes[-1].set_xlabel("time (s)")
        figure.tight_layout(rect=(0, 0, 1, 0.97))
        figure.savefig(output_dir / f"motor_{motor_id:02d}.png", dpi=150)
        plt.close(figure)
    print(f"rendered {len(snapshots)} snapshots for 16 motors to {output_dir}")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Record MT/MD telemetry, then render it offline")
    commands = parser.add_subparsers(dest="command", required=True)

    record = commands.add_parser("record", help="capture serial data to JSONL")
    record.add_argument("--port", required=True, help="UART device, e.g. /dev/ttyUSB0")
    record.add_argument("--baud", type=int, default=115200)
    record.add_argument("--output", type=Path, required=True,
                        help="JSONL output file")
    record.add_argument("--duration-seconds", type=float,
                        help="stop automatically after this duration")

    render = commands.add_parser("render", help="render JSONL to one PNG per motor")
    render.add_argument("--input", type=Path, required=True,
                        help="JSONL recording from record")
    render.add_argument("--output-dir", type=Path, required=True,
                        help="directory for generated motor PNG files")

    args = parser.parse_args()
    if args.command == "record":
        if args.duration_seconds is not None and args.duration_seconds <= 0.0:
            parser.error("--duration-seconds must be positive")
        record_stream(args.port, args.baud, args.output, args.duration_seconds)
    else:
        render_snapshots(load_snapshots(args.input), args.output_dir)


if __name__ == "__main__":
    main()
