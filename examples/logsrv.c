//
// zmq example
//

#include <zmq.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>


int main()
{
    void *context = zmq_ctx_new();

    //  Socket to send start of batch message on
    void *sink = zmq_socket(context, ZMQ_PULL);
    zmq_bind(sink, "ipc:///tmp/dojo_logger");

    //  Process tasks forever
    while (1) {
        int more;
        int num_part = 0;

        do {
            /* Create an empty ØMQ message to hold the message part */
            zmq_msg_t part;
            int rc = zmq_msg_init(&part);
            assert(rc == 0);

            /* Block until a message is available to be received from socket */
            rc = zmq_msg_recv(&part, sink, 0);
            assert (rc != -1);

            ++num_part;

            if (num_part == 1)
            {
                struct timeval now;
                gettimeofday(&now, NULL);
                char buf[sizeof "2018-11-23T09:00:00Z"];
                strftime(buf, sizeof buf, "%FT%T", localtime(&now.tv_sec));

                printf("%s.%06ld", buf, (long)now.tv_usec);
            }

            printf(" - ");
            fwrite(zmq_msg_data(&part), zmq_msg_size(&part), 1, stdout);

            // Look if there is another part to come
            more = zmq_msg_more(&part);

            zmq_msg_close(&part);
        } while (more);
    }

    zmq_close(sink);
    zmq_ctx_destroy(context);
    return 0;
}
