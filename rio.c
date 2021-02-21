#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <fcall.h>
#include <plumb.h>
#include "dat.h"
#include "err.h"
#include "fns.h"

/*
 *	WASHINGTON (AP) - The Food and Drug Administration warned
 * consumers Wednesday not to use ``Rio'' hair relaxer products
 * because they may cause severe hair loss or turn hair green....
 *	The FDA urged consumers who have experienced problems with Rio
 * to notify their local FDA office, local health department or the
 * company at 1‑800‑543‑3002.
 */

void		resize(void);
void		move(void);
void		delete(void);
void		hide(void);
void		unhide(int);
void		newtile(int);
Image		*sweep(void);
Image		*bandsize(Window *);
Image		*drag(Window *, Rectangle *);
void		refresh(Rectangle);
void		resized(void);
Channel		*exitchan;              /* chan(int) */
Channel		*winclosechan;          /* chan(Window *); */
Rectangle	viewr;
int		threadrforkflag = 0;    /* should be RFENVG but that hides rio from plumber */

void		mousethread(void *);
void		keyboardthread(void *);
void		winclosethread(void *);
void		deletethread(void *);
void		initcmd(void *);

char		*fontname;
int		mainpid;

enum
{
	New,
	Reshape,
	Move,
	Delete,
	Hide,
	Exit,
};

enum
{
	Cut,
	Paste,
	Snarf,
	Plumb,
	Send,
	Scroll,
};

char *menu2str[] = {
	[Cut]		"cut",
	[Paste]		"paste",
	[Snarf]		"snarf",
	[Plumb]		"plumb",
	[Send]		"send",
	[Scroll]	"scroll",
	nil
};

Menu menu2 =
{
	menu2str
};

int Hidden = Exit+1;

char *menu3str[100] = {
	[New]		"New",
	[Reshape]	"Resize",
	[Move]		"Move",
	[Delete]	"Delete",
	[Hide]		"Hide",
	[Exit]		"Exit",
	nil
};

Menu menu3 =
{
	menu3str
};

char *shargv[]  = { "rc", "-i", nil };
char *kbdargv[] = { "rc", "-c", nil, nil };

int errorshouldabort = FALSE;

void
derror(Display *, char *errorstr)
{
	error(errorstr);
}

void
usage(void)
{
	fprint(2, "usage: rio [-f font] [-i initcmd] [-k kbdcmd] [-s]\n");
	exits("usage");
}

