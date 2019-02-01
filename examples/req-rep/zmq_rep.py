#! /usr/bin/env python3

"""
0MQ example: REP/REQ pattern
"""

import zmq

context = zmq.Context()
socket = context.socket(zmq.REP)

socket.bind("tcp://*:5555")

while True:
    #  Wait for next request from client
    message = socket.recv()
    print("Received request: ", message)
    socket.send(message[::-1])
