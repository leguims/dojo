#! /usr/bin/env python3

import zmq


context = zmq.Context.instance()
client = context.socket(zmq.ROUTER)
client.bind("tcp://*:9999")

while True:
    ident, request = client.recv_multipart()
    print(ident, request)
    client.send_multipart([ident, b'OK'])