void
threadmain(int argc, char *argv[])
{
	char *initstr, *kbdin, *s;
	char buf[256];
	Image *i;
	Rectangle r;

	if(strstr(argv[0], ".out") == nil){
		menu3str[Exit] = nil;
		Hidden--;
	}
	initstr = nil;
	kbdin = nil;
	maxtab = 0;
	ARGBEGIN{
	case 'f':
		fontname = ARGF();
		if(fontname == nil)
			usage();
		break;
	case 'i':
		initstr = ARGF();
		if(initstr == nil)
			usage();
		break;
	case 'k':
		if(kbdin != nil)
			usage();
		kbdin = ARGF();
		if(kbdin == nil)
			usage();
		break;
	case 's':
		scrolling = TRUE;
		break;
	}ARGEND

	mainpid = getpid();
	if(getwd(buf, sizeof buf) == nil)
		startdir = estrdup(".");
	else
		startdir = estrdup(buf);
	if(fontname == nil)
		fontname = getenv("font");
	if(fontname == nil)
		fontname = DEFAULTFONT;
	s = getenv("tabstop");
	if(s != nil)
		maxtab = strtol(s, nil, 0);
	if(maxtab == 0)
		maxtab = 4;
	free(s);
	/* check font before barging ahead */
	if(access(fontname, AEXIST) < 0){
		fprint(2, "rio: can't access font %s: %r\n", fontname);
		exits("font open");
	}
	putenv("font", fontname);

	snarffd = open("/dev/snarf", OREAD|OCEXEC);

	if(geninitdraw(nil, derror, nil, "rio", nil, Refnone) < 0){
		fprint(2, "rio: can't open display: %r\n");
		exits("display open");
	}
	setshell();
	iconinit();
	view = screen;
	viewr = view->r;
	mousectl = initmouse(nil, screen);
	if(mousectl == nil)
		error(Emouse);
	mouse = mousectl;
	keyboardctl = initkeyboard(nil);
	if(keyboardctl == nil)
		error(Ekbd);
	wscreen = allocscreen(screen, background, FALSE);
	if(wscreen == nil)
		error(Escrn);
	draw(view, viewr, background, nil, ZP);
	flushimage(display, TRUE);

	exitchan = chancreate(sizeof(int), 0);
	winclosechan = chancreate(sizeof(Window *), 0);
	deletechan = chancreate(sizeof(char *), 0);

	timerinit();
	threadcreate(keyboardthread, nil, STACK);
	threadcreate(mousethread, nil, STACK);
	threadcreate(winclosethread, nil, STACK);
	threadcreate(deletethread, nil, STACK);
	filsys = filsysinit(xfidinit());

	if(filsys == nil)
		fprint(2, "rio: can't create file system server: %r\n");
	else{
		errorshouldabort = TRUE;	/* suicide if there's trouble after this */
		if(initstr != nil)
			proccreate(initcmd, initstr, STACK);
		if(kbdin != nil){
			kbdargv[2] = kbdin;
			r = screen->r;
			r.max.x = r.min.x+300;
			r.max.y = r.min.y+80;
			i = allocwindow(wscreen, r, Refbackup, CLWINDOW);
			wkeyboard = new(i, FALSE, scrolling, 0, nil, shell, kbdargv);
			if(wkeyboard == nil)
				error(Ewkbd);
		}
		threadnotify(shutdown, TRUE);
		recv(exitchan, nil);
	}
	killprocs();
	threadexitsall(nil);
}

/*
 * /dev/snarf updates when the file is closed, so we must open our own
 * fd here rather than use snarffd
 */
void
putsnarf(void)
{
	int fd, i, n;

	if(snarffd<0 || nsnarf==0)
		return;
	fd = open("/dev/snarf", OWRITE);
	if(fd < 0)
		return;
	/* snarf buffer could be huge, so fprint will truncate; do it in blocks */
	for(i=0; i<nsnarf; i+=n){
		n = nsnarf-i;
		if(n >= 256)
			n = 256;
		if(fprint(fd, "%.*S", n, snarf+i) < 0)
			break;
	}
	close(fd);
}

void
getsnarf(void)
{
	int i, n, nb, nulls;
	char *sn, buf[1024];

	if(snarffd < 0)
		return;
	sn = nil;
	i = 0;
	seek(snarffd, 0, 0);
	while((n = read(snarffd, buf, sizeof buf)) > 0){
		sn = erealloc(sn, i+n+1);
		memmove(sn+i, buf, n);
		i += n;
		sn[i] = 0;
	}
	if(i > 0){
		snarf = runerealloc(snarf, i+1);
		cvttorunes(sn, i, snarf, &nb, &nsnarf, &nulls);
		free(sn);
	}
}

void
setshell(void)
{
	int i, l;

	shell = getenv("shell");
	if(shell == nil)
		shell = DEFAULTSHELL;
	if(access(shell, AEXEC) < 0){
		fprint(2, "rio: can't access shell %s: %r\n", shell);
		exits("shell");
	}
	putenv("shell", shell);

	for(i = strlen(shell)-1; i >= 0; i--)
		if(shell[i] == '/')
			break;
	l = strlen(shell)-i-1;
	shellname = (char *)malloc(sizeof(char)*l);
	strcpy(shellname, shell+i+1);
	shargv[0] = shellname;
	kbdargv[0] = shellname;
}

void
initcmd(void *arg)
{
	char *cmd;

	cmd = arg;
	rfork(RFENVG|RFFDG|RFNOTEG|RFNAMEG);
	procexecl(nil, shell, shellname, "-c", cmd, nil);
	fprint(2, "rio: exec failed: %r\n");
	exits("exec");
}

char *oknotes[] =
{
	"delete",
	"hangup",
	"kill",
	"exit",
	nil
};

