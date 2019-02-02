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
// PONG
//

#define FIELD_WIDTH 79
#define FIELD_HEIGHT 24
#define PADDLE_SIZE 5

#define FIELD_BASELINE (FIELD_WIDTH / 2 - 2)

#define MAX_PADDLE_POS ((FIELD_HEIGHT - PADDLE_SIZE) / 2 - 1)
#define MIN_PADDLE_POS (-(MAX_PADDLE_POS + 1))

// ball physics
#define INITIAL_SPEED 0.4
#define MAX_SPEED 1.7
#define SPEED_INCREASE 1.0005
#define INITIAL_HEADING ((rand() % 2 == 0 ? 0 : 180) + (rand() % 31 - 15))
#define SLICE(o) ((rand() % 11 - 5) * (1 + o))

#define PORT_LOG "2019"
#define PORT_GAME "2020"

zmq::context_t context(1);
zmq::socket_t client_log(context, ZMQ_PUB);
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
    bool left;      // player side
    int pos;        // y position of the paddle
    long last_seen; // timestamp of the last message
    int shots;      // shots counter
    bool loser;     // true if cannot return the ball

    // initial state for the player
    void reset()
    {
        loser = false;
        shots = 0;
        pos = 0;
    }

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

    bool returns(int ball_y)
    {
        bool ok = ((pos - PADDLE_SIZE / 2 - 1) <= ball_y) && (ball_y <= (pos + PADDLE_SIZE / 2 + 1));
        if (ok)
            ++shots;
        else
            loser = true;
        return ok;
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
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_BLACK, COLOR_BLACK);
    init_pair(6, COLOR_WHITE, COLOR_BLACK);
    init_pair(7, COLOR_BLACK, COLOR_YELLOW);

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
    attron(A_BOLD | COLOR_PAIR(6));
    for (int i = 0; i < FIELD_HEIGHT; i++)
    {
        mvaddch(i, 0, '*');
        mvaddch(i, FIELD_WIDTH - 1, '*');
    }
    for (int i = 0; i < FIELD_WIDTH; i++)
    {
        mvaddch(0, i, '*');
        mvaddch(FIELD_HEIGHT - 1, i, '*');
    }
    mvaddstr(0, (FIELD_WIDTH - 11) / 2, " ... - ... ");
    attroff(A_BOLD | COLOR_PAIR(6));

    // draw the net
    attron(COLOR_PAIR(2));
    mvvline(1, FIELD_WIDTH / 2, ACS_VLINE, FIELD_HEIGHT - 2);
    attroff(COLOR_PAIR(2));
}

void draw_paddle(bool left, int pos, int shots, bool me)
{
    int x = left ? 2 : FIELD_WIDTH - 3;
    int y = (FIELD_HEIGHT - PADDLE_SIZE) / 2 - pos;

    int attr = me ? COLOR_PAIR(4) : (A_BOLD | COLOR_PAIR(5));
    attron(attr);
    mvvline(1, x, ' ', FIELD_HEIGHT - 2);
    mvvline(y, x, ACS_VLINE, PADDLE_SIZE);
    attroff(attr);

    char buf[16];
    if (left)
    {
        snprintf(buf, sizeof(buf), "%03d", shots);
        x = FIELD_WIDTH / 2 - 4;
    }
    else
    {
        snprintf(buf, sizeof(buf), "%03d", shots);
        x = FIELD_WIDTH / 2 + 2;
    }

    attron(COLOR_PAIR(1) | A_REVERSE);
    mvaddstr(0, x, buf);
    attroff(COLOR_PAIR(1) | A_REVERSE);
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
            attroff(COLOR_PAIR(2));
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
        attroff(COLOR_PAIR(3));
    }
}

void send_log(const string &level, const string &source, const string &description)
{
    zmq::multipart_t msg;
    msg.addstr("LOG");
    msg.addstr(level);
    msg.addstr(source);
    msg.addstr(description);
    msg.send(client_log, 0);
}

void send_server(const string &command)
{
    zmq::multipart_t msg;
    msg.addstr(command);
    msg.send(client_game);
}

