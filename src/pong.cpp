//
// Coding Dojo
//

#include <bits/stdc++.h>
#include <boost/program_options.hpp>
#include <cmath>
#include <ncurses.h>
#undef addstr
#include <sys/time.h>
#include <zmq.h>
#include <zmq.hpp>
#include <zmq_addon.hpp>

using namespace std;

//
// DEMO
//

#define FIELD_WIDTH 79
#define FIELD_HEIGHT 24
#define PADDLE_SIZE 5

#define MAX_PADDLE_POS ((FIELD_HEIGHT - PADDLE_SIZE) / 2 - 1)
#define MIN_PADDLE_POS (-(MAX_PADDLE_POS + 1))

#define PORT_LOG "2019"
#define PORT_GAME "2020"

zmq::context_t context(1);
zmq::socket_t client_log(context, ZMQ_PUSH);
zmq::socket_t client_game(context, ZMQ_DEALER);

string client_name;

// like Arduino's one
long millis()
{
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_nsec / 1000000 + spec.tv_sec * 1000L;
}

struct PlayerState
{
    bool left;
    int pos;
    long last_seen;

    void update(const string &command)
    {
        last_seen = millis();

        if (command == "UP")
        {
            if (pos < MAX_PADDLE_POS)
                ++pos;
        }
        else if (command == "DOWN")
        {
            if (pos > MIN_PADDLE_POS)
                --pos;
        }
    }
};

void reset_screen()
{
    delwin(stdscr);
    endwin();
    refresh();
}

void init_screen()
{
    initscr(); // start ncurses

    if (!has_colors())
    {
        reset_screen();
        // error: we want colors
        exit(2);
    }
    start_color(); // start colors

    if (COLOR_PAIRS < 16)
    {
        reset_screen();
        // error: we want at least 16 pairs
        exit(2);
    }

    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_pair(4, COLOR_RED | COLOR_GREEN, COLOR_BLACK);
    init_pair(5, COLOR_BLACK, COLOR_BLACK);

    cbreak();             // line buffering disabled
    keypad(stdscr, true); // enables F_keys
    noecho();             // disables echoing of characters
    curs_set(0);          // hide cursor

    int width, height;
    getmaxyx(stdscr, height, width);

    if (height < FIELD_HEIGHT || width < FIELD_WIDTH)
    {
        reset_screen();
        // error: terminal too small
        exit(2);
    }

    atexit(reset_screen); // vacuum
}

void draw_field()
{
    // draw a rectangle
    for (int i = 0; i < FIELD_HEIGHT; i++)
    {
        mvaddstr(i, 0, "*");
        mvaddstr(i, FIELD_WIDTH - 1, "*");
    }
    for (int i = 0; i < FIELD_WIDTH; i++)
    {
        mvaddstr(0, i, "*");
        mvaddstr(FIELD_HEIGHT - 1, i, "*");
    }
    char buf[32];
    int n = snprintf(buf, sizeof(buf), " 0 - 0 ");
    attron(COLOR_PAIR(1));
    mvaddstr(0, (FIELD_WIDTH - n) / 2, buf);
    attron(COLOR_PAIR(1));

    // draw the net
    attron(COLOR_PAIR(2));
    mvvline(1, FIELD_WIDTH / 2, ACS_VLINE, FIELD_HEIGHT - 2);
    attron(COLOR_PAIR(2));
}

void draw_paddle(bool left, int pos)
{
    int x = left ? 2 : FIELD_WIDTH - 3;
    int y = (FIELD_HEIGHT - PADDLE_SIZE) / 2 - pos;

    attron(COLOR_PAIR(5));
    mvvline(1, x, ' ', FIELD_HEIGHT - 2);
    attron(COLOR_PAIR(5));

    attron(COLOR_PAIR(4));
    mvvline(y, x, ACS_VLINE, PADDLE_SIZE);
    attron(COLOR_PAIR(4));
}