int
shutdown(void *, char *msg)
{
	int i;
	static Lock shutdownlk;

	killprocs();
	for(i=0; oknotes[i]; i++)
		if(strncmp(oknotes[i], msg, strlen(oknotes[i])) == 0){
			lock(&shutdownlk); /* only one can threadexitsall */
			threadexitsall(msg);
		}
	fprint(2, "rio %d: abort: %s\n", getpid(), msg);
	abort();
	exits(msg);
	return 0;
}

void
killprocs(void)
{
	int i;

	for(i=0; i<nwindow; i++)
		postnote(PNGROUP, window[i]->pid, "hangup");
}

void
keyboardthread(void *)
{
	Rune buf[2][20], *rp;
	int n, i;

	threadsetname("keyboardthread");
	n = 0;
	for(;;){
		rp = buf[n];
		n = 1-n;
		recv(keyboardctl->c, rp);
		for(i=1; i<nelem(buf[0])-1; i++)
			if(nbrecv(keyboardctl->c, rp+i) <= 0)
				break;
		rp[i] = L'\0';
		if(input != nil)
			sendp(input->ck, rp);
	}
}

/*
 * Used by /dev/kbdin
 */
void
keyboardsend(char *s, int cnt)
{
	Rune *r;
	int i, nb, nr;

	r = runemalloc(cnt);
	/* BUGlet: partial runes will be converted to error runes */
	cvttorunes(s, cnt, r, &nb, &nr, nil);
	for(i=0; i<nr; i++)
		send(keyboardctl->c, &r[i]);
	free(r);
}

int
portion(int x, int lo, int hi)
{
	x -= lo;
	hi -= lo;
	if(x < 20)
		return 0;
	if(x > hi-20)
		return 2;
	return 1;
}

int
whichcorner(Window *w, Point p)
{
	int i, j;

	i = portion(p.x, w->screenr.min.x, w->screenr.max.x);
	j = portion(p.y, w->screenr.min.y, w->screenr.max.y);
	return 3*j+i;
}

void
cornercursor(Window *w, Point p, int force)
{
	if(w!=nil && winborder(w, p))
		riosetcursor(corners[whichcorner(w, p)], force);
	else
		wsetcursor(w, force);
}

/* thread to allow fsysproc to synchronize window closing with main proc */
void
winclosethread(void *)
{
	Window *w;

	threadsetname("winclosethread");
	for(;;){
		w = recvp(winclosechan);
		wclose(w);
	}
}

/* thread to make Deleted windows that the client still holds disappear offscreen after an interval */
void
deletethread(void *)
{
	char *s;
	Image *i;

	threadsetname("deletethread");
	for(;;){
		s = recvp(deletechan);
		i = namedimage(display, s);
		if(i != nil){
			/* move it off-screen to hide it, since client is slow in letting it go */
			originwindow(i, i->r.min, view->r.max);
		}
		freeimage(i);
		free(s);
	}
}

void
deletetimeoutproc(void *v)
{
	char *s;

	s = v;
	sleep(750); /* remove window from screen after 3/4 of a second */
	sendp(deletechan, s);
}

/*
 * Button 6 - keyboard toggle - has been pressed.
 * Send event to keyboard, wait for button up, send that.
 * Note: there is no coordinate translation done here; this
 * is just about getting button 6 to the keyboard simulator.
 */
void
keyboardhide(void)
{
	send(wkeyboard->mc.c, mouse);
	do
		readmouse(mousectl);
	while((mouse->buttons & (1<<5)) != 0);
	send(wkeyboard->mc.c, mouse);
}

