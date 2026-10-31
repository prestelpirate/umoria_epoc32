// EPOC32 (ER5) window-server front end for the Angband port.
// Draws an 80x24+ text grid in its own full-screen window with a fixed-pitch
// font, and provides an EPOC-style drop-down menu bar opened with the Menu key.
#include <e32base.h>
#include <e32keys.h>
#include <f32file.h>
#include <w32std.h>
#include <gdi.h>
#include <fbs.h>
#include "epoccons.h"

#define MAXCOLS 128
#define MAXROWS 64

static RWsSession gWs;
/* RWindowGroup/RWindow must be built with the session (their default
 * constructors leave the command buffer NULL -> KERN-EXEC 3 on Construct). */
static RWindowGroup *gGroupP = 0;
static RWindow *gWinP = 0;
#define gGroup (*gGroupP)
#define gWin (*gWinP)
static CWsScreenDevice *gDev = 0;
static CWindowGc *gGc = 0;
static CFont *gFont = 0;
static TBool gOpen = EFalse;
static TBool gGcActive = EFalse;
static TBool gMono = ETrue;
static TInt gCw = 8, gCh = 10, gAsc = 8, gCols = 80, gRows = 24, gOffX = 0, gOffY = 0;
static TSize gScreen;
static char gCells[MAXROWS][MAXCOLS];
static unsigned short gAttr[MAXROWS][MAXCOLS];
static TRgb gPal[256];
static TInt gShade = 28;
#define BG_RGB KRgbBlack
static TInt gCurX = 0, gCurY = 0;
static TBool gCurVis = EFalse, gCurDrawn = EFalse;
static TRequestStatus gEvStat, gRdStat;
static TBool gEvPending = EFalse, gRdPending = EFalse;
static TBool gInMenu = EFalse;
static CTrapCleanup *gCleanup = 0;
static TBool gFbs = EFalse;

// ---------------------------------------------------------------- drawing
static void GcOn()
{
	if (gGcActive) return;
	gGc->Activate(gWin);
	gGc->UseFont(gFont);
	gGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
	gGcActive = ETrue;
}

static void GcOff()
{
	if (!gGcActive) return;
	gGc->DiscardFont();
	gGc->Deactivate();
	gGcActive = EFalse;
}

static void CellColours(unsigned short a, TRgb &fg, TRgb &bg)
{
	fg = gPal[a & 0xFF];
	switch (a >> 8) {
	case 1:  bg = fg; break;
	case 2:  bg = gPal[gShade & 0xFF]; break;
	default: bg = BG_RGB; break;
	}
}

static void DrawRun(TInt x, TInt y, TInt n, TRgb fg, TRgb bg)
{
	gGc->SetPenColor(fg);
	gGc->SetBrushColor(bg);
	TInt px = gOffX + x * gCw, py = gOffY + y * gCh;
	if (gMono) {
		TPtrC p((const TText *)&gCells[y][x], n);
		gGc->DrawText(p, TRect(px, py, px + n * gCw, py + gCh), gAsc);
	} else {
		for (TInt i = 0; i < n; i++) {
			TPtrC p((const TText *)&gCells[y][x + i], 1);
			gGc->DrawText(p, TRect(px + i * gCw, py, px + (i + 1) * gCw, py + gCh),
				gAsc, CGraphicsContext::ECenter);
		}
	}
}

/* Draw cells, splitting into runs of the same colour */
static void DrawCells(TInt x, TInt y, TInt n, TBool inverse)
{
	if (y < 0 || y >= gRows || x >= gCols || n <= 0) return;
	if (x + n > gCols) n = gCols - x;
	TInt end = x + n;
	while (x < end) {
		unsigned short a = gAttr[y][x];
		TInt run = 1;
		while (x + run < end && gAttr[y][x + run] == a) run++;
		TRgb fg, bg;
		CellColours(a, fg, bg);
		if (inverse) {
			/* cursor: text in the background colour on a block of the text colour */
			TRgb t = fg; fg = bg; bg = t;
			if (fg == bg) { fg = KRgbBlack; bg = KRgbWhite; }
		}
		DrawRun(x, y, run, fg, bg);
		x += run;
	}
}

