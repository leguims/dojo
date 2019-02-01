import zmq


def main():
    context = zmq.Context()

    zmqlog = context.socket(zmq.PUSH)
    zmqlog.connect("tcp://localhost:2019")

    log = [b"Warning", b"logcli", b"essai de message"]

    zmqlog.send_multipart(log)


if __name__ == '__main__':
    main()