void
mousethread(void *)
{
	int sending, inside, scrolling, moving, band;
	Window *oin, *w, *winput;
	Image *i;
	Rectangle r;
	Point xy;
	Mouse tmp;
	enum {
		MReshape,
		MMouse,
		NALT
	};
	static Alt alts[NALT+1];

	threadsetname("mousethread");
	sending = FALSE;
	scrolling = FALSE;
	moving = FALSE;

	alts[MReshape].c = mousectl->resizec;
	alts[MReshape].v = nil;
	alts[MReshape].op = CHANRCV;
	alts[MMouse].c = mousectl->c;
	alts[MMouse].v = &mousectl->Mouse;
	alts[MMouse].op = CHANRCV;
	alts[NALT].op = CHANEND;

	for(;;)
	    switch(alt(alts)){
		case MReshape:
			resized();
			break;
		case MMouse:
			if(wkeyboard!=nil && (mouse->buttons & (1<<5))!=0){
				keyboardhide();
				break;
			}
		Again:
			winput = input;
			/* override everything for the keyboard window */
			if(wkeyboard != nil && ptinrect(mouse->xy, wkeyboard->screenr)){
				/* make sure it's on top; this call is free if it is */
				wtopme(wkeyboard);
				winput = wkeyboard;
			}
			if(winput!=nil && winput->i!=nil){
				/* convert to logical coordinates */
				xy.x = mouse->xy.x + (winput->i->r.min.x-winput->screenr.min.x);
				xy.y = mouse->xy.y + (winput->i->r.min.y-winput->screenr.min.y);

				/* the up and down scroll buttons are not subject to the usual rules */
				if((mouse->buttons&(8|16))!=0 && !winput->mouseopen)
					goto Sending;

				inside = ptinrect(mouse->xy, insetrect(winput->screenr, Selborder));
				if(winput->mouseopen)
					scrolling = FALSE;
				else if(scrolling)
                    			scrolling = mouse->buttons;
				else
					scrolling = mouse->buttons!=0 && ptinrect(xy, winput->scrollr);
				/* topped will be zero or less if window has been bottomed */
				if(!sending && !scrolling && winborder(winput, mouse->xy) && winput->topped>0){
					moving = TRUE;
				}else if(inside && (scrolling || winput->mouseopen || (mouse->buttons&1)!=0))
					sending = TRUE;
			}else
				sending = FALSE;
			if(sending){
			Sending:
				if(mouse->buttons == 0){
					cornercursor(winput, mouse->xy, FALSE);
					sending = FALSE;
				}else
					wsetcursor(winput, FALSE);
				tmp = mousectl->Mouse;
				tmp.xy = xy;
				send(winput->mc.c, &tmp);
				continue;
			}
			w = wpointto(mouse->xy);
			/* change cursor if over anyone's border */
			if(w != nil)
				cornercursor(w, mouse->xy, FALSE);
			else
				riosetcursor(nil, FALSE);
			if(moving && (mouse->buttons&7)!=0){
				oin = winput;
				band = mouse->buttons & 3;
				sweeping = TRUE;
				if(band)
					i = bandsize(winput);
				else
					i = drag(winput, &r);
				sweeping = FALSE;
				if(i != nil){
					if(winput == oin){
						if(band)
							wsendctlmesg(winput, Reshaped, i->r, i);
						else
							wsendctlmesg(winput, Moved, r, i);
						cornercursor(winput, mouse->xy, TRUE);
					}else
						freeimage(i);
				}
			}
			if(w != nil)
				cornercursor(w, mouse->xy, FALSE);
			/* we're not sending the event, but if button is down maybe we should */
			if(mouse->buttons != 0){
				/* w->topped will be zero or less if window has been bottomed */
				if(w==nil || (w==winput && w->topped>0)){
					if((mouse->buttons&1) != 0){
						;
					}else if((mouse->buttons&2) != 0){
						if(winput!=nil && !winput->mouseopen)
							button2menu(winput);
					}else if((mouse->buttons&4) != 0)
						button3menu();
				}else{
					/* if button 1 event in the window, top the window and wait for button up. */
					/* otherwise, top the window and pass the event on */
					if(wtop(mouse->xy) && (mouse->buttons!=1 || winborder(w, mouse->xy)))
						goto Again;
					goto Drain;
				}
			}
			moving = FALSE;
			break;

		Drain:
			do
				readmouse(mousectl);
			while(mousectl->buttons != 0);
			moving = FALSE;
			goto Again; /* recalculate mouse position, cursor */
		}
}