static void HideCursor()
{
	if (gCurDrawn) { GcOn(); DrawCells(gCurX, gCurY, 1, EFalse); gCurDrawn = EFalse; }
}

static void ShowCursor()
{
	if (gCurVis && !gCurDrawn && !gInMenu) { GcOn(); DrawCells(gCurX, gCurY, 1, ETrue); gCurDrawn = ETrue; }
}

static void PaintAll()
{
	GcOn();
	// margins outside the grid
	gGc->SetPenStyle(CGraphicsContext::ENullPen);
	gGc->SetBrushColor(BG_RGB);
	gGc->DrawRect(TRect(TPoint(0, 0), gScreen));
	gGc->SetPenStyle(CGraphicsContext::ESolidPen);
	for (TInt y = 0; y < gRows; y++) DrawCells(0, y, gCols, EFalse);
	gCurDrawn = EFalse;
}

static void HandleRedraw();
static void ServiceRedraws();

static void Flush()
{
	ShowCursor();
	GcOff();
	gWs.Flush();
	/* Angband can run for a long time without reading input (e.g. while
	 * loading its data files), so answer any pending redraw requests here
	 * too, or the window stays half-painted over the previous screen. */
	ServiceRedraws();
}

// ---------------------------------------------------------------- events
static void Arm()
{
	if (!gEvPending) { gWs.EventReady(&gEvStat); gEvPending = ETrue; }
	if (!gRdPending) { gWs.RedrawReady(&gRdStat); gRdPending = ETrue; }
}

static void PaintMenu();

static void ServiceRedraws()
{
	for (;;) {
		if (!gRdPending) { gWs.RedrawReady(&gRdStat); gRdPending = ETrue; }
		if (gRdStat == KRequestPending) return;
		User::WaitForAnyRequest();   /* consume this completion's signal */
		gRdPending = EFalse;
		HandleRedraw();
	}
}

static void HandleRedraw()
{
	TWsRedrawEvent r;
	gWs.GetRedraw(r);
	if (r.Handle() != (TUint32)&gWin) return;
	GcOff();
	gGc->Activate(gWin);
	gWin.BeginRedraw(r.Rect());
	gGcActive = ETrue;
	gGc->UseFont(gFont);
	gGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
	PaintAll();
	if (gInMenu) PaintMenu();
	gWin.EndRedraw();
	GcOff();
	gWs.Flush();
}

static TInt MapKey(const TKeyEvent &k)
{
	switch (k.iCode) {
	case EKeyUpArrow:    return ECON_UP;
	case EKeyDownArrow:  return ECON_DOWN;
	case EKeyLeftArrow:  return ECON_LEFT;
	case EKeyRightArrow: return ECON_RIGHT;
	case EKeyPageUp:     return ECON_PGUP;
	case EKeyPageDown:   return ECON_PGDN;
	case EKeyHome:       return ECON_HOME;
	case EKeyEnd:        return ECON_END;
	case EKeyDelete:     return ECON_DEL;
	case EKeyMenu:       return ECON_MENU;
	default: break;
	}
	if (k.iCode > 0 && k.iCode < 256) return (TInt)k.iCode;
	return 0;
}

// One pass of the event loop. Returns a key, or -1 if nothing (when !aWait).
static TInt Pump(TBool aWait)
{
	for (;;) {
		Arm();
		TBool any = (gEvStat != KRequestPending) || (gRdStat != KRequestPending);
		if (!any && !aWait) return -1;
		if (!any) Flush();
		User::WaitForAnyRequest();   // consumes the completion signal
		if (gRdStat != KRequestPending) {
			gRdPending = EFalse;
			HandleRedraw();
			continue;
		}
		if (gEvStat != KRequestPending) {
			gEvPending = EFalse;
			TWsEvent e;
			gWs.GetEvent(e);
			if (e.Type() == EEventKey) {
				TInt k = MapKey(*e.Key());
				if (k) return k;
			}
			continue;
		}
		// stray signal from elsewhere: loop
	}
}

