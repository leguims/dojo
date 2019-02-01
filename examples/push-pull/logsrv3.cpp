//
// czmq example
//

#include <bits/stdc++.h>
#include <sys/time.h>
#include <zmq_addon.hpp>


static void print_timestamp()
{
    struct timeval now;
    char buf[sizeof "2018-11-23T09:00:00"];

    gettimeofday(&now, NULL);
    strftime(buf, sizeof buf, "%FT%T", localtime(&now.tv_sec));

    printf("%s.%06ld - ", buf, (long)now.tv_usec);
}


int main()
{
    zmq::context_t  context(1);
    zmq::socket_t   sink(context, ZMQ_PULL);
    sink.bind("ipc:///tmp/dojo_logger");

    while (true) {
        zmq::multipart_t msg;
        zmq::message_t  part;

        msg.recv(sink);

        // two parts: topic.level and text
        assert(msg.size() == 2);

        // part 1
        part = msg.pop();
        print_timestamp();
        fwrite(part.data(), part.size(), 1, stdout);

        // part 2
        printf(" - ");
        part = msg.pop();
        fwrite(part.data(), part.size(), 1, stdout);

    }

    return 0;
}