void
resized(void)
{
	Image *im;
	int i, j, ishidden;
	Rectangle r;
	Point o, n;
	Window *w;

	if(getwindow(display, Refnone) < 0)
		error(Ereattach);
	freescrtemps();
	view = screen;
	freescreen(wscreen);
	wscreen = allocscreen(screen, background, FALSE);
	if(wscreen == nil)
		error(Ereallocscrn);
	draw(view, view->r, background, nil, ZP);
	o = subpt(viewr.max, viewr.min);
	n = subpt(view->clipr.max, view->clipr.min);
	for(i=0; i<nwindow; i++){
		w = window[i];
		if(w->deleted)
			continue;
		r = rectsubpt(w->i->r, viewr.min);
		r.min.x = (r.min.x*n.x)/o.x;
		r.min.y = (r.min.y*n.y)/o.y;
		r.max.x = (r.max.x*n.x)/o.x;
		r.max.y = (r.max.y*n.y)/o.y;
		r = rectaddpt(r, screen->clipr.min);
		ishidden = FALSE;
		for(j=0; j<nhidden; j++)
			if(w == hidden[j]){
				ishidden = TRUE;
				break;
			}
		if(ishidden){
			im = allocimage(display, r, screen->chan, FALSE, CLWINDOW);
			r = ZR;
		}else
			im = allocwindow(wscreen, r, Refbackup, CLWINDOW);
		if(im != nil)
			wsendctlmesg(w, Reshaped, r, im);
	}
	viewr = screen->r;
	flushimage(display, TRUE);
}

void
button3menu(void)
{
	int i;

	for(i=0; i<nhidden; i++)
		menu3str[i+Hidden] = hidden[i]->label;
	menu3str[i+Hidden] = nil;

	sweeping = TRUE;
	i = menuhit(3, mousectl, &menu3, wscreen);
	switch(i){
	case -1:
		break;
	case New:
		new(sweep(), FALSE, scrolling, 0, nil, shell, nil);
		break;
	case Reshape:
		resize();
		break;
	case Move:
		move();
		break;
	case Delete:
		delete();
		break;
	case Hide:
		hide();
		break;
	case Exit:
		if(Hidden > Exit){
			send(exitchan, nil);
			break;
		}
		/* else fall through */
	default:
		unhide(i);
		break;
	}
	sweeping = FALSE;
}

void
button2menu(Window *w)
{
	if(w->deleted)
		return;
	incref(w);
	if(w->scrolling)
		menu2str[Scroll] = "noscroll";
	else
		menu2str[Scroll] = "scroll";
	switch(menuhit(2, mousectl, &menu2, wscreen)){
	case Cut:
		wsnarf(w);
		wcut(w);
		wscrdraw(w);
		break;

	case Snarf:
		wsnarf(w);
		break;

	case Paste:
		getsnarf();
		wpaste(w);
		wscrdraw(w);
		break;

	case Plumb:
		wplumb(w);
		break;

	case Send:
		getsnarf();
		wsnarf(w);
		if(nsnarf == 0)
			break;
		if(w->rawing){
			waddraw(w, snarf, nsnarf);
			if(snarf[nsnarf-1]!='\n' && snarf[nsnarf-1]!='\004')
				waddraw(w, L"\n", 1);
		}else{
			winsert(w, snarf, nsnarf, w->nr);
			if(snarf[nsnarf-1]!='\n' && snarf[nsnarf-1]!='\004')
				winsert(w, L"\n", 1, w->nr);
		}
		wsetselect(w, w->nr, w->nr);
		wshow(w, w->nr);
		break;

	case Scroll:
		if(w->scrolling ^= 1)
			wshow(w, w->nr);
		break;
	}
	wclose(w);
	wsendctlmesg(w, Wakeup, ZR, nil);
	flushimage(display, TRUE);
}

Point
onscreen(Point p)
{
	p.x = max(screen->clipr.min.x, p.x);
	p.x = min(screen->clipr.max.x, p.x);
	p.y = max(screen->clipr.min.y, p.y);
	p.y = min(screen->clipr.max.y, p.y);
	return p;
}

