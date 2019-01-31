//
// zmq.hpp example: REP/REQ pattern
//

#include <zmq.hpp>
#include <string>
#include <iostream>
#include <unistd.h>


int main(int argc, char *argv[])
{
    //  Prepare our context and socket
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REQ);

    std::cout << "Connecting to reverse server..." << std::endl;
    socket.connect("tcp://localhost:5555");

    //  Do a request, waiting for a response

    std::string data = (argc < 2) ? "Hello" : argv[1];

    zmq::message_t request(data.length());
    memcpy(request.data(), data.data(), data.length());
    std::cout << "Sending: " << data << std::endl;
    socket.send(request);

    //  Get the reply.
    zmq::message_t reply;
    socket.recv(&reply);
    std::string str(static_cast<const char *>(reply.data()), reply.size());
    std::cout << "Received: " << str << std::endl;

    return 0;
}
