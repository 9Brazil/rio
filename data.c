#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <fcall.h>
#include "dat.h"
#include "err.h"
#include "fns.h"

Cursor crosscursor = {
	{-7, -7},
	{
		0x03, 0xC0,
		0x03, 0xC0,
		0x03, 0xC0,
		0x03, 0xC0,
		0x03, 0xC0,
		0x03, 0xC0,
		0xFF, 0xFF,
		0xFF, 0xFF,
		0xFF, 0xFF,
		0xFF, 0xFF,
		0x03, 0xC0,
		0x03, 0xC0,
		0x03, 0xC0,
		0x03, 0xC0,
		0x03, 0xC0,
		0x03, 0xC0,
	},
	{
		0x00, 0x00,
		0x01, 0x80,
		0x01, 0x80,
		0x01, 0x80,
		0x01, 0x80,
		0x01, 0x80,
		0x01, 0x80,
		0x7F, 0xFE,
		0x7F, 0xFE,
		0x01, 0x80,
		0x01, 0x80,
		0x01, 0x80,
		0x01, 0x80,
		0x01, 0x80,
		0x01, 0x80,
		0x00, 0x00,
	}
};

Cursor boxcursor = {
	{-7, -7},
	{
		0xFF, 0xFF,
		0xFF, 0xFF,
		0xFF, 0xFF,
		0xFF, 0xFF,
		0xFF, 0xFF,
		0xF8, 0x1F,
		0xF8, 0x1F,
		0xF8, 0x1F,
		0xF8, 0x1F,
		0xF8, 0x1F,
		0xF8, 0x1F,
		0xFF, 0xFF,
		0xFF, 0xFF,
		0xFF, 0xFF,
		0xFF, 0xFF,
		0xFF, 0xFF,
	},
	{
		0x00, 0x00,
		0x7F, 0xFE,
		0x7F, 0xFE,
		0x7F, 0xFE,
		0x70, 0x0E,
		0x70, 0x0E,
		0x70, 0x0E,
		0x70, 0x0E,
		0x70, 0x0E,
		0x70, 0x0E,
		0x70, 0x0E,
		0x70, 0x0E,
		0x7F, 0xFE,
		0x7F, 0xFE,
		0x7F, 0xFE,
		0x00, 0x00,
	}
};

Cursor sightcursor = {
	{-7, -7},
	{
		0x1F, 0xF8,
		0x3F, 0xFC,
		0x7F, 0xFE,
		0xFB, 0xDF,
		0xF3, 0xCF,
		0xE3, 0xC7,
		0xFF, 0xFF,
		0xFF, 0xFF,
		0xFF, 0xFF,
		0xFF, 0xFF,
		0xE3, 0xC7,
		0xF3, 0xCF,
		0x7B, 0xDF,
		0x7F, 0xFE,
		0x3F, 0xFC,
		0x1F, 0xF8,
	},
	{
		0x00, 0x00,
		0x0F, 0xF0,
		0x31, 0x8C,
		0x21, 0x84,
		0x41, 0x82,
		0x41, 0x82,
		0x41, 0x82,
		0x7F, 0xFE,
		0x7F, 0xFE,
		0x41, 0x82,
		0x41, 0x82,
		0x41, 0x82,
		0x21, 0x84,
		0x31, 0x8C,
		0x0F, 0xF0,
		0x00, 0x00,
	}
};