// ---------------------------------------------------------------- menus
static const struct econ_menu *gMenus = 0;
static TInt gNMenus = 0, gSelMenu = 0, gSelItem = 0;
static TInt gBarH = 0, gItemH = 0;
static TInt gTitleX[16], gTitleW[16];
static TRect gPaneRect;

static TInt TextW(const char *s)
{
	TPtrC p((const TText *)s, User::StringLength((const TText *)s));
	return gFont->TextWidthInPixels(p);
}

static void DrawStr(const char *s, TInt x, TInt y, TInt w, TBool inv, TBool right)
{
	TPtrC p((const TText *)s, User::StringLength((const TText *)s));
	gGc->SetPenColor(inv ? KRgbWhite : KRgbBlack);
	gGc->SetBrushColor(inv ? KRgbBlack : KRgbWhite);
	gGc->DrawText(p, TRect(x, y, x + w, y + gItemH), (gItemH - gCh) / 2 + gAsc,
		right ? CGraphicsContext::ERight : CGraphicsContext::ELeft, 4);
}

static void LayoutBar()
{
	gBarH = gCh + 6;
	gItemH = gCh + 4;
	TInt x = 4;
	for (TInt i = 0; i < gNMenus && i < 16; i++) {
		gTitleW[i] = TextW(gMenus[i].title) + 16;
		gTitleX[i] = x;
		x += gTitleW[i];
	}
}

static TBool IsSep(TInt m, TInt i) { return gMenus[m].items[i].label == 0; }

static void PaintMenu()
{
	GcOn();
	TInt i;
	// bar
	gGc->SetPenColor(KRgbBlack);
	gGc->SetBrushColor(KRgbWhite);
	gGc->DrawRect(TRect(0, 0, gScreen.iWidth, gBarH));
	for (i = 0; i < gNMenus && i < 16; i++) {
		TPtrC p((const TText *)gMenus[i].title, User::StringLength((const TText *)gMenus[i].title));
		TBool sel = (i == gSelMenu);
		gGc->SetPenColor(sel ? KRgbWhite : KRgbBlack);
		gGc->SetBrushColor(sel ? KRgbBlack : KRgbWhite);
		gGc->DrawText(p, TRect(gTitleX[i], 2, gTitleX[i] + gTitleW[i], gBarH - 2),
			(gBarH - 4 - gCh) / 2 + gAsc, CGraphicsContext::ECenter);
	}
	// pane
	const struct econ_menu &m = gMenus[gSelMenu];
	TInt lw = 0, kw = 0;
	for (i = 0; i < m.count; i++) {
		if (IsSep(gSelMenu, i)) continue;
		TInt w = TextW(m.items[i].label); if (w > lw) lw = w;
		w = TextW(m.items[i].key); if (w > kw) kw = w;
	}
	TInt pw = lw + kw + 32, ph = 4;
	for (i = 0; i < m.count; i++) ph += IsSep(gSelMenu, i) ? 5 : gItemH;
	TInt px = gTitleX[gSelMenu];
	if (px + pw > gScreen.iWidth) px = gScreen.iWidth - pw;
	if (px < 0) px = 0;
	TInt py = gBarH - 1;
	if (py + ph > gScreen.iHeight) ph = gScreen.iHeight - py;
	gPaneRect = TRect(px, py, px + pw, py + ph);
	// shadow + frame
	gGc->SetPenStyle(CGraphicsContext::ENullPen);
	gGc->SetBrushColor(KRgbDarkGray);
	gGc->DrawRect(TRect(px + 3, py + 3, px + pw + 3, py + ph + 3));
	gGc->SetPenStyle(CGraphicsContext::ESolidPen);
	gGc->SetPenColor(KRgbBlack);
	gGc->SetBrushColor(KRgbWhite);
	gGc->DrawRect(gPaneRect);
	TInt y = py + 2;
	for (i = 0; i < m.count && y < py + ph - 2; i++) {
		if (IsSep(gSelMenu, i)) {
			gGc->SetPenColor(KRgbBlack);
			gGc->DrawLine(TPoint(px + 4, y + 2), TPoint(px + pw - 4, y + 2));
			y += 5;
			continue;
		}
		TBool sel = (i == gSelItem);
		DrawStr(m.items[i].label, px + 2, y, lw + 12, sel, EFalse);
		DrawStr(m.items[i].key, px + 2 + lw + 12, y, pw - 4 - lw - 12, sel, ETrue);
		y += gItemH;
	}
}

