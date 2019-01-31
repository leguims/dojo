#! /usr/bin/env python3

"""
0MQ example: REP/REQ pattern
"""

import zmq
import sys

context = zmq.Context()
print("Connecting to server...")
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:5555")

request = "python" if len(sys.argv) == 1 else sys.argv[1]

# Send request
print("Sending request:", request)
socket.send_string(request)

# Get the reply
message = socket.recv_string()
print("Received reply:", message)
