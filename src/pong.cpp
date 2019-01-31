//
// Coding Dojo
//

#include <bits/stdc++.h>
#include <boost/program_options.hpp>
#include <ncurses.h>
#include <zmq.h>

using namespace std;

//
// DEMO
//

#define FIELD_WIDTH 79
#define FIELD_HEIGHT 24
#define PADDLE_SIZE 5

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

    attron(COLOR_PAIR(4));
    mvvline(y, x, ACS_VLINE, PADDLE_SIZE);
    attron(COLOR_PAIR(4));
}

void demo()
{
    init_screen();
    draw_field();
    draw_paddle(true, 0);
    draw_paddle(false, 1);

    getch();
}

// END OF DEMO

int main(int argc, char *argv[])
{
    try
    {
        //
        // Define and parse the program options
        //

        namespace po = boost::program_options;
        po::options_description desc("Options");
        desc.add_options()("help,h", "Print help");
        desc.add_options()("value,v", po::value<string>(), "name");
        desc.add_options()("demo", "demo");

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
        if (vm.count("demo"))
        {
            demo();
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