static void FirstItem()
{
	gSelItem = 0;
	while (gSelItem < gMenus[gSelMenu].count && IsSep(gSelMenu, gSelItem)) gSelItem++;
}

static void MoveItem(TInt d)
{
	TInt n = gMenus[gSelMenu].count;
	if (n <= 0) return;
	for (TInt tries = 0; tries < n; tries++) {
		gSelItem = (gSelItem + d + n) % n;
		if (!IsSep(gSelMenu, gSelItem)) return;
	}
}

extern "C" int econ_run_menu(const struct econ_menu *menus, int nmenus)
{
	if (!gOpen || nmenus <= 0) return -1;
	gMenus = menus; gNMenus = nmenus > 16 ? 16 : nmenus;
	if (gSelMenu >= gNMenus) gSelMenu = 0;
	LayoutBar();
	FirstItem();
	HideCursor();
	gInMenu = ETrue;
	TInt result = -1;
	for (;;) {
		PaintAll();
		PaintMenu();
		Flush();
		TInt k = Pump(ETrue);
		if (k == ECON_LEFT)  { gSelMenu = (gSelMenu + gNMenus - 1) % gNMenus; FirstItem(); }
		else if (k == ECON_RIGHT) { gSelMenu = (gSelMenu + 1) % gNMenus; FirstItem(); }
		else if (k == ECON_UP)    MoveItem(-1);
		else if (k == ECON_DOWN)  MoveItem(1);
		else if (k == ECON_HOME)  FirstItem();
		else if (k == ECON_END)   { gSelItem = 0; MoveItem(-1); }
		else if (k == 27 || k == ECON_MENU) break;
		else if (k == 13 || k == 10 || k == ' ') {
			if (gSelItem < gMenus[gSelMenu].count && !IsSep(gSelMenu, gSelItem))
				result = gSelMenu * 256 + gSelItem;
			break;
		}
	}
	gInMenu = EFalse;
	PaintAll();
	Flush();
	return result;
}

// ---------------------------------------------------------------- init
static TBool TryFont(const TDesC &aName, TInt aHeight)
{
	CFont *f = 0;
	if (gDev->GetNearestFontInPixels(f, TFontSpec(aName, aHeight)) != KErrNone || !f) return EFalse;
	TInt cw = f->MaxCharWidthInPixels(), ch = f->HeightInPixels();
	if (cw <= 0 || ch <= 0 || gScreen.iWidth / cw < 80 || gScreen.iHeight / ch < 24) {
		gDev->ReleaseFont(f);
		return EFalse;
	}
	gFont = f; gCw = cw; gCh = ch; gAsc = f->AscentInPixels();
	gMono = (f->TextWidthInPixels(_L("i")) == f->TextWidthInPixels(_L("W")));
	return ETrue;
}

static void ChooseFontL()
{
	static const TText *names[] = { _S("Courier"), _S("Courier New"), _S("Fixed"), _S("Swiss"), 0 };
	for (TInt h = 16; h >= 6 && !gFont; h--)
		for (TInt i = 0; names[i] && !gFont; i++)
			TryFont(TPtrC(names[i]), h);
	if (!gFont) {
		// nothing fits 80x24: take the smallest available and accept clipping
		User::LeaveIfError(gDev->GetNearestFontInPixels(gFont, TFontSpec(_L("Courier"), 6)));
		gCw = gFont->MaxCharWidthInPixels(); gCh = gFont->HeightInPixels();
		gAsc = gFont->AscentInPixels(); gMono = EFalse;
	}
}

