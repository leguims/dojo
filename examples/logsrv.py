#! /usr/bin/env python3

"""
PUSH/PULL logging server
"""

import zmq
import datetime


# SGR color constants
# rene-d 2018

class Colors:
    """ ANSI color codes """
    BLACK = "\033[0;30m"
    RED = "\033[0;31m"
    GREEN = "\033[0;32m"
    YELLOW = "\033[0;33m"
    BLUE = "\033[0;34m"
    MAGENTA = "\033[0;35m"
    CYAN = "\033[0;36m"
    WHITE = "\033[0;37m"
    LIGHT_BLACK = "\033[1;30m"
    LIGHT_RED = "\033[1;31m"
    LIGHT_GREEN = "\033[1;32m"
    LIGHT_YELLOW = "\033[1;33m"
    LIGHT_BLUE = "\033[1;34m"
    LIGHT_MAGENTA = "\033[1;35m"
    LIGHT_CYAN = "\033[1;36m"
    LIGHT_WHITE = "\033[1;37m"
    BOLD = "\033[1m"
    FAINT = "\033[2m"
    ITALIC = "\033[3m"
    UNDERLINE = "\033[4m"
    BLINK = "\033[5m"
    # RAPID_BLINK = "\033[6m"
    NEGATIVE = "\033[7m"
    # CONCEAL = "\033[8m"
    CROSSED = "\033[9m"
    END = "\033[0m"
    # cancel SGR codes if we don't write to a terminal
    if not __import__("sys").stdout.isatty():
        for _ in dir():
            if isinstance(_, str) and _[0] != "_":
                locals()[_] = ""
    else:
        # set Windows console in VT mode
        if __import__("platform").system() == "Windows":
            kernel32 = __import__("ctypes").windll.kernel32
            kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)
            del kernel32


colors = {"DEBUG": Colors.LIGHT_BLACK,
          "WARNING": Colors.YELLOW,
          "INFO": Colors.GREEN,
          "ERROR": Colors.RED,
          "CRITICAL": Colors.MAGENTA}

context = zmq.Context()
zmqlog = context.socket(zmq.PULL)
zmqlog.bind("ipc:///tmp/pong_logger")

try:
    while True:
        msg = zmqlog.recv_multipart()
        topic, text = map(bytes.decode, msg)
        topic, _, level = topic.rpartition(".")
        color = colors.get(level, Colors.BLUE)
        d = datetime.datetime.now().isoformat()
        print("{}{} {:8}-{:5}-{}{}".format(color, d, level, topic, text.rstrip(), Colors.END))
except KeyboardInterrupt:
    pass
