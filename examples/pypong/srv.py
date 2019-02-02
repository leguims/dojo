#! /usr/bin/env python3

"""
PONG game server
"""

import zmq
import random
import math


def main():

    # Prepare our context
    context = zmq.Context()

    # Socket to publish game updates
    publisher = context.socket(zmq.PUB)
    publisher.bind("tcp://*:5556")

    # Socket to receive messages
    receiver = context.socket(zmq.PULL)
    receiver.bind("tcp://*:5555")

    # Initialize poll set
    poller = zmq.Poller()
    poller.register(publisher, zmq.POLLIN)
    poller.register(receiver, zmq.POLLIN)

    ball_x, ball_y = 0, 0
    ball_angle = random.random() * math.pi * 2
    players = {}
    last_msg = None
    SIZE = 40

    def print_one(msg):
        nonlocal last_msg
        if last_msg != msg:
            print(msg)
            last_msg = msg

    while True:
        try:
            socks = dict(poller.poll(20))
        except KeyboardInterrupt:
            break

        if receiver in socks:
            message = receiver.recv_string()
            # process update
            print("got:", message)
            name, command = message.split()
            if command == "hello":
                if name not in players:
                    players[name] = {'position': 0}
            elif command == "bye":
                if name in players:
                    del players[name]
            elif command == "up":
                if name not in players:
                    players[name] = {'position': 0}
                players[name]['position'] = min(32, players[name]['position'] + 8)
            elif command == "down":
                if name not in players:
                    players[name] = {'position': 0}
                players[name]['position'] = max(-32, players[name]['position'] - 8)

        if len(players) == 2:

            # here is the game logic

            # move the ball
            ball_x = ball_x + math.cos(ball_angle * math.pi / 180)
            ball_y = ball_y + math.sin(ball_angle * math.pi / 180)

            # bounce it
            if ball_x > SIZE:
                ball_angle = 180 - ball_angle
                ball_x = SIZE - 1
                ball_angle += random.randint(-1, 1)
            elif ball_x < -SIZE:
                ball_angle = 180 - ball_angle
                ball_x = -SIZE + 1
                ball_angle += random.randint(-1, 1)

            if ball_y > SIZE:
                ball_angle = -ball_angle
                ball_y = SIZE - 1
                ball_angle += random.randint(-1, 1)
            elif ball_y < -SIZE:
                ball_angle = -ball_angle
                ball_y = -SIZE + 1
                ball_angle += random.randint(-1, 1)

            # add random
            ball_angle = ball_angle % 360
            if ball_angle <= 5 or ball_angle >= 355:
                ball_angle += random.randint(5, 20)
            if 85 <= ball_angle <= 95 or -85 <= ball_angle <= -95:
                ball_angle -= random.randint(5, 20)

            # update for the clients
            msg = "play {:.3f} {},{}".format(0, int(ball_x), int(ball_y))
            for k, v in players.items():
                msg += " {}:{}".format(k, v['position'])

            print_one(msg + " heading {:.1f}".format(ball_angle % 360))
            publisher.send_multipart([b'game', msg.encode()])
        else:
            msg = "waiting {}".format(2 - len(players))
            print_one(msg)
            publisher.send_multipart([b'game', msg.encode()])

    # We never get here but clean up anyhow
    publisher.close()
    receiver.close()
    context.term()


if __name__ == "__main__":
    main()
