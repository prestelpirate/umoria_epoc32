// curses subset + EPOC start-up glue for umoria.  The game draws into
// stdscr; refresh() pushes changed rows to the window-server front end
// (epocwin.cpp, shared with the Angband port).
#include <string.h>
#include <unistd.h>
#include "epoc_curses.h"
#include "epoccons.h"
#include "estring.h"

epoc_null_stream cerr;

static WINDOW scr_main, scr_cur, scr_extra[4];
static int extra_used = 0;
WINDOW *stdscr = &scr_main;
WINDOW *curscr = &scr_cur;
int LINES = 24, COLS = 80;
static int timeout_ms = -1;
static int opened = 0;

static void wclear_buf(WINDOW *w)
{
    memset(w->c, ' ', sizeof(w->c));
    memset(w->dirty, 1, sizeof(w->dirty));
    w->y = w->x = 0;
}

extern "C++" void epocMainInit()
{
    // the game opens "data/..." and "scores.dat" relative to the current
    // directory: make that the folder the exe lives in
    char dir[256];
    econ_install_crash_handler();
    econ_exe_dir(dir, sizeof(dir));
    chdir(dir);
    initscr();
}

WINDOW *initscr(void)
{
    if (!opened) {
        int cols = 80, rows = 24;
        if (econ_init(&cols, &rows) == 0) {
            opened = 1;
            COLS = cols > CURS_MAXCOLS ? CURS_MAXCOLS : cols;
            LINES = rows > CURS_MAXROWS ? CURS_MAXROWS : rows;
            for (int i = 0; i < 256; i++) econ_palette(i, 255, 255, 255);
            econ_palette(0, 0, 0, 0);
            econ_cursor(1);
        }
        wclear_buf(stdscr);
    }
    return stdscr;
}

int endwin(void) { refresh(); return OK; }

// Fatal message (replaces printf to a console that EPOC does not have)
void epocShowFatal(const char *msg)
{
    if (!opened) return;
    clear();
    mvaddstr(0, 0, "Umoria stopped:");
    mvaddstr(2, 0, msg ? msg : "");
    mvaddstr(4, 0, "[Press any key]");
    refresh();
    econ_getkey(1);
}

WINDOW *newwin(int, int, int, int)
{
    if (extra_used >= 4) return 0;
    WINDOW *w = &scr_extra[extra_used++];
    wclear_buf(w);
    return w;
}

int overwrite(const WINDOW *src, WINDOW *dst)
{
    memcpy(dst->c, src->c, sizeof(dst->c));
    memset(dst->dirty, 1, sizeof(dst->dirty));
    return OK;
}

int touchwin(WINDOW *w) { memset(w->dirty, 1, sizeof(w->dirty)); return OK; }

int wrefresh(WINDOW *w)
{
    if (w == curscr) { touchwin(stdscr); w = stdscr; }
    if (w != stdscr) return OK;
    for (int y = 0; y < LINES; y++) {
        if (!w->dirty[y]) continue;
        char row[CURS_MAXCOLS];
        for (int x = 0; x < COLS; x++) row[x] = (char) (w->c[y][x] & 0x7F); // high bit = standout
        econ_text(0, y, row, COLS, 1);
        w->dirty[y] = 0;
    }
    econ_cursor_pos(w->x, w->y);
    econ_flush();
    return OK;
}

int refresh(void) { return wrefresh(stdscr); }

int move(int y, int x)
{
    if (y < 0 || y >= LINES || x < 0 || x >= COLS) return ERR;
    stdscr->y = y; stdscr->x = x;
    return OK;
}

int addch(int ch)
{
    WINDOW *w = stdscr;
    if (ch == '\n') { clrtoeol(); if (w->y < LINES - 1) w->y++; w->x = 0; return OK; }
    if (w->x >= COLS) return ERR;
    w->c[w->y][w->x] = (char) ch;
    w->dirty[w->y] = 1;
    if (++w->x >= COLS) { w->x = COLS - 1; return OK; }
    return OK;
}

int addstr(const char *s) { while (*s) if (addch((unsigned char) *s++) == ERR) return ERR; return OK; }
int mvaddch(int y, int x, int ch) { return move(y, x) == ERR ? ERR : addch(ch); }
int mvaddstr(int y, int x, const char *s) { return move(y, x) == ERR ? ERR : addstr(s); }

int clrtoeol(void)
{
    WINDOW *w = stdscr;
    memset(&w->c[w->y][w->x], ' ', COLS - w->x);
    w->dirty[w->y] = 1;
    return OK;
}

int clrtobot(void)
{
    clrtoeol();
    for (int y = stdscr->y + 1; y < LINES; y++) { memset(stdscr->c[y], ' ', COLS); stdscr->dirty[y] = 1; }
    return OK;
}

int clear(void) { wclear_buf(stdscr); return OK; }

int mvcur(int, int, int ny, int nx) { move(ny, nx); return OK; }

void timeout(int ms) { timeout_ms = ms; }

static int map_key(int k)
{
    // the original keyset moves with the digit keys
    switch (k) {
    case ECON_UP: return '8';
    case ECON_DOWN: return '2';
    case ECON_LEFT: return '4';
    case ECON_RIGHT: return '6';
    case ECON_HOME: return '7';
    case ECON_PGUP: return '9';
    case ECON_END: return '1';
    case ECON_PGDN: return '3';
    case ECON_DEL: return 127;
    case ECON_MENU: return 27;
    case 10: return 13;
    default: return k < 256 ? k : ERR;
    }
}

int getch(void)
{
    refresh();
    int k;
    if (timeout_ms < 0) {
        do k = map_key(econ_getkey(1)); while (k == ERR);
        return k;
    }
    k = econ_getkey(0);
    if (k < 0 && timeout_ms > 0) { econ_sleep_ms(timeout_ms); k = econ_getkey(0); }
    return k < 0 ? ERR : map_key(k);
}
