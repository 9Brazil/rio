enum
{
	Qdir,				/* /dev for this window */
	Qcons,
	Qconsctl,
	Qcursor,
	Qwdir,
	Qwinid,
	Qwinname,
	Qkbdin,
	Qlabel,
	Qmouse,
	Qnew,
	Qscreen,
	Qsnarf,
	Qtext,
	Qwctl,
	Qwindow,
	Qwsys,				/* directory of window directories */
	Qwsysdir,			/* window directory, child of wsys */

	QMAX,
};

enum
{
	Kscrolloneup	= KF|0x20,
	Kscrollonedown	= KF|0x21,

	Ksoh		= 0x01,
	Kenq		= 0x05,
	Kack		= 0x06,
	Kbs		= 0x08,
	Knack		= 0x15,
	Ketb		= 0x17,
	Kdel		= 0x7F,
	Kesc		= 0x1B,
};

#define STACK			8192
#define MAXSNARF		100*1024

typedef struct Consreadmesg	Consreadmesg;
typedef struct Conswritemesg	Conswritemesg;
typedef struct Stringpair	Stringpair;
typedef struct Dirtab		Dirtab;
typedef struct Fid		Fid;
typedef struct Filsys		Filsys;
typedef struct Mouseinfo	Mouseinfo;
typedef struct Mousereadmesg	Mousereadmesg;
typedef struct Mousestate	Mousestate;
typedef struct Ref		Ref;
typedef struct Timer		Timer;
typedef struct Wctlmesg		Wctlmesg;
typedef struct Window		Window;
typedef struct Xfid		Xfid;

enum
{
	Selborder	= 4,		/* border of selected window */
	Unselborder	= 1,		/* border of unselected window */
};

enum
{
	Scrollwid	= 12,		/* width of scroll bar */
	Scrollgap	= 4,		/* gap right of scroll bar */
};

enum
{
	TRUE		= 1,
	FALSE		= 0,
};

enum
{
	DDodgerblue	= 0x3399FFFF,
	DGrey60		= 0x999999FF,
	DNavajowhite	= 0xFEDBA9FF,
	DNavy		= 0x000080FF,
	DPinetree	= 0x11120FFF,
	DPurered	= 0xDD0000FF,
	DSilver		= 0xC0C0C0FF,
	DTeal		= 0x008080FF,
};

/* rio background */
#define CLBACKGROUND		DTeal
/* window background */
#define CLWINDOW		DBlack
/* text color */
#define CLWINDOWTEXT11		DWhite
#define CLWINDOWTEXT01		DWhite
#define CLWINDOWTEXT10		DSilver
#define CLWINDOWTEXT00		DSilver
/* highlight color */
#define CLHIGHLIGHT11		DNavy
#define CLHIGHLIGHT01		DDodgerblue
#define CLHIGHLIGHT10		DNavajowhite
#define CLHIGHLIGHT00		DSilver
/* highlighted text color */
#define CLHIGHLIGHTTEXT11	DWhite
#define CLHIGHLIGHTTEXT01	DWhite
#define CLHIGHLIGHTTEXT10	DPinetree
#define CLHIGHLIGHTTEXT00	DPinetree
/* border color */
#define CLSIZEBORDER		DPurered
#define CLBORDER11		DNavy
#define CLBORDER01		DDodgerblue
#define CLBORDER10		DNavajowhite
#define CLBORDER00		DSilver
/* scrollbar color */
#define CLSCROLLBAR		DGrey60

#define DEFAULTFONT	"/lib/font/bit/lucm/unicode.9.font"

#define BIG		3		/* factor by which window dimension can exceed screen */

#define QID(w,q)	((w<<8)|(q))
#define WIN(q)		((((ulong)(q).path)>>8) & 0xFFFFFF)
#define FILE(q)		(((ulong)(q).path) & 0xFF)

typedef
enum Mesgtype				/* control messages */
{
	Wakeup,
	Reshaped,
	Moved,
	Refresh,
	Movemouse,
	Rawon,
	Rawoff,
	Holdon,
	Holdoff,
	Deleted,
	Exited,
} Mesgtype;

