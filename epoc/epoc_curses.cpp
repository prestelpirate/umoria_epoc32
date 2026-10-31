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
static unsigned char cur_attr = EC_WHITE;

void epoc_set_attr(int colour) { cur_attr = (unsigned char) colour; }

/* Palette: Angband-style colours on black */
static const unsigned char palette[EC_MAX][3] = {
    {0, 0, 0}, {255, 255, 255}, {175, 175, 175}, {100, 100, 100},
    {220, 30, 30}, {255, 140, 0}, {255, 255, 60}, {0, 160, 60},
    {80, 255, 80}, {50, 90, 255}, {80, 200, 255}, {150, 100, 40},
    {210, 160, 90}, {180, 60, 220}, {255, 120, 200}, {0, 220, 220},
};

/* Monster letters (classic coloured-moria style) */
static unsigned char monster_colour(char ch)
{
    switch (ch) {
    case 'a': return EC_ORANGE;     case 'b': return EC_UMBER;
    case 'c': return EC_YELLOW;     case 'd': return EC_LIGHTGREEN;
    case 'e': return EC_BLUE;       case 'f': return EC_LIGHTUMBER;
    case 'g': return EC_GREY;       case 'h': return EC_LIGHTUMBER;
    case 'i': return EC_LIGHTGREEN; case 'j': return EC_YELLOW;
    case 'k': return EC_GREEN;      case 'l': return EC_UMBER;
    case 'm': return EC_LIGHTBLUE;  case 'n': return EC_GREEN;
    case 'o': return EC_UMBER;      case 'p': return EC_WHITE;
    case 'q': return EC_LIGHTUMBER; case 'r': return EC_UMBER;
    case 's': return EC_WHITE;      case 't': return EC_LIGHTUMBER;
    case 'u': return EC_RED;        case 'v': return EC_CYAN;
    case 'w': return EC_PINK;       case 'y': return EC_YELLOW;
    case 'z': return EC_GREY;
    case 'A': return EC_LIGHTBLUE;  case 'B': return EC_RED;
    case 'C': return EC_UMBER;      case 'D': return EC_RED;
    case 'E': return EC_LIGHTBLUE;  case 'F': return EC_GREEN;
    case 'G': return EC_WHITE;      case 'H': return EC_ORANGE;
    case 'I': return EC_LIGHTGREEN; case 'J': return EC_GREEN;
    case 'K': return EC_RED;        case 'L': return EC_VIOLET;
    case 'M': return EC_GREY;       case 'O': return EC_UMBER;
    case 'P': return EC_LIGHTUMBER; case 'Q': return EC_VIOLET;
    case 'R': return EC_GREEN;      case 'S': return EC_GREY;
    case 'T': return EC_GREEN;      case 'U': return EC_ORANGE;
    case 'V': return EC_VIOLET;     case 'W': return EC_GREY;
    case 'X': return EC_UMBER;      case 'Y': return EC_WHITE;
    case 'Z': return EC_LIGHTUMBER;
    default: return EC_WHITE;
    }
}

int epoc_symbol_colour(char ch, int is_monster, int lit)
{
    if (ch == '@') return EC_YELLOW;
    if (is_monster) return monster_colour(ch);
    switch (ch) {
    case '.': return lit ? EC_GREY : EC_DARKGREY;
    case '#': return EC_GREY;
    case '%': return EC_UMBER;
    case '*': return EC_ORANGE;
    case ':': return EC_UMBER;
    case '+': case '\'': return EC_LIGHTUMBER;
    case '<': case '>': return EC_WHITE;
    case '^': return EC_RED;
    case ';': return EC_LIGHTBLUE;
    case '$': return EC_YELLOW;
    case '!': return EC_LIGHTBLUE;
    case '?': return EC_WHITE;
    case '=': return EC_RED;
    case '"': return EC_ORANGE;
    case '_': return EC_LIGHTUMBER;
    case '-': return EC_LIGHTGREEN;
    case ',': return EC_UMBER;
    case '~': return EC_YELLOW;
    case '&': return EC_LIGHTUMBER;
    case '{': case '}': return EC_UMBER;
    case '|': case '/': case '\\': return EC_GREY;
    case '(': case ')': case '[': case ']': return EC_GREY;
    default:
        if (ch >= '1' && ch <= '9') return EC_LIGHTGREEN; /* town store entrances */
        return EC_WHITE;
    }
}
static int opened = 0;

