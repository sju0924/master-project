#!/usr/bin/env python3
"""Capture an STM32 UART stream to both the terminal and a log file."""

import argparse
import sys
import time
from pathlib import Path


def parse_args():
    parser = argparse.ArgumentParser(
        description="Capture UART output until the result line or Ctrl+C."
    )
    parser.add_argument("port", help="Serial port, for example /dev/ttyUSB0")
    parser.add_argument("output", type=Path, help="Log file to create")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument(
        "--timeout",
        type=float,
        default=0.2,
        help="Serial read timeout in seconds (default: 0.2)",
    )
    parser.add_argument(
        "--stop-on",
        default="Result:",
        help="Stop after a line containing this text; use an empty value to disable",
    )
    parser.add_argument(
        "--reconnect-timeout",
        type=float,
        default=15.0,
        help="Seconds to retry after a transient serial disconnect (default: 15)",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)

    try:
        import serial
    except ImportError:
        print(
            "pyserial is required. Install it with: python3 -m pip install pyserial",
            file=sys.stderr,
        )
        return 2

    def open_uart():
        return serial.Serial(
            port=args.port,
            baudrate=args.baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=args.timeout,
        )

    try:
        uart = open_uart()
    except serial.SerialException as exc:
        print(f"Cannot open {args.port}: {exc}", file=sys.stderr)
        return 1

    print(
        f"Capturing {args.port} at {args.baud} baud to {args.output}. "
        "Reset the board now; press Ctrl+C to stop.",
        file=sys.stderr,
    )

    pending = ""
    try:
        with args.output.open("w", encoding="utf-8", newline="") as log:
            while True:
                try:
                    data = uart.read(uart.in_waiting or 1)
                except serial.SerialException as exc:
                    print(
                        f"\nSerial disconnected: {exc}; reconnecting...",
                        file=sys.stderr,
                    )
                    try:
                        uart.close()
                    except serial.SerialException:
                        pass

                    deadline = time.monotonic() + args.reconnect_timeout
                    while True:
                        try:
                            uart = open_uart()
                            print(f"Reconnected to {args.port}.", file=sys.stderr)
                            break
                        except serial.SerialException:
                            if time.monotonic() >= deadline:
                                print(
                                    f"Reconnect timed out after "
                                    f"{args.reconnect_timeout:g}s.",
                                    file=sys.stderr,
                                )
                                return 1
                            time.sleep(0.25)
                    continue

                if not data:
                    continue

                text = data.decode("utf-8", errors="replace")
                sys.stdout.write(text)
                sys.stdout.flush()
                log.write(text)
                log.flush()

                pending += text
                lines = pending.splitlines(keepends=True)
                if lines and not lines[-1].endswith(("\n", "\r")):
                    pending = lines.pop()
                else:
                    pending = ""

                if args.stop_on and any(args.stop_on in line for line in lines):
                    # Allow the final separator already buffered by the UART to arrive.
                    time.sleep(0.05)
                    tail = uart.read(uart.in_waiting)
                    if tail:
                        tail_text = tail.decode("utf-8", errors="replace")
                        sys.stdout.write(tail_text)
                        sys.stdout.flush()
                        log.write(tail_text)
                    uart.close()
                    return 0
    except KeyboardInterrupt:
        uart.close()
        print("\nCapture stopped.", file=sys.stderr)
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
