#! /usr/bin/env python3

"""
PUSH/PULL log producer
"""

import zmq
import argparse
import logging
import zmq.log.handlers


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--verbose", help="increase verbosity", action='store_true')
    parser.add_argument("-t", "--topic", help="topic", default="")
    parser.add_argument("-l", "--level", help="level", default='debug',
                        choices=['debug', 'info', 'warning', 'error', 'critical'])
    parser.add_argument("text", help="text", nargs='+')
    args = parser.parse_args()

    # Prepare our context and sockets
    context = zmq.Context()

    # logging with ZeroMQ
    zmqlog = context.socket(zmq.PUSH)
    zmqlog.connect("ipc:///tmp/dojo_logger")

    handler = zmq.log.handlers.PUBHandler(zmqlog)

    # replace PUBHandler formatters
    handler.formatters[logging.DEBUG] = logging.Formatter("%(message)s\n")
    handler.formatters[logging.INFO] = logging.Formatter("%(message)s\n")
    handler.formatters[logging.WARN] = logging.Formatter("%(message)s\n")
    handler.formatters[logging.ERROR] = logging.Formatter("%(message)s\n")
    handler.formatters[logging.CRITICAL] = logging.Formatter("%(message)s\n")

    handler.root_topic = args.topic
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    logger.addHandler(handler)

    # send the log
    getattr(logging, args.level)(" ".join(args.text))


if __name__ == "__main__":
    main()