Cursor whitearrow = {
	{0, 0},
	{
		0xFF, 0xFF,
		0xFF, 0xFF,
		0xFF, 0xFE,
		0xFF, 0xFC,
		0xFF, 0xF0,
		0xFF, 0xF0,
		0xFF, 0xF8,
		0xFF, 0xFC,
		0xFF, 0xFE,
		0xFF, 0xFF,
		0xFF, 0xFE,
		0xFF, 0xFC,
		0xF3, 0xF8,
		0xF1, 0xF0,
		0xE0, 0xE0,
		0xC0, 0x40,
	},
	{
		0xFF, 0xFF,
		0xFF, 0xFF,
		0xC0, 0x06,
		0xC0, 0x1C,
		0xC0, 0x30,
		0xC0, 0x30,
		0xC0, 0x38,
		0xC0, 0x1C,
		0xC0, 0x0E,
		0xC0, 0x07,
		0xCE, 0x0E,
		0xDF, 0x1C,
		0xD3, 0xB8,
		0xF1, 0xF0,
		0xE0, 0xE0,
		0xC0, 0x40,
	}
};

Cursor query = {
	{-7,-7},
	{
		0x0F, 0xF0,
		0x1F, 0xF8,
		0x3F, 0xFC,
		0x7F, 0xFE,
	 	0x7C, 0x7E,
		0x78, 0x7E,
		0x00, 0xFC,
		0x01, 0xF8,
		0x03, 0xF0,
		0x07, 0xE0,
		0x07, 0xC0,
		0x07, 0xC0,
		0x07, 0xC0,
		0x07, 0xC0,
		0x07, 0xC0,
		0x07, 0xC0,
	},
	{
		0x00, 0x00,
		0x0F, 0xF0,
		0x1F, 0xF8,
		0x3C, 0x3C,
		0x38, 0x1C,
		0x00, 0x3C,
		0x00, 0x78,
		0x00, 0xF0,
		0x01, 0xE0,
		0x03, 0xC0,
		0x03, 0x80,
		0x03, 0x80,
		0x00, 0x00,
		0x03, 0x80,
		0x03, 0x80,
		0x00, 0x00,
	}
};

Cursor tl = {
	{-4, -4},
	{
		0xFE, 0x00,
		0x82, 0x00,
		0x8C, 0x00,
		0x87, 0xFF,
		0xA0, 0x01,
		0xB0, 0x01,
		0xD0, 0x01,
		0x11, 0xFF,
		0x11, 0x00,
		0x11, 0x00,
		0x11, 0x00,
		0x11, 0x00,
		0x11, 0x00,
		0x11, 0x00,
		0x11, 0x00,
		0x1F, 0x00,
	},
	{
		0x00, 0x00,
		0x7C, 0x00,
		0x70, 0x00,
		0x78, 0x00,
		0x5F, 0xFE,
		0x4F, 0xFE,
		0x0F, 0xFE,
		0x0E, 0x00,
		0x0E, 0x00,
		0x0E, 0x00,
		0x0E, 0x00,
		0x0E, 0x00,
		0x0E, 0x00,
		0x0E, 0x00,
		0x0E, 0x00,
		0x00, 0x00,
	}
};

Cursor t = {
	{-7, -8},
	{
		0x00, 0x00,
		0x00, 0x00,
		0x03, 0x80,
		0x06, 0xC0,
		0x1C, 0x70,
		0x10, 0x10,
		0x0C, 0x60,
		0xFC, 0x7F,
		0x80, 0x01,
		0x80, 0x01,
		0x80, 0x01,
		0xFF, 0xFF,
		0x00, 0x00,
		0x00, 0x00,
		0x00, 0x00,
		0x00, 0x00,
	},
	{
		0x00, 0x00,
		0x00, 0x00,
		0x00, 0x00,
		0x01, 0x00,
		0x03, 0x80,
		0x0F, 0xE0,
		0x03, 0x80,
		0x03, 0x80,
		0x7F, 0xFE,
		0x7F, 0xFE,
		0x7F, 0xFE,
		0x00, 0x00,
		0x00, 0x00,
		0x00, 0x00,
		0x00, 0x00,
		0x00, 0x00,
	}
};

