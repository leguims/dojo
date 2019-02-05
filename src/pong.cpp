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
#include <random>

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

#define MAX_Y_PADDLE ((FIELD_HEIGHT - PADDLE_SIZE) / 2-1)
#define MIN_Y_PADDLE -(MAX_Y_PADDLE+1)
#define MAX_Y_BALL ((FIELD_HEIGHT-2) / 2-1)
#define MIN_Y_BALL -(MAX_Y_BALL+1)
#define MAX_X (FIELD_WIDTH - 3)
#define MIN_X 2
const string BALL_NAME("BALLE");
const string CMD_ERROR("ERROR");

void sendlog(string level, string source, string description)
{

    zmq::multipart_t msg;

    msg.add(zmq::message_t(level.data(), level.size()));
    msg.add(zmq::message_t(source.data(), source.size()));
    msg.add(zmq::message_t(description.data(), description.size()));

    msg.send(client_log, 0);
}

struct Joueur
{
    string name;
    bool left;
    int y;

    void update(string command)
    {
        if (command == "UP")
        {
            if (y < MAX_Y_PADDLE)
                y = y + 1;
        }
        else if (command == "DOWN")
        {
            if (y > MIN_Y_PADDLE)
                y = y - 1;
        }
    }

    int max_Y() const
    {
        return (y + PADDLE_SIZE/2);
    }
    int min_Y() const
    {
        return (y - PADDLE_SIZE/2);
    }
};

struct Balle
{
    int old_x;
    int old_y;
    int pos_x;
    int pos_y;
    int direction_x;
    int direction_y;