static void InitL()
{
	/* Fonts live in the font & bitmap server; an exe (unlike an Eikon app)
	 * must connect to it itself before any CFont is used. */
	User::LeaveIfError(RFbsSession::Connect());
	gFbs = ETrue;
	User::LeaveIfError(gWs.Connect());
	gGroupP = new(ELeave) RWindowGroup(gWs);
	gWinP = new(ELeave) RWindow(gWs);
	gDev = new(ELeave) CWsScreenDevice(gWs);
	User::LeaveIfError(gDev->Construct());
	gScreen = gDev->SizeInPixels();
	User::LeaveIfError(gDev->CreateContext(gGc));
	User::LeaveIfError(gGroup.Construct((TUint32)&gGroup, ETrue));
	gGroup.SetName(_L("Angband"));
	gGroup.SetOrdinalPosition(0);
	User::LeaveIfError(gWin.Construct(gGroup, (TUint32)&gWin));
	gWin.SetExtent(TPoint(0, 0), gScreen);
	gWin.SetBackgroundColor(BG_RGB);
	gWin.Activate();
	ChooseFontL();
	gCols = gScreen.iWidth / gCw; if (gCols > MAXCOLS) gCols = MAXCOLS;
	gRows = gScreen.iHeight / gCh; if (gRows > MAXROWS) gRows = MAXROWS;
	gOffX = (gScreen.iWidth - gCols * gCw) / 2;
	gOffY = (gScreen.iHeight - gRows * gCh) / 2;
	Mem::Fill(gCells, sizeof(gCells), ' ');
	Mem::FillZ(gAttr, sizeof(gAttr));
	/* until Angband sends its colour table: 0 black, others white */
	for (TInt i = 0; i < 256; i++) gPal[i] = i ? KRgbWhite : KRgbBlack;
}

extern "C" int econ_init(int *cols, int *rows)
{
	if (!gCleanup) gCleanup = CTrapCleanup::New();
	TRAPD(err, InitL());
	if (err != KErrNone) return -1;
	gOpen = ETrue;
	*cols = gCols;
	*rows = gRows;
	/* Paint the whole window inside a redraw cycle so the window server
	 * treats it as valid straight away (no blank areas / desktop showing). */
	gGc->Activate(gWin);
	gWin.BeginRedraw();
	gGcActive = ETrue;
	gGc->UseFont(gFont);
	gGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
	PaintAll();
	gWin.EndRedraw();
	GcOff();
	Flush();
	return 0;
}

extern "C" void econ_close(void)
{
	if (!gOpen) return;
	GcOff();
	if (gEvPending) { gWs.EventReadyCancel(); User::WaitForRequest(gEvStat); gEvPending = EFalse; }
	if (gRdPending) { gWs.RedrawReadyCancel(); User::WaitForRequest(gRdStat); gRdPending = EFalse; }
	if (gFont) gDev->ReleaseFont(gFont);
	delete gGc; gGc = 0;
	if (gWinP) { gWin.Close(); delete gWinP; gWinP = 0; }
	if (gGroupP) { gGroup.Close(); delete gGroupP; gGroupP = 0; }
	delete gDev; gDev = 0;
	gWs.Close();
	if (gFbs) { RFbsSession::Disconnect(); gFbs = EFalse; }
	gOpen = EFalse;
}

// ---------------------------------------------------------------- C API
extern "C" void econ_palette(int index, int r, int g, int b)
{
	if (index >= 0 && index < 256) gPal[index] = TRgb(r, g, b);
}

extern "C" void econ_set_shade(int index) { gShade = index; }

extern "C" void econ_text(int x, int y, const char *s, int n, int attr)
{
	if (!gOpen || y < 0 || y >= gRows || x < 0 || x >= gCols) return;
	if (x + n > gCols) n = gCols - x;
	if (gCurDrawn && gCurY == y && gCurX >= x && gCurX < x + n) gCurDrawn = EFalse;
	Mem::Copy(&gCells[y][x], s, n);
	for (TInt i = 0; i < n; i++) gAttr[y][x + i] = (unsigned short)attr;
	GcOn();
	DrawCells(x, y, n, EFalse);
}