static void wclear_buf(WINDOW *w)
{
    memset(w->c, ' ', sizeof(w->c));
    memset(w->a, EC_WHITE, sizeof(w->a));
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
            for (int i = 0; i < EC_MAX; i++) econ_palette(i, palette[i][0], palette[i][1], palette[i][2]);
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
    memcpy(dst->a, src->a, sizeof(dst->a));
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
        // one draw call per run of the same colour
        for (int x = 0; x < COLS; ) {
            int run = 1;
            while (x + run < COLS && w->a[y][x + run] == w->a[y][x]) run++;
            econ_text(x, y, row + x, run, w->a[y][x]);
            x += run;
        }
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
    w->a[w->y][w->x] = cur_attr;
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
    memset(&w->a[w->y][w->x], EC_WHITE, COLS - w->x);
    w->dirty[w->y] = 1;
    return OK;
}

int clrtobot(void)
{
    clrtoeol();
    for (int y = stdscr->y + 1; y < LINES; y++) { memset(stdscr->c[y], ' ', COLS); memset(stdscr->a[y], EC_WHITE, COLS); stdscr->dirty[y] = 1; }
    return OK;
}

int clear(void) { wclear_buf(stdscr); return OK; }

int mvcur(int, int, int ny, int nx) { move(ny, nx); return OK; }

void timeout(int ms) { timeout_ms = ms; }

/* ------------------------------------------------------------ menus
 * Drop-down menus on the Menu key, built from umoria's own command
 * summaries (data/help.txt = original keys, data/rl_help.txt = roguelike).
 * Choosing an item types that command's key. */
namespace config { namespace options { extern bool use_roguelike_keys; } }

#define K_CTRL(c) ((c) & 0x1F)
struct MenuCmd { const char *label; int key; int rl_key; };  /* label 0 = separator; rl_key 0 = n/a */
struct MenuDef { const char *title; const MenuCmd *cmds; int count; };

static const MenuCmd m_file[] = {
    {"Save and quit", K_CTRL('X'), K_CTRL('X')},
    {"View scoreboard", 'V', 'V'},
    {0, 0, 0},
    {"Quit (kill character)", K_CTRL('K'), 'Q'},
};
static const MenuCmd m_items[] = {
    {"Inventory list", 'i', 'i'},
    {"Equipment list", 'e', 'e'},
    {"Wear/wield an item", 'w', 'w'},
    {"Take off an item", 't', 'T'},
    {"Exchange weapon", 'x', 'X'},
    {"Drop an item", 'd', 'd'},
    {"Inscribe an object", '{', '{'},
    {0, 0, 0},
    {"Eat some food", 'E', 'E'},
    {"Quaff a potion", 'q', 'q'},
    {"Read a scroll", 'r', 'r'},
    {"Aim and fire a wand", 'a', 'z'},
    {"Use a staff", 'u', 'Z'},
    {"Fill lamp with oil", 'F', 'F'},
    {"Fire/throw an item", 'f', 't'},
};
static const MenuCmd m_action[] = {
    {"Open a door/chest", 'o', 'o'},
    {"Close a door", 'c', 'c'},
    {"Disarm a trap/chest", 'D', 'D'},
    {"Bash item or monster", 'B', 'f'},
    {"Tunnel", 'T', 0},
    {"Jam/spike a door", 'j', 'S'},
    {0, 0, 0},
    {"Search for traps/doors", 's', 's'},
    {"Search mode", 'S', '#'},
    {"Rest", 'R', 'R'},
    {"Look around", 'l', 'x'},
    {0, 0, 0},
    {"Go up staircase", '<', '<'},
    {"Go down staircase", '>', '>'},
    {"Move without pickup", '-', '-'},
};
static const MenuCmd m_magic[] = {
    {"Cast a magic spell", 'm', 'm'},
    {"Pray", 'p', 'p'},
    {"Browse a book", 'b', 'P'},
    {"Gain new spells", 'G', 'G'},
};
static const MenuCmd m_info[] = {
    {"Character description", 'C', 'C'},
    {"Map (reduced size)", 'M', 'M'},
    {"Locate with map", 'L', 'W'},
    {"Identify a character", '/', '/'},
    {"Repeat last message", K_CTRL('P'), K_CTRL('P')},
    {"Version info", 'v', 'v'},
};
static const MenuCmd m_options[] = {
    {"Set options", '=', '='},
    {"Redraw the screen", K_CTRL('R'), K_CTRL('R')},
};
static const MenuCmd m_help[] = {
    {"Command summary", '?', '?'},
};
#define NCMD(a) ((int) (sizeof(a) / sizeof((a)[0])))
static const MenuDef menu_defs[] = {
    {"File", m_file, NCMD(m_file)},
    {"Items", m_items, NCMD(m_items)},
    {"Action", m_action, NCMD(m_action)},
    {"Magic", m_magic, NCMD(m_magic)},
    {"Info", m_info, NCMD(m_info)},
    {"Options", m_options, NCMD(m_options)},
    {"Help", m_help, NCMD(m_help)},
};
#define NMENUS NCMD(menu_defs)

