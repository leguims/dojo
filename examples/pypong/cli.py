#! /usr/bin/env python3

"""
PONG game client
"""

import zmq
import time
import curses
import argparse
import logging
import zmq.log.handlers


# ncurses Python example: https://gist.github.com/claymcleod/b670285f334acd56ad1c


def play(stdscr, name, game_server):

    # Prepare our context and sockets
    context = zmq.Context()

    # logging with ZeroMQ
    zmqlog = context.socket(zmq.PUSH)
    zmqlog.connect("ipc:///tmp/pong_logger")
    zmqlog.linger = 100     # https://github.com/zeromq/pyzmq/issues/153

    handler = zmq.log.handlers.PUBHandler(zmqlog)
    handler.root_topic = name
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    logger.addHandler(handler)

    logging.info("Starting pong client")

    # Connect to command transmitter
    sender = context.socket(zmq.PUSH)
    sender.connect(f"tcp://{game_server}:5555")

    # Connect to game server
    receiver = context.socket(zmq.SUB)
    receiver.connect(f"tcp://{game_server}:5556")
    receiver.setsockopt(zmq.SUBSCRIBE, 'game'.encode())

    # Clear and refresh the screen for a blank canvas
    stdscr.clear()
    stdscr.refresh()

    # Start colors in curses
    curses.start_color()
    curses.init_pair(1, curses.COLOR_CYAN, curses.COLOR_BLACK)
    curses.init_pair(2, curses.COLOR_RED, curses.COLOR_BLACK)

    curses.curs_set(False)
    stdscr.nodelay(True)

    height, width = stdscr.getmaxyx()

    # ok, let's go
    sender.send_string(f"{name} hello")

    logging.info("playing with server %s", game_server)

    try:
        stdscr.erase()
        stdscr.hline(int(height / 2 + -40 * height / 90) - 1, 2, curses.ACS_HLINE, width - 3)
        stdscr.hline(int(height / 2 + 40 * height / 90) + 1, 2, curses.ACS_HLINE, width - 3)

        n = 0
        ball_x, ball_y = 0, 0

        # Process messages from sockets and keyboard
        while True:

            key = stdscr.getch()

            # Process any waiting updates
            while True:
                try:
                    msg = receiver.recv_string(zmq.DONTWAIT)

                    stdscr.attron(curses.color_pair(2))
                    stdscr.addstr(0, 0, msg + "   ")
                    stdscr.attroff(curses.color_pair(2))

                    data = msg.split()

                    if data[0] == "play":

                        new_x, new_y = map(int, data[2].split(','))

                        # clear previous ball
                        stdscr.addstr(ball_y, ball_x, " ")

                        # draw the ball at the received position
                        ball_y = int(height / 2 - new_y * height / 90)
                        ball_x = int(width / 2 + new_x * width / 90)
                        stdscr.addch(ball_y, ball_x, "o")

                        # display the paddles
                        x = 6
                        for p in data[3:]:
                            na, pos = p.split(':')
                            pos = int(pos)

                            y1 = int(height / 2 - height * (pos + 10) / 90)
                            y2 = int(height / 2 - height * (pos - 10) / 90)
                            for y in range(1, height - 1):
                                stdscr.addch(y, x, '|' if y1 <= y <= y2 else ' ')

                            x = width - 6 - 1

                except zmq.Again:
                    break

            if key != -1:

                if key == curses.KEY_UP:
                    sender.send_string(f"{name} up")
                elif key == curses.KEY_DOWN:
                    sender.send_string(f"{name} down")
                elif key == ord('q'):
                    raise KeyboardInterrupt
                else:
                    logging.warning(f"{name}: unknown key {key} 0{key:o}")

            if key == -1:
                n += 1
                if n % 1000 == 0:
                    sender.send_string(f"{name} hello")
                    logging.debug("refresh hello")

            # sleep for 20 msec
            time.sleep(0.02)

    except KeyboardInterrupt:
        pass

    sender.send_string(f"{name} bye")
    logging.info("bye")

    logger.removeHandler(handler)

    sender.close(100)
    receiver.close(100)
    zmqlog.close()
    context.term()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--verbose", help="increase verbosity", action='store_true')
    parser.add_argument("-s", "--server", help="server address", default="localhost")
    parser.add_argument("name", help="subscriber name")
    args = parser.parse_args()

    curses.wrapper(play, args.name, args.server)
    print("bye")


if __name__ == "__main__":
    main()