struct Wctlmesg
{
	Mesgtype	type;
	Rectangle	r;
	Image		*image;
};

struct Conswritemesg
{
	Channel		*cw;		/* chan(Stringpair) */
};

struct Consreadmesg
{
	Channel		*c1;		/* chan(tuple(char *, int) == Stringpair) */
	Channel		*c2;		/* chan(tuple(char *, int) == Stringpair) */
};

struct Mousereadmesg
{
	Channel		*cm;		/* chan(Mouse) */
};

struct Stringpair			/* rune and nrune or byte and nbyte */
{
	void		*s;
	int		ns;
};

struct Mousestate
{
	Mouse;
	ulong		counter;	/* serial no. of mouse event */
};

struct Mouseinfo
{
	Mousestate	queue[16];
	int		ri;		/* read index into queue */
	int		wi;		/* write index */
	ulong		counter;	/* serial no. of last mouse event we received */
	ulong		lastcounter;	/* serial no. of last mouse event sent to client */
	int		lastb;		/* last button state we received */
	uchar		qfull;		/* filled the queue; no more recording until client comes back */
};

struct Window
{
	Ref;
	QLock;
	Frame;
	Image		*i;		/* window image, nil when deleted */
	Mousectl	mc;
	Mouseinfo	mouse;
	Channel		*ck;		/* chan(Rune[10]) */
	Channel		*cctl;		/* chan(Wctlmesg)[20] */
	Channel		*conswrite;	/* chan(Conswritemesg) */
	Channel		*consread;	/* chan(Consreadmesg) */
	Channel		*mouseread;	/* chan(Mousereadmesg) */
	Channel		*wctlread;	/* chan(Consreadmesg) */
	uint		nr;		/* number of runes in window */
	uint		maxr;		/* number of runes allocated in r */
	Rune		*r;
	uint		nraw;
	Rune		*raw;
	uint		org;
	uint		q0;
	uint		q1;
	uint		qh;
	int		id;
	char		name[32];
	uint		namecount;
	Rectangle	scrollr;
	/*
	 * Rio once used originwindow, so screenr could be different from i->r.
	 * Now they're always the same but the code doesn't assume so.
	*/
	Rectangle	screenr;	/* screen coordinates of window */
	int		resized;
	int		wctlready;
	Rectangle	lastsr;
	int		topped;
	int		notefd;
	uchar		scrolling;
	Cursor		cursor;
	Cursor		*cursorp;
	uchar		holding;
	uchar		rawing;
	uchar		ctlopen;
	uchar		wctlopen;
	uchar		deleted;
	uchar		mouseopen;
	char		*label;
	int		pid;
	char		*dir;
};

int			winborder(Window *, Point);
void			winctl(void *);
void			winshell(void *);
Window			*wlookid(int);
Window			*wmk(Image *, Mousectl *, Channel *, Channel *, int);
Window			*wpointto(Point);
Window			*wtop(Point);
void			wtopme(Window *);
void			wbottomme(Window *);
char			*wcontents(Window *, int *);
int			wbswidth(Window *, Rune);
int			wclickmatch(Window *, int, int, int, uint *);
int			wclose(Window *);
Mesgtype		wctlmesg(Window *, Mesgtype, Rectangle, Image *);
uint			wbacknl(Window *, uint, uint);
uint			winsert(Window *, Rune *, int, uint);
void			waddraw(Window *, Rune *, int);
void			wborder(Window *, int);
void			wclosewin(Window *);
void			wcurrent(Window *);
void			wcut(Window *);
void			wdelete(Window *, uint, uint);
void			wdoubleclick(Window *, uint *, uint *);
void			wfill(Window *);
void			wframescroll(Window *, int);
void			wkeyctl(Window *, Rune);
void			wmousectl(Window *);
void			wmovemouse(Window *, Point);
void			wpaste(Window *);
void			wplumb(Window *);
void			wrefresh(Window *, Rectangle);
void			wrepaint(Window *);
void			wresize(Window *, Image *, int);
void			wscrdraw(Window *);
void			wscroll(Window *, int);
void			wselect(Window *);
void			wsendctlmesg(Window *, Mesgtype, Rectangle, Image *);
void			wsetcursor(Window *, int);
void			wsetname(Window *);
void			wsetorigin(Window *, uint, int);
void			wsetpid(Window *, int, int);
void			wsetselect(Window *, uint, uint);
void			wshow(Window *, uint);
void			wsnarf(Window *);
void			wscrsleep(Window *, uint);
void			wsetcols(Window *);

