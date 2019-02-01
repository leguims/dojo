#! /usr/bin/env python3

import zmq


context = zmq.Context.instance()
client = context.socket(zmq.ROUTER)
client.bind("tcp://*:2020")

while True:
    ident = client.recv_multipart()
    print(ident)
    client.send_multipart([ident[0], b'OK', b'KKKK'])
