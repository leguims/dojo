#! /usr/bin/env python3

"""
PULL example (ventilator)
"""

import zmq


def main():
    # Prepare our context and sockets
    context = zmq.Context()

    sender = context.socket(zmq.PULL)
    sender.connect("tcp://localhost:5556")

    while True:
        msg = sender.recv_string()
        print(msg)


if __name__ == "__main__":
    main()