Image
*sweep(void)
{
	Image *i, *oi;
	Rectangle r;
	Point p0, p;

	i = nil;
	menuing = TRUE;
	riosetcursor(&crosscursor, TRUE);
	while(mouse->buttons == 0)
		readmouse(mousectl);
	p0 = onscreen(mouse->xy);
	p = p0;
	r.min = p;
	r.max = p;
	oi = nil;
	while(mouse->buttons == 4){
		readmouse(mousectl);
		if(mouse->buttons != 4 && mouse->buttons != 0)
			break;
		if(!eqpt(mouse->xy, p)){
			p = onscreen(mouse->xy);
			r = canonrect(Rpt(p0, p));
			if(Dx(r)>5 && Dy(r)>5){
				i = allocwindow(wscreen, r, Refnone, CLWINDOW);
				freeimage(oi);
				if(i == nil)
					goto Rescue;
				oi = i;
				border(i, r, Selborder, actioncol, ZP);
				flushimage(display, TRUE);
			}
		}
	}
	if(mouse->buttons != 0)
		goto Rescue;
	if(i==nil || Dx(i->r)<100 || Dy(i->r)<3*font->height)
		goto Rescue;
	oi = i;
	i = allocwindow(wscreen, oi->r, Refbackup, CLWINDOW);
	freeimage(oi);
	if(i == nil)
		goto Rescue;
	border(i, r, Selborder, actioncol, ZP);
	cornercursor(input, mouse->xy, TRUE);
	goto Return;

 Rescue:
	freeimage(i);
	i = nil;
	cornercursor(input, mouse->xy, TRUE);
	while(mouse->buttons != 0)
		readmouse(mousectl);

 Return:
	moveto(mousectl, mouse->xy);	/* force cursor update; ugly */
	menuing = FALSE;
	return i;
}

void
drawedge(Image **bp, Rectangle r)
{
	Image *b = *bp;
	if(b != nil && Dx(b->r) == Dx(r) && Dy(b->r) == Dy(r))
		originwindow(b, r.min, r.min);
	else{
		freeimage(b);
		*bp = allocwindow(wscreen, r, Refbackup, DRed);
	}
}

void
drawborder(Rectangle r, int show)
{
	static Image *b[4];
	int i;
	if(show){
		r = canonrect(r);
		drawedge(&b[0], Rect(r.min.x, r.min.y, r.min.x+Borderwidth, r.max.y));
		drawedge(&b[1], Rect(r.min.x+Borderwidth, r.min.y, r.max.x-Borderwidth, r.min.y+Borderwidth));
		drawedge(&b[2], Rect(r.max.x-Borderwidth, r.min.y, r.max.x, r.max.y));
		drawedge(&b[3], Rect(r.min.x+Borderwidth, r.max.y-Borderwidth, r.max.x-Borderwidth, r.max.y));
	}else{
		for(i = 0; i < 4; i++){
			freeimage(b[i]);
			b[i] = nil;
		}
	}
}

Image
*drag(Window *w, Rectangle *rp)
{
	Image *i, *ni;
	Point p, op, d, dm, om;
	Rectangle r;

	i = w->i;
	menuing = TRUE;
	om = mouse->xy;
	riosetcursor(&boxcursor, TRUE);
	dm = subpt(mouse->xy, w->screenr.min);
	d = subpt(i->r.max, i->r.min);
	op = subpt(mouse->xy, dm);
	drawborder(Rect(op.x, op.y, op.x+d.x, op.y+d.y), TRUE);
	flushimage(display, TRUE);
	while(mouse->buttons == 4){
		p = subpt(mouse->xy, dm);
		if(!eqpt(p, op)){
			drawborder(Rect(p.x, p.y, p.x+d.x, p.y+d.y), TRUE);
			flushimage(display, TRUE);
			op = p;
		}
		readmouse(mousectl);
	}
	r = Rect(op.x, op.y, op.x+d.x, op.y+d.y);
	drawborder(r, FALSE);
	cornercursor(w, mouse->xy, TRUE);
	moveto(mousectl, mouse->xy);	/* force cursor update; ugly */
	menuing = FALSE;
	flushimage(display, TRUE);
	if(mouse->buttons!=0 || (ni=allocwindow(wscreen, r, Refbackup, CLWINDOW))==nil){
		moveto(mousectl, om);
		while(mouse->buttons != 0)
			readmouse(mousectl);
		*rp = Rect(0, 0, 0, 0);
		return nil;
	}
	draw(ni, ni->r, i, nil, i->r.min);
	*rp = r;
	return ni;
}

