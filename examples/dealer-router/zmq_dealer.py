#! /usr/bin/env python3

import zmq


context = zmq.Context.instance()

client = context.socket(zmq.DEALER)

# client.identity = b'xxx-111'


client.setsockopt(zmq.IDENTITY, b"PEER2")


client.connect("tcp://localhost:2020")

for i in range(10):
    client.send_string('UP')
    msg = client.recv()
    print('Client received:', msg)
