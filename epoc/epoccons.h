/* C interface to the EPOC32 window-server front end (epocwin.cpp) */
#ifndef INCLUDED_EPOCCONS_H
#define INCLUDED_EPOCCONS_H
#ifdef __cplusplus
extern "C" {
#endif
/* Key codes returned by econ_getkey (besides plain characters) */
#define ECON_UP     0x10001
#define ECON_DOWN   0x10002
#define ECON_LEFT   0x10003
#define ECON_RIGHT  0x10004
#define ECON_PGUP   0x10005
#define ECON_PGDN   0x10006
#define ECON_HOME   0x10007
#define ECON_END    0x10008
#define ECON_DEL    0x10009
#define ECON_MENU   0x1000A

/* Drop-down menu description (built by main-epoc.c) */
struct econ_menu_item {
	const char *label;    /* NULL label = separator */
	char key[12];         /* shortcut text shown on the right */
};
struct econ_menu {
	const char *title;
	int count;
	struct econ_menu_item *items;
};

int  econ_init(int *cols, int *rows);   /* window + font; returns grid size */
void econ_close(void);
/* draw n chars at cell; attr = colour index + 256 * background mode
 * (0 = black, 1 = same as text colour, 2 = shade colour) */
void econ_text(int x, int y, const char *s, int n, int attr);
void econ_palette(int index, int r, int g, int b);  /* Angband colour table */
void econ_set_shade(int index);                     /* colour used for bg mode 2 */
void econ_wipe(int x, int y, int n);    /* blank n cells */
void econ_clear(void);
void econ_cursor_pos(int x, int y);
void econ_cursor(int visible);
void econ_flush(void);
int  econ_getkey(int wait);             /* -1 if none and !wait */
void econ_sleep_ms(int ms);
void econ_beep(void);
void econ_exe_dir(char *buf, int len);  /* e.g. "D:/System/Apps/Angband/" */
void econ_install_crash_handler(void);  /* trace.txt on a CPU fault */
/* Run the menu bar modally. Returns menu*256+item, or -1 if cancelled. */
int  econ_run_menu(const struct econ_menu *menus, int nmenus);
#ifdef __cplusplus
}
#endif
#endif