void draw_ball(int pos_x, int pos_y, bool clear = false)
{
    int y = FIELD_HEIGHT / 2 - pos_y;
    int x = FIELD_WIDTH / 2 + pos_x;

    if (clear)
    {
        if (pos_x == 0)
        {
            attron(COLOR_PAIR(2));
            mvaddch(y, x, ACS_VLINE);
            attron(COLOR_PAIR(2));
        }
        else
        {
            mvaddch(y, x, ' ');
        }
    }
    else
    {
        attron(COLOR_PAIR(3));
        mvaddch(y, x, 'o');
        attron(COLOR_PAIR(3));
    }
}

void send_log(const string &level, const string &source, const string &description)
{
    zmq::multipart_t msg;
    msg.addstr(level);
    msg.addstr(source);
    msg.addstr(description);
    msg.send(client_log, 0);
}

void send_server(const string &command)
{
    zmq::multipart_t msg;
    msg.addstr(command);
    msg.send(client_game, 0);
}

void client_graphique()
{
    init_screen();
    draw_field();
    draw_paddle(true, 0);
    draw_paddle(false, 1);
    nodelay(stdscr, 1); // non blocking key input

    client_game.setsockopt(ZMQ_IDENTITY, client_name.data(), client_name.size());
    client_game.connect("tcp://localhost:" PORT_GAME);

    long last_ping = millis();
    bool bye = false;

    int ball_x = 0, ball_y = 0;

    draw_ball(ball_x, ball_y);

    while (!bye)
    {
        int c = getch(); // blocking key input
        switch (c)
        {
        case KEY_UP:
            send_log("INFO", client_name, "UP");
            send_server("UP");
            break;

        case KEY_DOWN:
            send_log("INFO", client_name, "DOWN");
            send_server("DOWN");
            break;

        case 'r':
            draw_field();
            break;

        case 27:
        case 'q':
            bye = true;
            break;

        default:
        {
            long now = millis();
            if (now - last_ping > 200)
            {
                last_ping = now;
                send_server("PING");
            }
        }
        break;
        }

        // Read ZMQ
        zmq::multipart_t msg;
        msg.recv(client_game, ZMQ_DONTWAIT);

        if (!msg.empty())
        {
            draw_ball(ball_x, ball_y, true);

            ball_x = stoi(msg.popstr());
            ball_y = stoi(msg.popstr());

            draw_ball(ball_x, ball_y);

            while (msg.size() >= 3)
            {
                string name, left, pos;
                name = msg.popstr();
                left = msg.popstr();
                pos = msg.popstr();
                // send_log("INFO", client_name, "Update screen " + name + " " + left + " " + pos);

                draw_paddle(left == "1", stoi(pos));
            }
        }

        usleep(10000);
    }

    getch();
}

static void print_timestamp()
{
    struct timeval now;
    char buf[sizeof "2018-11-23T09:00:00"];

    gettimeofday(&now, NULL);
    strftime(buf, sizeof buf, "%FT%T", localtime(&now.tv_sec));

    printf("%s.%06ld - ", buf, (long)now.tv_usec);
}

void logsrv()
{
    zmq::socket_t serveur(context, ZMQ_PULL);

    serveur.bind("tcp://*:" PORT_LOG);

    while (true)
    {
        zmq::multipart_t msg;
        zmq::message_t level, source, description;

        msg.recv(serveur);

        // 3 parts: level, source and text
        assert(msg.size() == 3);
        level = msg.pop();
        source = msg.pop();
        description = msg.pop();

        print_timestamp();

        fwrite(level.data(), level.size(), 1, stdout);
        printf(" - ");
        fwrite(source.data(), source.size(), 1, stdout);
        printf(" - ");
        fwrite(description.data(), description.size(), 1, stdout);
        printf("\n");
    }
}