static int pending_key = -1;

static void key_text(char *buf, int k)
{
    if (k > 0 && k < 32) { strcpy(buf, "Ctrl+"); buf[5] = (char) (k + 64); buf[6] = 0; }
    else { buf[0] = (char) k; buf[1] = 0; }
}

static void run_menus()
{
    static struct econ_menu menus[NMENUS];
    static struct econ_menu_item items[NMENUS][20];
    static int keys[NMENUS][20];
    bool rl = config::options::use_roguelike_keys;
    for (int m = 0; m < NMENUS; m++) {
        int n = 0;
        for (int i = 0; i < menu_defs[m].count && n < 20; i++) {
            const MenuCmd &cmd = menu_defs[m].cmds[i];
            int k = rl ? cmd.rl_key : cmd.key;
            if (cmd.label && !k) continue;              /* no roguelike equivalent */
            items[m][n].label = cmd.label;
            if (cmd.label) key_text(items[m][n].key, k); else items[m][n].key[0] = 0;
            keys[m][n] = k;
            n++;
        }
        menus[m].title = menu_defs[m].title;
        menus[m].count = n;
        menus[m].items = items[m];
    }
    int r = econ_run_menu(menus, NMENUS);
    touchwin(stdscr);   /* repaint what the menu covered */
    refresh();
    if (r >= 0) pending_key = keys[r / 256][r % 256];
}

static int map_key(int k)
{
    // the original keyset moves with the digit keys
    switch (k) {
    case ECON_UP: return config::options::use_roguelike_keys ? 'k' : '8';
    case ECON_DOWN: return config::options::use_roguelike_keys ? 'j' : '2';
    case ECON_LEFT: return config::options::use_roguelike_keys ? 'h' : '4';
    case ECON_RIGHT: return config::options::use_roguelike_keys ? 'l' : '6';
    case ECON_HOME: return config::options::use_roguelike_keys ? 'y' : '7';
    case ECON_PGUP: return config::options::use_roguelike_keys ? 'u' : '9';
    case ECON_END: return config::options::use_roguelike_keys ? 'b' : '1';
    case ECON_PGDN: return config::options::use_roguelike_keys ? 'n' : '3';
    case ECON_DEL: return 127;
    case ECON_MENU: run_menus(); return pending_key >= 0 ? -2 : ERR;
    case 10: return 13;
    default: return k < 256 ? k : ERR;
    }
}

int getch(void)
{
    refresh();
    int k;
    if (pending_key >= 0) { k = pending_key; pending_key = -1; return k; }
    if (timeout_ms < 0) {
        for (;;) {
            k = map_key(econ_getkey(1));
            if (k == -2) { k = pending_key; pending_key = -1; return k; }  /* menu chose a command */
            if (k != ERR) return k;
        }
    }
    k = econ_getkey(0);
    if (k < 0 && timeout_ms > 0) { econ_sleep_ms(timeout_ms); k = econ_getkey(0); }
    if (k < 0) return ERR;
    k = map_key(k);
    if (k == -2) { k = pending_key; pending_key = -1; }
    return k;
}
