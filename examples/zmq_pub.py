#! /usr/bin/env python3

"""
PUB example
"""

import time
import zmq


def main():

    # Prepare our context and publisher
    context = zmq.Context()
    publisher = context.socket(zmq.PUB)
    publisher.bind("tcp://*:5556")

    while True:
        # Write two messages, each with an envelope and content
        publisher.send_multipart([b"A", b"only A can see that"])
        publisher.send_multipart([b"B", b"only B can see that"])
        time.sleep(1)

    # We never get here but clean up anyhow
    publisher.close()
    context.term()


if __name__ == "__main__":
    main()
