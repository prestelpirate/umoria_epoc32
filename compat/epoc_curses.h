/* Tiny curses subset for umoria on EPOC, drawn through epoccons.h */
#ifndef EPOC_CURSES_H
#define EPOC_CURSES_H
#define ERR (-1)
#define OK 0
#define CURS_MAXROWS 64
#define CURS_MAXCOLS 128

typedef struct {
    char c[CURS_MAXROWS][CURS_MAXCOLS];
    unsigned char dirty[CURS_MAXROWS];
    int y, x;
} WINDOW;

extern WINDOW *stdscr, *curscr;
extern int LINES, COLS;

WINDOW *initscr(void);
int endwin(void);
WINDOW *newwin(int lines, int cols, int y, int x);
int overwrite(const WINDOW *src, WINDOW *dst);
int touchwin(WINDOW *w);
int wrefresh(WINDOW *w);
int refresh(void);
int move(int y, int x);
int addch(int ch);
int addstr(const char *s);
int mvaddch(int y, int x, int ch);
int mvaddstr(int y, int x, const char *s);
int clrtoeol(void);
int clrtobot(void);
int clear(void);
int getch(void);
void timeout(int ms);
int mvcur(int oy, int ox, int ny, int nx);
#define getyx(w, yy, xx) ((yy) = (w)->y, (xx) = (w)->x)
#define raw() 0
#define cbreak() 0
#define noecho() 0
#define echo() 0
#define nonl() 0
#define keypad(w, b) 0
#define curs_set(v) 0
#define standout() 0
#define standend() 0
#endif