struct Dirtab
{
	char		*name;
	uchar		type;
	uint		qid;
	uint		perm;
};

struct Fid
{
	int		fid;
	int		busy;
	int		open;
	int		mode;
	Qid		qid;
	Window		*w;
	Dirtab		*dir;
	Fid		*next;
	int		nrpart;
	uchar		rpart[UTFmax];
};

struct Xfid
{
	Ref;
	Xfid		*next;
	Xfid		*free;
	Fcall;
	Channel		*c;		/* chan(void(*)(Xfid *)) */
	Fid		*f;
	uchar		*buf;
	Filsys		*fs;
	QLock		active;
	int		flushing;	/* another Xfid is trying to flush us */
	int		flushtag;	/* our tag, so flush can find us */
	Channel		*flushc;	/* channel(int) to notify us we're being flushed */
};

Channel			*xfidinit(void);
void			xfidctl(void *);
void			xfidflush(Xfid *);
void			xfidattach(Xfid *);
void			xfidopen(Xfid *);
void			xfidclose(Xfid *);
void			xfidread(Xfid *);
void			xfidwrite(Xfid *);

enum
{
	Nhash = 16,
};

struct Filsys
{
	int		cfd;
	int		sfd;
	int		pid;
	char		*user;
	Channel		*cxfidalloc;	/* chan(Xfid *) */
	Fid		*fids[Nhash];
};

Filsys			*filsysinit(Channel *);
int			filsysmount(Filsys *, int);
Xfid			*filsysrespond(Filsys *, Xfid *, Fcall *, char *);
void			filsyscancel(Xfid *);

void			wctlproc(void *);
void			wctlthread(void *);

void			deletetimeoutproc(void *);

struct Timer
{
	int		dt;
	int		cancel;
	Channel		*c;		/* chan(int) */
	Timer		*next;
};

Font			*font;
Mousectl		*mousectl;
Mouse			*mouse;
Keyboardctl		*keyboardctl;
Display			*display;
Image			*view;
Screen			*wscreen;
Cursor			boxcursor;
Cursor			crosscursor;
Cursor			sightcursor;
Cursor			whitearrow;
Cursor			query;
Cursor			*corners[9];
Image			*cols[NCOL];
Image			*background;
Image			*textcol00;
Image			*textcol01;
Image			*textcol10;
Image			*textcol11;
Image			*htextcol00;
Image			*htextcol01;
Image			*htextcol10;
Image			*htextcol11;
Image			*highcol00;
Image			*highcol01;
Image			*highcol10;
Image			*highcol11;
Image			*sizecol;
Image			*bordcol00;
Image			*bordcol01;
Image			*bordcol10;
Image			*bordcol11;
Window			**window;
Window			*wkeyboard;	/* window of simulated keyboard */
int			nwindow;
int			snarffd;
Window			*input;
QLock			all;		/* BUG */
Filsys			*filsys;
Window			*hidden[100];
int			nhidden;
int			nsnarf;
Rune			*snarf;
int			scrolling;
int			maxtab;
Channel			*winclosechan;
Channel			*deletechan;
char			*startdir;
int			sweeping;
int			wctlfd;
char			srvpipe[64];
char			srvwctl[64];
int			errorshouldabort;
int			menuing;	/* menu action is pending; waiting for window to be indicated */
int			snarfversion;	/* updated each time it is written */
int			messagesize;	/* negotiated in 9P version setup */
int			debug;
