#include <bits/stdc++.h>
#include <cmath>
#include <ncurses.h>
#include <sys/time.h>
#include <unistd.h>

void reset_screen()
{
    delwin(stdscr);
    endwin();
    refresh();

    puts("Bye.");
}

int main()
{
    initscr();            // start ncurses
    atexit(reset_screen); // vacuum

    if (!has_colors())
    {
        exit(2); // we want colors
    }
    start_color(); // start colors

    if (COLOR_PAIRS < 16)
    {
        exit(2); // at least 16 pairs
    }

    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_BLACK, COLOR_YELLOW);

    cbreak();             // line buffering disabled
    keypad(stdscr, true); // enables F_keys
    noecho();             // disables echoing of characters
    curs_set(0);          // hide cursor

    int x, y;
    getmaxyx(stdscr, y, x);

    attron(COLOR_PAIR(3) | A_BLINK);
    mvaddstr(y / 2, x / 2 - 10, "WAITING FOR PLAYER...");
    attroff(COLOR_PAIR(3) | A_BLINK);

    getch(); // blocking key input
    erase();

    int last_key = -1;
    int angle = 0;
    int radius = 5;
    int speed = 5;

    nodelay(stdscr, 1); // non blocking key input
    while (true)
    {
        int c = getch();

        if (c == 27)
            break;
        if (c == 'q')
            break;

        switch (c)
        {
        case KEY_RESIZE:
            getmaxyx(stdscr, y, x);
            break;
        case KEY_DOWN:
            speed = speed > -10 ? speed - 1 : -10;
            break;
        case KEY_UP:
            speed = speed < 10 ? speed + 1 : 10;
            break;
        case KEY_RIGHT:
            radius = radius < 10 ? radius + 1 : 10;
            break;
        case KEY_LEFT:
            radius = radius > 1 ? radius - 1 : 1;
            break;
        }

        // clear screan
        erase();

        // draw the XY axis
        mvvline(0, x / 2, ACS_VLINE, y);
        mvhline(y / 2, 0, ACS_HLINE, x);

        // print CROSS at center of the screen
        attron(COLOR_PAIR(1));
        mvaddstr(y / 2, x / 2 - 2, "CROSS");
        attroff(COLOR_PAIR(1));

        // print screen size
        attron(COLOR_PAIR(2));
        mvprintw(y - 2, 2, "x,y = %d,%d", x, y);
        attroff(COLOR_PAIR(2));

        // print the last key
        if (c != -1)
        {
            last_key = c;
        }
        if (last_key >= 32 && last_key <= 126)
        {
            mvprintw(2, 2, "last key: %c 0x%x", last_key, last_key);
        }
        else if (last_key != -1)
        {
            mvprintw(2, 2, "last key: 0x%x 0%03o", last_key, last_key, last_key);
        }

        // draw the radar pursuit
        int bx, by;
        bx = (int)(x / 2 + (x / 20.) * radius * cos(angle * M_PI / 180.));
        by = (int)(y / 2 - (y / 20.) * radius * sin(angle * M_PI / 180.));

        if (mvinch(by, bx) != ' ')
        {
            // let's add some alea when hiting the cross
            if (rand() % 2)
            {
                // bounce the ball
                speed = -speed;
            }
            else
            {
                // accelerate or decelerate the ball
                int r = rand() % 3;
                if (r == 1 && speed < 10)
                    speed++;
                else if (r == 2 && speed > -10)
                    speed--;
            }
        }
        mvaddch(by, bx, '@');

        // print the parameters
        mvprintw(0, x - 10, "a=%3d r=%2d", angle, radius);
        mvprintw(1, x - 9, "speed=%3d", speed);

        struct timeval tv;
        gettimeofday(&tv, nullptr);
        mvprintw(1, 2, "%ld.%06ld", tv.tv_sec, tv.tv_usec);

        // update
        angle = (angle + speed + 360) % 360;

        //
        usleep(40000); // 40 ms ~ 25 Hz
    }

    return 0;
}
