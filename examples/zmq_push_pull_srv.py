#! /usr/bin/env python3

"""
PUSH/PULL example (server)
"""

import time
import zmq


def main():

    # Prepare our context and publisher
    context = zmq.Context()
    publisher = context.socket(zmq.PUSH)
    publisher.bind("tcp://*:5556")

    # Socket to receive messages
    receiver = context.socket(zmq.PULL)
    receiver.bind("tcp://*:5555")

    # Initialize poll set
    poller = zmq.Poller()
    # poller.register(publisher, zmq.POLLIN)
    poller.register(receiver, zmq.POLLIN)

    while True:
        try:
            socks = dict(poller.poll(10))
        except KeyboardInterrupt:
            break

        if receiver in socks:
            message = receiver.recv()
            # process update
            print("got:", message)

        # Write a message
        publisher.send_string(str(time.time()))

    # We never get here but clean up anyhow
    publisher.close()
    receiver.close()
    context.term()


if __name__ == "__main__":
    main()