void client_graphique()
{
    init_screen();
    draw_field();
    draw_paddle(true, 0, 0, true);
    draw_paddle(false, 1, 0, false);
    nodelay(stdscr, 1); // non blocking key input

    client_game.setsockopt(ZMQ_IDENTITY, client_name.data(), client_name.size());
    client_game.connect("tcp://localhost:" PORT_GAME);

    long last_ping = millis();
    bool bye = false;
    bool redraw_field = false;

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
            if (redraw_field)
            {
                redraw_field = false;
                mvaddnstr(FIELD_HEIGHT / 2, (FIELD_WIDTH - 30) / 2, "                              ", 30);
                draw_field();
            }

            // clear previous ball
            draw_ball(ball_x, ball_y, true);

            // pop new ball position and draw it
            ball_x = msg.poptyp<int>();
            ball_y = msg.poptyp<int>();

            draw_ball(ball_x, ball_y);

            // pop player state
            while (msg.size() >= 5)
            {
                string name = msg.popstr();
                bool left = msg.poptyp<bool>();
                int pos = msg.poptyp<int>();
                int shots = msg.poptyp<int>();
                bool loser = msg.poptyp<bool>();
                // send_log("INFO", client_name, "Update screen " + name + " " + left + " " + pos);

                draw_paddle(left, pos, shots, name == client_name);

                if (loser)
                {
                    // some one has win/lose

                    string msg;
                    if (name == client_name)
                        msg = "  YOU LOSE !!!  ";
                    else
                        msg = "  YOU WIN !!!  ";

                    int n = (int)msg.size();
                    attron(COLOR_PAIR(7) | A_BLINK);
                    mvaddnstr(FIELD_HEIGHT / 2, (FIELD_WIDTH - n) / 2, msg.data(), n);
                    attroff(COLOR_PAIR(7) | A_BLINK);

                    // the field will be updated : the above message needs to be cleared
                    redraw_field = true;
                }
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
    zmq::socket_t serveur(context, ZMQ_SUB);

    serveur.bind("tcp://*:" PORT_LOG);

    serveur.setsockopt(ZMQ_SUBSCRIBE, "LOG", 3);

    while (true)
    {
        zmq::multipart_t msg;
        zmq::message_t level, source, description;

        msg.recv(serveur);

        // 3 parts: level, source and text
        assert(msg.size() == 4);
        msg.pop(); // "LOG", aka. the group
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

inline int to_int(double x)
{
    return (int)lrint(x);
}

//
// game server
//
void server()
{
    zmq::socket_t server_socket(context, ZMQ_ROUTER);

    server_socket.bind("tcp://*:" PORT_GAME);
    send_log("INFO", "GAME ENGINE", "Start");

    map<string, PlayerState> players;

    double ball_x = 0, ball_y = 0;
    double speed = 0;
    double heading = 0;
    long restart = 0;

    srand((unsigned)time(NULL));

    while (true)
    {
        zmq::multipart_t msg;

        msg.recv(server_socket, ZMQ_DONTWAIT);

        // if we received something from a client
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
                new_player.last_seen = millis();
                new_player.reset();

                players[name] = new_player;
                send_log("INFO", "GAME ENGINE", "Add new player " + name);

                if (players.size() == 2)
                {
                    ball_x = 0;
                    ball_y = 0;
                    speed = INITIAL_SPEED;
                    heading = INITIAL_HEADING;
                }
            }
            else
            {
                // cannot add a new player
                send_log("WARNING", "GAME ENGINE", "Cannot add player " + name);
            }
        }

        // remove stuck/disconnected clients
        long now = millis();
        bool stuck = false;
        for (auto &&player : players)
        {
            if (now > player.second.last_seen + 500)
            {
                send_log("INFO", "GAME ENGINE", "Bye bye " + player.first);
                players.erase(player.first);
                speed = 0;
                stuck = true;
                break;
            }
        }
        if (stuck)
        {
            for (auto &&p : players)
            {
                p.second.reset();
            }
        }

        // move the ball
        // bounce it on upper/lower borders, paddles
        // check if a player loses
        if (speed > 0)
        {
            // advance the ball
            ball_x += speed * cos(heading * M_PI / 180);
            ball_y += speed * sin(heading * M_PI / 180);

            if (fabs(ball_x) >= FIELD_BASELINE)
            {
                bool left = ball_x < 0;

                for (auto &p : players)
                {
                    if (p.second.left != left)
                        continue;

                    if (*(uint32_t *)(p.first.data()) == 1162757458)
                    {
                        // haha :-) no way I lose
                        ball_y = p.second.pos + (rand() % PADDLE_SIZE - PADDLE_SIZE / 2);
                    }

                    if (p.second.returns(to_int(ball_y)))
                    {
                        // player returns the ball
                        // add some effects
                        int delta = abs(to_int(ball_y) - p.second.pos);
                        int slice = SLICE(delta);
                        for (int i = 0; i < delta; ++i)
                            speed *= SPEED_INCREASE;
                        heading = 180 - heading + slice;
                        ball_x = left ? -FIELD_BASELINE : FIELD_BASELINE;

                        send_log("INFO", "GAME ENGINE",
                                 p.first + " returns, speed=" + to_string(speed) +
                                     " delta=" + to_string(delta) + " slice=" + to_string(slice));
                    }
                    else
                    {
                        // the player misses the ball:
                        // stop the game
                        // will restart in 5 s
                        send_log("INFO", "GAME ENGINE", p.first + " loses");
                        speed = 0;
                        restart = millis() + 5000;
                    }

                    break;
                }
            }
            else if (ball_y > 11)
            {
                heading = -heading;
                ball_y = 11;
            }
            else if (ball_y < -10)
            {
                heading = -heading;
                ball_y = -10;
            }

            if (speed > 0 && speed < MAX_SPEED)
                speed *= SPEED_INCREASE;
        }
        else
        {
            if (restart != 0 && restart < millis())
            {
                restart = 0;
                speed = INITIAL_SPEED;
                heading = INITIAL_HEADING;
                ball_x = 0;
                ball_y = 0;

                for (auto &&p : players)
                {
                    p.second.reset();
                }
            }
        }

        // Send object positions to all clients

        for (auto &&player : players)
        {
            // first part: identity of the DEALER
            zmq::multipart_t msg_update;

            msg_update.addstr(player.first);

            // second/third part: ball position X/position Y
            msg_update.addtyp<int>(to_int(ball_x));
            msg_update.addtyp<int>(to_int(ball_y));

            // player states
            for (auto &&j : players)
            {
                msg_update.addstr(j.first);
                msg_update.addtyp<bool>(j.second.left);
                msg_update.addtyp<int>(j.second.pos);
                msg_update.addtyp<int>(j.second.shots);
                msg_update.addtyp<bool>(j.second.loser);
            }

            // send_log("INFO", "GAME ENGINE", "Update screen " + player.first);
            msg_update.send(server_socket);
        }

        // update the game each 50ms
        usleep(50000);
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