Point
cornerpt(Rectangle r, Point p, int which)
{
	switch(which){
	case 0: /* top left */
		p = Pt(r.min.x, r.min.y);
		break;
	case 2: /* top right */
		p = Pt(r.max.x,r.min.y);
		break;
	case 6: /* bottom left */
		p = Pt(r.min.x, r.max.y);
		break;
	case 8: /* bottom right */
		p = Pt(r.max.x, r.max.y);
		break;
	case 1: /* top edge */
		p = Pt(p.x,r.min.y);
		break;
	case 5: /* right edge */
		p = Pt(r.max.x, p.y);
		break;
	case 7: /* bottom edge */
		p = Pt(p.x, r.max.y);
		break;
	case 3: /* left edge */
		p = Pt(r.min.x, p.y);
		break;
	}
	return p;
}

Rectangle
whichrect(Rectangle r, Point p, int which)
{
	switch(which){
	case 0: /* top left */
		r = Rect(p.x, p.y, r.max.x, r.max.y);
		break;
	case 2: /* top right */
		r = Rect(r.min.x, p.y, p.x, r.max.y);
		break;
	case 6: /* bottom left */
		r = Rect(p.x, r.min.y, r.max.x, p.y);
		break;
	case 8: /* bottom right */
		r = Rect(r.min.x, r.min.y, p.x, p.y);
		break;
	case 1: /* top edge */
		r = Rect(r.min.x, p.y, r.max.x, r.max.y);
		break;
	case 5: /* right edge */
		r = Rect(r.min.x, r.min.y, p.x, r.max.y);
		break;
	case 7: /* bottom edge */
		r = Rect(r.min.x, r.min.y, r.max.x, p.y);
		break;
	case 3: /* left edge */
		r = Rect(p.x, r.min.y, r.max.x, r.max.y);
		break;
	}
	return canonrect(r);
}

Image
*bandsize(Window *w)
{
	Image *i;
	Rectangle r, or;
	Point p, startp;
	int which, but;

	p = mouse->xy;
	but = mouse->buttons;
	which = whichcorner(w, p);
	p = cornerpt(w->screenr, p, which);
	wmovemouse(w, p);
	readmouse(mousectl);
	r = whichrect(w->screenr, p, which);
	drawborder(r, TRUE);
	or = r;
	startp = p;

	while(mouse->buttons == but){
		p = onscreen(mouse->xy);
		r = whichrect(w->screenr, p, which);
		if(!eqrect(r, or) && goodrect(r)){
			drawborder(r, TRUE);
			flushimage(display, TRUE);
			or = r;
		}
		readmouse(mousectl);
	}
	p = mouse->xy;
	drawborder(or, FALSE);
	flushimage(display, TRUE);
	wsetcursor(w, TRUE);
	if(mouse->buttons!=0 || Dx(or)<100 || Dy(or)<3*font->height){
		while(mouse->buttons != 0)
			readmouse(mousectl);
		return nil;
	}
	if(abs(p.x-startp.x)+abs(p.y-startp.y) <= 1)
		return nil;
	i = allocwindow(wscreen, or, Refbackup, CLWINDOW);
	if(i == nil)
		return nil;
	border(i, r, Selborder, actioncol, ZP);
	return i;
}

Window
*pointto(int wait)
{
	Window *w;

	menuing = TRUE;
	riosetcursor(&sightcursor, TRUE);
	while(mouse->buttons == 0)
		readmouse(mousectl);
	if(mouse->buttons == 4)
		w = wpointto(mouse->xy);
	else
		w = nil;
	if(wait){
		while(mouse->buttons != 0){
			if(mouse->buttons!=4 && w !=nil){	/* cancel */
				cornercursor(input, mouse->xy, FALSE);
				w = nil;
			}
			readmouse(mousectl);
		}
		if(w!=nil && wpointto(mouse->xy)!=w)
			w = nil;
	}
	cornercursor(input, mouse->xy, FALSE);
	moveto(mousectl, mouse->xy);	/* force cursor update; ugly */
	menuing = FALSE;
	return w;
}

