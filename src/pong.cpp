//
// Coding Dojo
//

#include <bits/stdc++.h>
#include <boost/program_options.hpp>
#include <cmath>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <ncurses.h>
#include <zmq.h>
#include <sys/time.h>

using namespace std;

//
// DEMO
//

#define FIELD_WIDTH 79
#define FIELD_HEIGHT 24
#define PADDLE_SIZE 5

#define PORT_LOG "2019"
#define PORT_GAME "2020"

zmq::context_t context(2);
zmq::socket_t client_log(context, ZMQ_PUSH);
zmq::socket_t client_game(context, ZMQ_DEALER);

string client_name;

#define MAX_Y ((FIELD_HEIGHT - PADDLE_SIZE) / 2-1)
#define MIN_Y -(MAX_Y+1)

struct Joueur
{
    string name;
    bool left;
    int y;

    void update(string command)
    {
        if (command == "UP")
        {
            if (y < MAX_Y)
                y = y + 1;
        }
        else if (command == "DOWN")
        {
            if (y > MIN_Y)
                y = y - 1;
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
    mvvline(1, x, ACS_VLINE, FIELD_HEIGHT - 2);
    attron(COLOR_PAIR(5));

    attron(COLOR_PAIR(4));
    mvvline(y, x, ACS_VLINE, PADDLE_SIZE);
    attron(COLOR_PAIR(4));
}

void sendlog(string level, string source, string description)
{

    zmq::multipart_t msg;

    msg.add(zmq::message_t(level.data(), level.size()));
    msg.add(zmq::message_t(source.data(), source.size()));
    msg.add(zmq::message_t(description.data(), description.size()));

    msg.send(client_log, 0);
}

void send_server(string command)
{
    zmq::multipart_t msg;

    //    msg.push(zmq::message_t(client_name.data(), client_name.size()));
    msg.add(zmq::message_t(command.data(), command.size()));

    msg.send(client_game, 0);
}

void client_graphique()
{
    init_screen();
    draw_field();
    draw_paddle(true, 0);
    draw_paddle(false, 1);
    nodelay(stdscr, 1); // non blocking key input
    int c;

    client_game.setsockopt(ZMQ_IDENTITY, client_name.data(), client_name.size());
    client_game.connect("tcp://localhost:" PORT_GAME);

    while (1)
    {
        c = getch(); // blocking key input
        switch (c)
        {
        case KEY_UP:
            sendlog("INFO", client_name, "UP");
            send_server("UP");
            break;

        case KEY_DOWN:
            sendlog("INFO", client_name, "DOWN");
            send_server("DOWN");
            break;

        default:
            break;
        }

        // Read ZMQ
        zmq::multipart_t msg;
        msg.recv(client_game, ZMQ_DONTWAIT);

        if (!msg.empty())
        {
            while(msg.size()>=3){
                /* code */

                string name, left, y;
                sendlog("INFO", client_name, "Update screen from server");
                name = msg.popstr();
                left = msg.popstr();
                y = msg.popstr();
                sendlog("INFO", client_name, "Update screen : "+name+" "+left+" "+y);

                draw_paddle(left=="1", stoi(y));
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

        // level
        fwrite(level.data(), level.size(), 1, stdout);

        // source
        printf(" - ");
        fwrite(source.data(), source.size(), 1, stdout);

        // source
        printf(" - ");

        fwrite(description.data(), description.size(), 1, stdout);

        printf("\n");
    }
}

void add_str(zmq::multipart_t &msg, const string &str)
{
    msg.add(zmq::message_t(str.data(), str.size()));
}

void server()
{
    zmq::socket_t serveur(context, ZMQ_ROUTER);

    serveur.bind("tcp://*:" PORT_GAME);
    sendlog("INFO", "GAME ENGINE", "Start");

    map<string, Joueur> mapJoueur;

    while (true)
    {
        zmq::multipart_t msg;
        zmq::message_t name, command;

        msg.recv(serveur);

        assert(msg.size() == 2);

        name = msg.pop();
        command = msg.pop();

        string s_name(static_cast<const char *>(name.data()), name.size());
        string s_command(static_cast<const char *>(command.data()), command.size());

        sendlog("INFO", "GAME ENGINE", "commande " + s_name + " " + s_command);

        auto it = mapJoueur.find(s_name);

        if (it != mapJoueur.end())
        {
            it->second.update(s_command);
        }
        else
        {
            Joueur newjoueur;

            newjoueur.left = mapJoueur.size() % 2 == 0 ? true : false;
            newjoueur.y = 0;
            newjoueur.name = s_name;

            newjoueur.update(s_command);

            mapJoueur[s_name] = newjoueur;
            sendlog("INFO", "GAME ENGINE", "Add new player " + s_name);
        }

        // Send object positions
        sendlog("INFO", "GAME ENGINE", "Update screen");

        for (auto &joueur : mapJoueur)
        {
            zmq::multipart_t msg_update;

            add_str(msg_update, joueur.first);

            for (auto &j : mapJoueur)
            {
                add_str(msg_update, j.second.name);
                add_str(msg_update, to_string(j.second.left));
                add_str(msg_update, to_string(j.second.y));
            }

            sendlog("INFO", "GAME ENGINE", "Update screen " + joueur.first);
            msg_update.send(serveur);
        }
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
            sendlog("INFO", client_name, "Start");
            client_graphique();
        }

        // --server
        if (vm.count("server"))
        {
            sendlog("INFO", "server", "Start");
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