    void update(const Joueur &joueur)
    {
        old_x = pos_x;
        old_y = pos_y;

        if ((pos_y+direction_y) > MAX_Y_BALL)
        {
            // Bounce on the top wall
            pos_y = pos_y - (direction_y - 2*(MAX_Y_BALL - pos_y));
            direction_y = -direction_y;
        }
        else if ((pos_y+direction_y) < MIN_Y_BALL)
        {
            // Bounce on the bottom wall
            pos_y = pos_y - (direction_y + 2*(pos_y - MIN_Y_BALL));
            direction_y = -direction_y;
        }
        else
            pos_y = pos_y + direction_y;
        

        if ( (pos_x < MAX_X)
            && ((pos_x+direction_x) >= MAX_X) 
            && (!joueur.left) 
            && (joueur.min_Y() <= pos_y) && (pos_y <= joueur.max_Y()))
        {
            sendlog("INFO", "GAME ENGINE", "Balle::update 'Bounce on the right paddle' ("+to_string(old_x)+", "+to_string(old_y)+") ; "+"("+to_string(pos_x)+", "+to_string(pos_y)+") ; "+"("+to_string(direction_x)+", "+to_string(direction_y)+")");
            // Bounce on the right paddle
            pos_x = pos_x - (direction_x - 2*(MAX_X - (pos_x+1)));
            direction_x = -direction_x;
        }
        else if ( (pos_x > MIN_X)
            &&((pos_x+direction_x) <= MIN_X)
            && joueur.left 
            && (joueur.min_Y() <= pos_y) && (pos_y <= joueur.max_Y()))
        {
            sendlog("INFO", "GAME ENGINE", "Balle::update 'Bounce on the left paddle' ("+to_string(old_x)+", "+to_string(old_y)+") ; "+"("+to_string(pos_x)+", "+to_string(pos_y)+") ; "+"("+to_string(direction_x)+", "+to_string(direction_y)+")");
            // Bounce on the left paddle
            pos_x = pos_x - (direction_x + 2*((pos_x-1) - MIN_X));
            direction_x = -direction_x;
        }
        else
            pos_x = pos_x + direction_x;
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

void draw_net()
{
    attron(COLOR_PAIR(2));
    mvvline(1, FIELD_WIDTH / 2, ACS_VLINE, FIELD_HEIGHT - 2);
    attron(COLOR_PAIR(2));
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
    draw_net();
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

void draw_ball(int old_x, int old_y, int pos_x, int pos_y)
{
    // Clear old ball
    int x = old_x;
    int y = FIELD_HEIGHT / 2 - old_y -1;

    attron(COLOR_PAIR(5));
    mvaddch(y, x, ACS_BULLET);
    attron(COLOR_PAIR(5));

    // draw the net
    draw_net();

    // Draw new ball
    x = pos_x;
    y = FIELD_HEIGHT / 2 - pos_y -1;

    attron(COLOR_PAIR(4));
    mvaddch(y, x, ACS_BULLET);
    attron(COLOR_PAIR(4));
}

void send_server(string command)
{
    zmq::multipart_t msg;

    //    msg.push(zmq::message_t(client_name.data(), client_name.size())); // msg.push == msg.push_front !!
    msg.add(zmq::message_t(command.data(), command.size()));

    msg.send(client_game, 0);
}

void client_graphique(const string &ip_server)
{
    init_screen();
    draw_field();
    draw_paddle(true, 0);
    draw_paddle(false, 1);
    nodelay(stdscr, 1); // non blocking key input
    int c;

    client_game.setsockopt(ZMQ_IDENTITY, client_name.data(), client_name.size());
    client_game.connect("tcp://"+ip_server+":" PORT_GAME);
    sendlog("INFO", client_name, "Connected to server "+ip_server+":" PORT_GAME);

    while (1)
    {
        c = getch(); // non blocking key input (cf 'nodelay' above)
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
            //sendlog("INFO", client_name, "Update screen from server");
            while(!msg.empty()){
                /* code */

                string name;
                name = msg.popstr();
                if (name == CMD_ERROR)
                {
                    string description = msg.popstr();
                    sendlog("INFO", client_name, "Refuse by server for : "+description);
                    // End of game
                    return;
                }
                if (name == BALL_NAME)
                {
                    string old_x, old_y, pos_x, pos_y, direction_x, direction_y;
                    old_x = msg.popstr();
                    old_y = msg.popstr();
                    pos_x = msg.popstr();
                    pos_y = msg.popstr();
                    direction_x = msg.popstr();
                    direction_y = msg.popstr();
                    //sendlog("INFO", client_name, "Update screen : "+BALL_NAME+" old("+old_x+", "+old_y+") "+" pos("+pos_x+", "+pos_y+") "+" dir("+direction_x+", "+direction_y+") ");
                    draw_ball(stoi(old_x), stoi(old_y), stoi(pos_x), stoi(pos_y));
                }
                else
                {
                    string left, y;
                    left = msg.popstr();
                    y = msg.popstr();
                    //sendlog("INFO", client_name, "Update screen : "+name+" "+left+" "+y);
                    draw_paddle(left=="1", stoi(y));
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

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(-MAX_Y_PADDLE/4, MAX_Y_PADDLE/4);

    serveur.bind("tcp://*:" PORT_GAME);
    sendlog("INFO", "GAME ENGINE", "Start");

    map<string, Joueur> mapJoueur;
    map<bool, string> mapCote;
    Balle balle{(MAX_X+MIN_X)/2, (MAX_Y_BALL+MIN_Y_BALL)/2, (MAX_X+MIN_X)/2, (MAX_Y_BALL+MIN_Y_BALL)/2, 0, 0}; // No move

    while (true)
    {
        zmq::multipart_t msg;
        zmq::message_t name, command;

        if (msg.recv(serveur, ZMQ_NOBLOCK)) // non blocking receive
        {
            // Receive command from players
            assert(msg.size() == 2);

            name = msg.pop();
            command = msg.pop();

            string s_name(static_cast<const char *>(name.data()), name.size());
            string s_command(static_cast<const char *>(command.data()), command.size());
            sendlog("INFO", "GAME ENGINE", "commande " + s_name + " " + s_command);

            auto it = mapJoueur.find(s_name);

            // Manage players
            if (it != mapJoueur.end())
            {
                it->second.update(s_command);
            }
            else if (mapJoueur.size() < 2)
            {
                Joueur newjoueur;

                newjoueur.left = mapJoueur.size() % 2 == 0 ? true : false;
                newjoueur.y = 0;
                newjoueur.name = s_name;

                newjoueur.update(s_command);

                mapJoueur[s_name] = newjoueur;
                sendlog("INFO", "GAME ENGINE", "Add new player " + s_name);

                mapCote[newjoueur.left] = s_name;

                if (mapJoueur.size() == 2)
                {
                    // Games is starting => Random direction
                    while (balle.direction_x == 0)
                        balle.direction_x = dist(mt);
                    while (balle.direction_y == 0)
                        balle.direction_y = dist(mt);
                }
            }
            else
            {
                sendlog("INFO", "GAME ENGINE", "Cannot add new player " + s_name);
                zmq::multipart_t msg_update;

                add_str(msg_update, s_name);
                add_str(msg_update, CMD_ERROR);
                add_str(msg_update, "The room is full");
                msg_update.send(serveur);
            }
        }
        // else if (errno == ETERM)
        // {
        //     sendlog("INFO", "GAME ENGINE", "Players leaved");
        //     // Connexions closed ==> Remove players
        //     if (!mapJoueur.empty())
        //         mapJoueur.clear();
        //     if (!mapCote.empty())
        //         mapCote.clear();
        // }

        // Manage ball
        if (mapJoueur.size() == 2)
        {
            balle.update(mapJoueur[ mapCote[balle.pos_x < FIELD_WIDTH/2] ]);
            //sendlog("INFO", "GAME ENGINE", "Update ball : ("+to_string(balle.old_x)+", "+to_string(balle.old_y)+") => ("+to_string(balle.pos_x)+", "+to_string(balle.pos_y)+") - dir("+to_string(balle.direction_x)+", "+to_string(balle.direction_y)+")");
        }

        // Send object positions
        //sendlog("INFO", "GAME ENGINE", "Update screen");
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
            // Send ball
            add_str(msg_update, BALL_NAME);
            add_str(msg_update, to_string(balle.old_x));
            add_str(msg_update, to_string(balle.old_y));
            add_str(msg_update, to_string(balle.pos_x));
            add_str(msg_update, to_string(balle.pos_y));
            add_str(msg_update, to_string(balle.direction_x));
            add_str(msg_update, to_string(balle.direction_y));

            //sendlog("INFO", "GAME ENGINE", "Update screen " + joueur.first);
            msg_update.send(serveur);
            
            // wait 0.1s betwin each move
            usleep(100000);
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
        desc.add_options()("ip_server,ip", po::value<string>()->default_value("localhost"), "IP of the game server");

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
            string ip_server = vm["ip_server"].as<string>();
            client_name = vm["client"].as<string>();
            sendlog("INFO", client_name, "Start");
            client_graphique(ip_server);
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
