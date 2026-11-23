#!/usr/bin/env python3
"""Probe a USB CDC-ACM CAN adapter for SLCAN (Lawicel) support.

Sends harmless ASCII query commands and prints any response. Only reads the
serial line; sends no CAN frames. Safe to run with a motor bus connected
(no power applied to motors is required for these queries).
"""
import sys
import time

import serial

PORT = "/dev/ttyACM0"

# SLCAN / Lawicel query commands (all read-only, ASCII terminated by CR)
QUERIES = [b"V\r", b"N\r", b"F\r", b"L\r"]

BAUDS = [115200, 9600, 57600, 230400]


def probe(port, baud):
    try:
        ser = serial.Serial(port, baud, timeout=0.4, write_timeout=0.5)
    except serial.SerialException as exc:
        return "open failed: %s" % exc

    responses = []
    for query in QUERIES:
        try:
            ser.reset_input_buffer()
            ser.write(query)
            time.sleep(0.15)
            data = ser.read(256)
        except serial.SerialException as exc:
            responses.append("%s -> error: %s" % (query, exc))
            break
        if data:
            responses.append("%s -> %r" % (query, data))
        else:
            responses.append("%s -> (no reply)" % query)
    ser.close()
    return " | ".join(responses)


def main():
    print("Port: %s" % PORT)
    for baud in BAUDS:
        result = probe(PORT, baud)
        print("  %7d baud: %s" % (baud, result))


if __name__ == "__main__":
    main()
