//
// zmq.hpp example: REP/REQ pattern
//

#include <zmq.hpp>
#include <string>
#include <iostream>
#include <unistd.h>

int main()
{
    //  Prepare our context and socket
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REP);
    socket.bind("tcp://*:5555");

    while (true)
    {
        zmq::message_t request;

        //  Wait for next request from client
        socket.recv(&request);
        std::string hello(static_cast<const char *>(request.data()), request.size());
        std::cout << "Received: " << hello << std::endl;

        std::reverse(hello.begin(), hello.end());

        //  Send reply back to client
        zmq::message_t reply(hello.length());
        memcpy(reply.data(), hello.data(), hello.length());
        socket.send(reply);
    }

    return 0;
}
