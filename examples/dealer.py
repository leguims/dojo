#! /usr/bin/env python3

import zmq

context = zmq.Context.instance()

client = context.socket(zmq.DEALER)
client.identity = b'xxx-111'
client.connect("tcp://localhost:9999")

client.send_string('blahblah')
msg = client.recv()
print('Client received:', msg)