void
delete(void)
{
	Window *w;

	w = pointto(TRUE);
	if(w != nil)
		wsendctlmesg(w, Deleted, ZR, nil);
}

void
resize(void)
{
	Window *w;
	Image *i;

	w = pointto(TRUE);
	if(w == nil)
		return;
	i = sweep();
	if(i != nil)
		wsendctlmesg(w, Reshaped, i->r, i);
}

void
move(void)
{
	Window *w;
	Image *i;
	Rectangle r;

	w = pointto(FALSE);
	if(w == nil)
		return;
	i = drag(w, &r);
	if(i != nil)
		wsendctlmesg(w, Moved, r, i);
	cornercursor(input, mouse->xy, TRUE);
}

int
whide(Window *w)
{
	Image *i;
	int j;

	for(j=0; j<nhidden; j++)
		if(hidden[j] == w)	/* already hidden */
			return -1;
	i = allocimage(display, w->screenr, w->i->chan, FALSE, CLWINDOW);
	if(i != nil){
		hidden[nhidden++] = w;
		wsendctlmesg(w, Reshaped, ZR, i);
		return 1;
	}
	return 0;
}

int
wunhide(int h)
{
	Image *i;
	Window *w;

	w = hidden[h];
	i = allocwindow(wscreen, w->i->r, Refbackup, CLWINDOW);
	if(i != nil){
		--nhidden;
		memmove(hidden+h, hidden+h+1, (nhidden-h)*sizeof(Window *));
		wsendctlmesg(w, Reshaped, w->i->r, i);
		return TRUE;
	}
	return FALSE;
}

void
hide(void)
{
	Window *w;

	w = pointto(TRUE);
	if(w == nil)
		return;
	whide(w);
}

void
unhide(int h)
{
	Window *w;

	h -= Hidden;
	w = hidden[h];
	if(w == nil)
		return;
	wunhide(h);
}

Window
*new(Image *i, int hideit, int scrollit, int pid, char *dir, char *cmd, char **argv)
{
	Window *w;
	Mousectl *mc;
	Channel *cm, *ck, *cctl, *cpid;
	void **arg;

	if(i == nil)
		return nil;
	cm = chancreate(sizeof(Mouse), 0);
	ck = chancreate(sizeof(Rune *), 0);
	cctl = chancreate(sizeof(Wctlmesg), 4);
	cpid = chancreate(sizeof(int), 0);
	if(cm==nil || ck==nil || cctl==nil)
		error(Echanalloc);
	mc = emalloc(sizeof(Mousectl));
	*mc = *mousectl;
	mc->image = i;
	mc->c = cm;
	w = wmk(i, mc, ck, cctl, scrollit);
	free(mc);	/* wmk copies *mc */
	window = erealloc(window, ++nwindow*sizeof(Window *));
	window[nwindow-1] = w;
	if(hideit){
		hidden[nhidden++] = w;
		w->screenr = ZR;
	}
	threadcreate(winctl, w, STACK);
	if(!hideit)
		wcurrent(w);
	flushimage(display, TRUE);
	if(pid == 0){
		arg = emalloc(5*sizeof(void *));
		arg[0] = w;
		arg[1] = cpid;
		arg[2] = cmd;
		if(argv == nil)
			arg[3] = shargv;
		else
			arg[3] = argv;
		arg[4] = dir;
		proccreate(winshell, arg, STACK);
		pid = recvul(cpid);
		free(arg);
	}
	if(pid == 0){
		/* window creation failed */
		wsendctlmesg(w, Deleted, ZR, nil);
		chanfree(cpid);
		return nil;
	}
	wsetpid(w, pid, TRUE);
	wsetname(w);
	if(dir != nil)
		w->dir = estrdup(dir);
	chanfree(cpid);
	return w;
}
