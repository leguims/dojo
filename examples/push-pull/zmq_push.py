#! /usr/bin/env python3

"""
PUSH example (aka. ventilator)
"""

import zmq
import time


def main():
    # Prepare our context and sockets
    context = zmq.Context()

    sender = context.socket(zmq.PUSH)
    sender.bind("tcp://*:5556")

    n = 0
    while True:
        n += 1
        sender.send_string(f"message {n}")
        time.sleep(1)


if __name__ == "__main__":
    main()