Cursor tr = {
	{-11, -4},
	{
		0x00, 0x7F,
		0x00, 0x41,
		0x00, 0x31,
		0xFF, 0xE1,
		0x80, 0x05,
		0x80, 0x0D,
		0x80, 0x0B,
		0xFF, 0x88,
		0x00, 0x88,
		0x00, 0x88,
		0x00, 0x88,
		0x00, 0x88,
		0x00, 0x88,
		0x00, 0x88,
		0x00, 0x88,
		0x00, 0xF8,
	},
	{
		0x00, 0x00,
		0x00, 0x3E,
		0x00, 0x0E,
		0x00, 0x1E,
		0x7F, 0xFA,
		0x7F, 0xF2,
		0x7F, 0xF0,
		0x00, 0x70,
		0x00, 0x70,
		0x00, 0x70,
		0x00, 0x70,
		0x00, 0x70,
		0x00, 0x70,
		0x00, 0x70,
		0x00, 0x70,
		0x00, 0x00,
	}
};

Cursor r = {
	{-8, -7},
	{
		0x07, 0xC0,
		0x04, 0x40,
		0x04, 0x40,
		0x04, 0x58,
		0x04, 0x68,
		0x04, 0x6C,
		0x04, 0x06,
		0x04, 0x02,
		0x04, 0x06,
		0x04, 0x6C,
		0x04, 0x68,
		0x04, 0x58,
		0x04, 0x40,
		0x04, 0x40,
		0x04, 0x40,
		0x07, 0xC0,
	},
	{
		0x00, 0x00,
		0x03, 0x80,
		0x03, 0x80,
		0x03, 0x80,
		0x03, 0x90,
		0x03, 0x90,
		0x03, 0xF8,
		0x03, 0xFC,
		0x03, 0xF8,
		0x03, 0x90,
		0x03, 0x90,
		0x03, 0x80,
		0x03, 0x80,
		0x03, 0x80,
		0x03, 0x80,
		0x00, 0x00,
	}
};

Cursor br = {
	{-11, -11},
	{
		0x00, 0xF8,
		0x00, 0x88,
		0x00, 0x88,
		0x00, 0x88,
		0x00, 0x88,
		0x00, 0x88,
		0x00, 0x88,
		0x00, 0x88,
		0xFF, 0x88,
		0x80, 0x0B,
		0x80, 0x0D,
		0x80, 0x05,
		0xFF, 0xE1,
		0x00, 0x31,
		0x00, 0x41,
		0x00, 0x7F,
	},
	{
		0x00, 0x00,
		0x00, 0x70,
		0x00, 0x70,
		0x00, 0x70,
		0x00, 0x70,
		0x00, 0x70,
		0x00, 0x70,
		0x00, 0x70,
		0x00, 0x70,
		0x7F, 0xF0,
		0x7F, 0xF2,
		0x7F, 0xFA,
		0x00, 0x1E,
		0x00, 0x0E,
		0x00, 0x3E,
		0x00, 0x00,
	}
};

Cursor b = {
	{-7, -7},
	{
		0x00, 0x00,
		0x00, 0x00,
		0x00, 0x00,
		0x00, 0x00,
		0xFF, 0xFF,
		0x80, 0x01,
		0x80, 0x01,
		0x80, 0x01,
		0xFC, 0x7F,
		0x0C, 0x60,
		0x10, 0x10,
		0x1C, 0x70,
		0x06, 0xC0,
		0x03, 0x80,
		0x00, 0x00,
		0x00, 0x00,
	},
	{
		0x00, 0x00,
		0x00, 0x00,
		0x00, 0x00,
		0x00, 0x00,
		0x00, 0x00,
		0x7F, 0xFE,
		0x7F, 0xFE,
		0x7F, 0xFE,
		0x03, 0x80,
		0x03, 0x80,
		0x0F, 0xE0,
		0x03, 0x80,
		0x01, 0x00,
		0x00, 0x00,
		0x00, 0x00,
		0x00, 0x00,
	}
};