void server()
{
    zmq::socket_t server_socket(context, ZMQ_ROUTER);

    server_socket.bind("tcp://*:" PORT_GAME);
    send_log("INFO", "GAME ENGINE", "Start");

    map<string, PlayerState> players;

    double ball_x = 0, ball_y = 0;
    double speed = 0;
    double heading = 0;

    while (true)
    {
        zmq::multipart_t msg;

        msg.recv(server_socket, ZMQ_DONTWAIT);

        if (!msg.empty())
        {
            assert(msg.size() == 2);

            string name = msg.popstr();
            string command = msg.popstr();

            //send_log("INFO", "GAME ENGINE", "Received " + name + " " + command);

            auto &&it = players.find(name);

            if (it != players.end())
            {
                it->second.update(command);
            }
            else if (players.size() < 2)
            {
                PlayerState new_player;

                new_player.left = players.empty() ? true : !players.begin()->second.left;
                new_player.pos = 0;
                new_player.last_seen = millis();

                //new_player.update(command);

                players[name] = new_player;
                send_log("INFO", "GAME ENGINE", "Add new player " + name);

                if (players.size() == 2)
                {
                    ball_x = 0;
                    ball_y = 0;
                    speed = 0.1;
                    heading = 10;
                }
            }
            else
            {
                // cannot add a new player
                send_log("WARNING", "GAME ENGINE", "Cannot add player " + name);
            }
        }

        // remove stuck clients
        long now = millis();
        for (auto &&player : players)
        {
            if (now > player.second.last_seen + 500)
            {
                send_log("INFO", "GAME ENGINE", "Bye bye " + player.first);
                players.erase(player.first);
                speed = 0;
                break;
            }
        }

        if (speed > 0)
        {
            ball_x += speed * cos(heading * M_PI / 180);
            ball_y += speed * sin(heading * M_PI / 180);

            if (ball_x > 38 || ball_x < -38)
            {
                heading = 180 - heading;
                ball_x += speed * cos(heading * M_PI / 180);
                ball_y += speed * sin(heading * M_PI / 180);
            }
            else if (ball_y > 11 || ball_y < -10)
            {
                heading = -heading;
                ball_x += speed * cos(heading * M_PI / 180);
                ball_y += speed * sin(heading * M_PI / 180);
            }

            if (speed < 0.4)
                speed += 0.0001;
        }

        // Send object positions to all clients

        for (auto &&player : players)
        {
            // first part: identity of the DEALER
            zmq::multipart_t msg_update;
            msg_update.addstr(player.first);

            // second/third part: ball position X/position Y
            msg_update.addstr(to_string(lround(ball_x)));
            msg_update.addstr(to_string(lround(ball_y)));

            // player states
            for (auto &&j : players)
            {
                msg_update.addstr(j.first);
                msg_update.addstr(to_string(j.second.left));
                msg_update.addstr(to_string(j.second.pos));
            }

            // send_log("INFO", "GAME ENGINE", "Update screen " + player.first);
            msg_update.send(server_socket);

            // remove first part
            // msg_update.popstr();
        }

        usleep(10000);
    }
}

int main(int argc, char *argv[])
{
    try
    {
        //connect log client
        client_log.connect("tcp://localhost:" PORT_LOG);

        //
        // Define and parse the program options
        //

        namespace po = boost::program_options;
        po::options_description desc("Options");
        desc.add_options()("help,h", "Print help");
        desc.add_options()("value,v", po::value<string>(), "name");
        desc.add_options()("client,c", po::value<string>(), "passer le nom du client en parametre");
        desc.add_options()("server", "launch server");
        desc.add_options()("logsrv", "logsrv");

        po::variables_map vm;
        try
        {
            po::store(po::parse_command_line(argc, argv, desc), vm); // can throw

            // --help option
            if (vm.count("help"))
            {
                cout << "Pong App" << endl
                     << desc << endl;
                return 0;
            }

            po::notify(vm); // throws on error, so do after help in case there are any problems
        }
        catch (po::error &e)
        {
            cerr << "ERROR: " << e.what() << endl
                 << endl;
            cerr << desc << endl;
            return 1;
        }

        //
        // Application code here
        //
        // --value=
        if (vm.count("value"))
        {
            cout << "value = " << vm["value"].as<string>() << endl;
        }

        // --demo
        if (vm.count("client"))
        {
            client_name = vm["client"].as<string>();
            send_log("INFO", client_name, "Start");
            client_graphique();
        }

        // --server
        if (vm.count("server"))
        {
            send_log("INFO", "server", "Start");
            server();
        }

        if (vm.count("logsrv"))
        {
            logsrv();
        }
    }
    catch (exception &e)
    {
        cerr << "Unhandled Exception reached the top of main: "
             << e.what() << ", application will now exit" << endl;
        return 2;
    }

    return 0;
}
