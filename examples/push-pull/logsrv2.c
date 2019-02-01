//
// czmq example
//

#include <czmq.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>


static void print_timestamp()
{
    struct timeval now;
    char buf[sizeof "2018-11-23T09:00:00"];

    gettimeofday(&now, NULL);
    strftime(buf, sizeof buf, "%FT%T", localtime(&now.tv_sec));

    printf("%s.%06ld - ", buf, (long)now.tv_usec);
}


static bool ends_with(const byte *data, size_t size, const char *with)
{
    size_t len_with = strlen(with);

    if (len_with == size)
        return memcmp(data, with, size) == 0;

    return (size > len_with)
           && data[size - len_with - 1] == '.'
           && (memcmp(data + size - len_with,  with, len_with) == 0);
}


static const char *get_sgr_color(zframe_t *frame)
{
    const byte *data = zframe_data(frame);
    size_t size = zframe_size(frame);

    if (ends_with(data, size, "DEBUG"))
        return "\033[1;30m";    // LIGHT_BLACK

    else if (ends_with(data, size, "WARNING"))
        return "\033[0;33m";    // YELLOW

    else if (ends_with(data, size, "INFO"))
        return "\033[1;32m";    // GREEN

    else if (ends_with(data, size, "ERROR"))
        return "\033[0;31m";    // RED

    else if (ends_with(data, size, "CRITICAL"))
        return "\033[0;35m";    // MAGENTA

    return "";
}


int main()
{
    zsock_t *sink = zsock_new_pull("ipc:///tmp/dojo_logger");

    while (true) {

        zmsg_t *msg;
        zframe_t *frame;

        msg = zmsg_recv(sink);

        // two parts: topic.level and text
        assert(zmsg_size(msg) == 2);

        // part 1
        frame = zmsg_pop(msg);
        printf("%s", get_sgr_color(frame));
        print_timestamp();
        fwrite(zframe_data(frame), zframe_size(frame), 1, stdout);
        zframe_destroy(&frame);

        // part 2
        frame = zmsg_pop(msg);
        printf(" - ");
        fwrite(zframe_data(frame), zframe_size(frame), 1, stdout);
        printf("\033[0m");      // END
        zframe_destroy(&frame);

        zmsg_destroy(&msg);
    }

    zsock_destroy(&sink);
    return 0;
}