Cursor bl = {
	{-4, -11},
	{
		0x1F, 0x00,
		0x11, 0x00,
		0x11, 0x00,
		0x11, 0x00,
		0x11, 0x00,
		0x11, 0x00,
		0x11, 0x00,
		0x11, 0x00,
		0x11, 0xFF,
		0xD0, 0x01,
		0xB0, 0x01,
		0xA0, 0x01,
		0x87, 0xFF,
		0x8C, 0x00,
		0x82, 0x00,
		0xFE, 0x00,
	},
	{
		0x00, 0x00,
		0x0E, 0x00,
		0x0E, 0x00,
		0x0E, 0x00,
		0x0E, 0x00,
		0x0E, 0x00,
		0x0E, 0x00,
		0x0E, 0x00,
		0x0E, 0x00,
		0x0F, 0xFE,
		0x4F, 0xFE,
		0x5F, 0xFE,
		0x78, 0x00,
		0x70, 0x00,
		0x7C, 0x00,
		0x00, 0x00,
	}
};

Cursor l = {
	{-7, -7},
	{
		0x03, 0xE0,
		0x02, 0x20,
		0x02, 0x20,
		0x1A, 0x20,
		0x16, 0x20,
		0x36, 0x20,
		0x60, 0x20,
		0x40, 0x20,
		0x60, 0x20,
		0x36, 0x20,
		0x16, 0x20,
		0x1A, 0x20,
		0x02, 0x20,
		0x02, 0x20,
		0x02, 0x20,
		0x03, 0xE0,
	},
	{
		0x00, 0x00,
		0x01, 0xC0,
		0x01, 0xC0,
		0x01, 0xC0,
		0x09, 0xC0,
		0x09, 0xC0,
		0x1F, 0xC0,
		0x3F, 0xC0,
		0x1F, 0xC0,
		0x09, 0xC0,
		0x09, 0xC0,
		0x01, 0xC0,
		0x01, 0xC0,
		0x01, 0xC0,
		0x01, 0xC0,
		0x00, 0x00,
	}
};

Cursor *corners[9] = {
	&tl, &t,  &tr,
	&l,  nil, &r,
	&bl, &b,  &br,
};

void
iconinit(void)
{
	background = alloccolor(CLBACKGROUND, RGB24);
	sizecol = alloccolor(CLSIZEBORDER, RGB24);
	bordcol11 = alloccolor(CLBORDER11, RGB24);
	bordcol01 = alloccolor(CLBORDER01, RGB24);
	bordcol10 = alloccolor(CLBORDER10, RGB24);
	bordcol00 = alloccolor(CLBORDER00, RGB24);
	textcol11 = alloccolor(CLWINDOWTEXT11, RGB24);
	textcol01 = alloccolor(CLWINDOWTEXT01, RGB24);
	textcol10 = alloccolor(CLWINDOWTEXT10, RGB24);
	textcol00 = alloccolor(CLWINDOWTEXT00, RGB24);
	highcol11 = alloccolor(CLHIGHLIGHT11, RGB24);
	highcol01 = alloccolor(CLHIGHLIGHT01, RGB24);
	highcol10 = alloccolor(CLHIGHLIGHT10, RGB24);
	highcol00 = alloccolor(CLHIGHLIGHT00, RGB24);
	htextcol11 = alloccolor(CLHIGHLIGHTTEXT11, RGB24);
	htextcol01 = alloccolor(CLHIGHLIGHTTEXT01, RGB24);
	htextcol10 = alloccolor(CLHIGHLIGHTTEXT10, RGB24);
	htextcol00 = alloccolor(CLHIGHLIGHTTEXT00, RGB24);
	cols[BACK] = alloccolor(CLWINDOW, RGB24);
	cols[HIGH] = highcol11;
	cols[BORD] = alloccolor(CLSCROLLBAR, RGB24);
	cols[TEXT] = textcol11;
	cols[HTEXT] = htextcol11;
}
