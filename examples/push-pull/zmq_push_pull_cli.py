#! /usr/bin/env python3

"""
PUSH/PULL example (client)
"""

import zmq
import time
import curses
import argparse


# ncurses Python example: https://gist.github.com/claymcleod/b670285f334acd56ad1c


def draw(stdscr, name):

    # Clear and refresh the screen for a blank canvas
    stdscr.clear()
    stdscr.refresh()

    # Start colors in curses
    curses.start_color()
    curses.init_pair(1, curses.COLOR_CYAN, curses.COLOR_BLACK)
    curses.init_pair(2, curses.COLOR_RED, curses.COLOR_BLACK)

    curses.curs_set(False)
    stdscr.nodelay(True)

    # Prepare our context and sockets
    context = zmq.Context()

    # Connect to task ventilator
    sender = context.socket(zmq.PUSH)
    sender.connect("tcp://localhost:5555")

    # Connect to weather server
    receiver = context.socket(zmq.PULL)
    receiver.connect("tcp://localhost:5556")

    sender.send_string(f"name {name} hello")

    try:

        # Process messages from sockets and keyboard
        while True:

            key = stdscr.getch()

            # Process any waiting updates
            while True:
                try:
                    msg = receiver.recv(zmq.DONTWAIT)

                    stdscr.attron(curses.color_pair(2))
                    stdscr.addstr(2, 2, msg)
                    stdscr.attroff(curses.color_pair(2))

                except zmq.Again:
                    break

            if key != -1:
                stdscr.attron(curses.A_BOLD)
                stdscr.attron(curses.color_pair(1))
                stdscr.addstr(5, 2, f"key pressed: 0x{key:x}   ")
                stdscr.attroff(curses.color_pair(1))
                stdscr.attroff(curses.A_BOLD)

                sender.send_string(f"name {name} key {key}")

            # No activity, so sleep for 1 msec
            time.sleep(0.001)

    except KeyboardInterrupt:
        pass

    sender.send_string(f"name {name} bye")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--verbose", help="increase verbosity", action='store_true')
    parser.add_argument("name", help="subscriber name")
    args = parser.parse_args()

    curses.wrapper(draw, args.name)


if __name__ == "__main__":
    main()