extern "C" void econ_wipe(int x, int y, int n)
{
	if (!gOpen || y < 0 || y >= gRows || x < 0 || x >= gCols) return;
	if (x + n > gCols) n = gCols - x;
	if (gCurDrawn && gCurY == y && gCurX >= x && gCurX < x + n) gCurDrawn = EFalse;
	Mem::Fill(&gCells[y][x], n, ' ');
	for (TInt i = 0; i < n; i++) gAttr[y][x + i] = 1;
	GcOn();
	DrawCells(x, y, n, EFalse);
}

extern "C" void econ_clear(void)
{
	if (!gOpen) return;
	Mem::Fill(gCells, sizeof(gCells), ' ');
	for (TInt y = 0; y < MAXROWS; y++) for (TInt x = 0; x < MAXCOLS; x++) gAttr[y][x] = 1;
	PaintAll();
}

extern "C" void econ_cursor_pos(int x, int y)
{
	if (!gOpen) return;
	if (x == gCurX && y == gCurY) return;
	HideCursor();
	gCurX = x; gCurY = y;
}

extern "C" void econ_cursor(int visible)
{
	if (!gOpen) return;
	if (!visible) HideCursor();
	gCurVis = visible ? ETrue : EFalse;
}

extern "C" void econ_flush(void) { if (gOpen) Flush(); }

extern "C" int econ_getkey(int wait)
{
	if (!gOpen) return -1;
	return Pump(wait ? ETrue : EFalse);
}

extern "C" void econ_sleep_ms(int ms)
{
	if (gOpen) Flush();
	if (ms > 0) User::After(ms * 1000);
}

extern "C" void econ_beep(void) { }

// ---------------------------------------------------------------- crash tracing (see compat/trace.c)
extern "C" {
extern unsigned int epoc_tr_shadow[256];
extern int epoc_tr_depth;
extern unsigned int epoc_tr_ring[64];
extern unsigned int epoc_tr_ringpos;
extern unsigned int epoc_tr_minsp;
extern unsigned int epoc_tr_mainaddr;
void epoc_trace_init(void);
}
static unsigned int gStackTop = 0;

static void CrashHandler(TExcType aType)
{
	RFs fs;
	if (fs.Connect() == KErrNone) {
		TFileName name = RProcess().FileName();
		TInt slash = name.LocateReverse('\\');
		name.SetLength(slash + 1);
		name.Append(_L("trace.txt"));
		RFile f;
		if (f.Replace(fs, name, EFileWrite) == KErrNone) {
			TBuf8<128> l;
			unsigned int sp = (unsigned int)&sp;
			l.Format(_L8("exc=%d depth=%d main=%08x sptop=%08x minsp=%08x sp=%08x\r\n"),
				(TInt)aType, epoc_tr_depth, epoc_tr_mainaddr, gStackTop, epoc_tr_minsp, sp);
			f.Write(l);
			f.Write(_L8("stack:\r\n"));
			TInt d = epoc_tr_depth < 256 ? epoc_tr_depth : 256;
			for (TInt i = d - 1; i >= 0; i--) {
				l.Format(_L8("S %08x\r\n"), epoc_tr_shadow[i]);
				f.Write(l);
			}
			f.Write(_L8("recent:\r\n"));
			for (TUint j = 0; j < 64 && j < epoc_tr_ringpos; j++) {
				l.Format(_L8("R %08x\r\n"), epoc_tr_ring[(epoc_tr_ringpos - 1 - j) % 64]);
				f.Write(l);
			}
			f.Close();
		}
		fs.Close();
	}
	User::Panic(_L("ANGCRASH"), (TInt)aType);
}

extern "C" void econ_install_crash_handler(void)
{
	unsigned int sp = (unsigned int)&sp;
	gStackTop = sp;
	epoc_trace_init();
	RThread().SetExceptionHandler(CrashHandler, 0xFFFFFFFF);
}

extern "C" void econ_exe_dir(char *buf, int len)
{
	TFileName f = RProcess().FileName();
	TInt slash = f.LocateReverse('\\');
	TPtrC dp = f.Left(slash + 1);
	int i, n = dp.Length();
	if (n > len - 1) n = len - 1;
	for (i = 0; i < n; i++) {
		char c = (char)dp[i];
		buf[i] = (c == '\\') ? '/' : c;
	}
	buf[n] = 0;
}
