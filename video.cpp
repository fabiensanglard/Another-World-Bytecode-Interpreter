/* Raw - Another World Interpreter
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "video.h"
#include "resource.h"
#include "serializer.h"
#include "sys.h"


void Polygon::readVertices(const uint8 *p, uint16 zoom) {
	bbw = (*p++) * zoom / 64;
	bbh = (*p++) * zoom / 64;
	numPoints = *p++;
	assert((numPoints & 1) == 0 && numPoints < MAX_POINTS);

	//Read all points, directly from bytecode segment
	for (int i = 0; i < numPoints; ++i) {
		Point *pt = &points[i];
		pt->x = (*p++) * zoom / 64;
		pt->y = (*p++) * zoom / 64;
	}
}

Video::Video(Resource *resParameter, System *stub) 
	: res(resParameter), sys(stub) {
}

void Video::init() {

	paletteIdRequested = NO_PALETTE_CHANGE_REQUESTED;

	uint8* tmp = (uint8 *)malloc(4*VID_PAGE_SIZE);
	memset(tmp,0,4*VID_PAGE_SIZE);
	
	/*
	for (int i = 0; i < 4; ++i) {
		_pagePtrs[i] = allocPage();
	}
	*/
	for (int i = 0; i < 4; ++i) {
		_pagePtrs[i] = tmp + i * VID_PAGE_SIZE;
	}

	_curPagePtr3 = getPagePtr(1);
	_curPagePtr2 = getPagePtr(2);


	changePagePtr1(0xFE);

	_interpTable[0] = 0x4000;

	for (int i = 1; i < 0x400; ++i) {
		_interpTable[i] = 0x4000 / i;
	}
}

/*
	This
*/
void Video::setDataBuffer(uint8 *dataBuf, uint16 offset) {

	_dataBuf = dataBuf;
	_pData.pc = dataBuf + offset;
}


/*  A shape can be given in two different ways:

	 - A list of screenspace vertices.
	 - A list of objectspace vertices, based on a delta from the first vertex.

	 This is a recursive function. */
void Video::readAndDrawPolygon(uint8 color, uint16 zoom, const Point &pt) {

	uint8 i = _pData.fetchByte();

	//This is 
	if (i >= 0xC0) {	// 0xc0 = 192

		// WTF ?
		if (color & 0x80) {   //0x80 = 128 (1000 0000)
			color = i & 0x3F; //0x3F =  63 (0011 1111)   
		}

		// pc is misleading here since we are not reading bytecode but only
		// vertices informations.
		polygon.readVertices(_pData.pc, zoom);

		fillPolygon(color, zoom, pt);
#if TRACE_FRAMEBUFFER
		//	dumpFrameBuffers();
#endif


	} else {
		i &= 0x3F;  //0x3F = 63
		if (i == 1) {
			warning("Video::readAndDrawPolygon() ec=0x%X (i != 2)", 0xF80);
		} else if (i == 2) {
			readAndDrawPolygonHierarchy(zoom, pt);
#if TRACE_FRAMEBUFFER
		//	dumpFrameBuffers();
#endif
		} else {
			warning("Video::readAndDrawPolygon() ec=0x%X (i != 2)", 0xFBB);
		}
	}

#if TRACE_FRAMEBUFFER
		//	dumpFrameBuffers();
#endif

}

void Video::fillPolygon(uint16 color, uint16 zoom, const Point &pt) {

	if (polygon.bbw == 0 && polygon.bbh == 1 && polygon.numPoints == 4) {
		drawPoint(color, pt.x, pt.y);
#if TRACE_FRAMEBUFFER
			//dumpFrameBuffers();
#endif
		return;
	}
	
	int16 x1 = pt.x - polygon.bbw / 2;
	int16 x2 = pt.x + polygon.bbw / 2;
	int16 y1 = pt.y - polygon.bbh / 2;
	int16 y2 = pt.y + polygon.bbh / 2;

	if (x1 > 319 || x2 < 0 || y1 > 199 || y2 < 0)
		return;

	_hliney = y1;
	
	uint16 i, j;
	i = 0;
	j = polygon.numPoints - 1;
	
	x2 = polygon.points[i].x + x1;
	x1 = polygon.points[j].x + x1;

	++i;
	--j;

	drawLine drawFct;
	if (color < 0x10) {
		drawFct = &Video::drawLineN;
	} else if (color > 0x10) {
		drawFct = &Video::drawLineP;
	} else {
		drawFct = &Video::drawLineBlend;
	}

	uint32 cpt1 = x1 << 16;
	uint32 cpt2 = x2 << 16;

	while (1) {
		polygon.numPoints -= 2;
		if (polygon.numPoints == 0) {
#if TRACE_FRAMEBUFFER
				dumpFrameBuffers("fillPolygonEnd");
		#endif
			break;
		}
		uint16 h;
		int32 step1 = calcStep(polygon.points[j + 1], polygon.points[j], h);
		int32 step2 = calcStep(polygon.points[i - 1], polygon.points[i], h);

		++i;
		--j;

		cpt1 = (cpt1 & 0xFFFF0000) | 0x7FFF;
		cpt2 = (cpt2 & 0xFFFF0000) | 0x8000;

		if (h == 0) {	
			cpt1 += step1;
			cpt2 += step2;
		} else {
			for (; h != 0; --h) {
				if (_hliney >= 0) {
					x1 = cpt1 >> 16;
					x2 = cpt2 >> 16;
					if (x1 <= 319 && x2 >= 0) {
						if (x1 < 0) x1 = 0;
						if (x2 > 319) x2 = 319;
						(this->*drawFct)(x1, x2, color);
					}
				}
				cpt1 += step1;
				cpt2 += step2;
				++_hliney;					
				if (_hliney > 199) break;
			}
		}

		#if TRACE_FRAMEBUFFER
				dumpFrameBuffers("fillPolygonChild");
		#endif
	}





}

/*
    What is read from the bytecode is not a pure screnspace polygon but a polygonspace polygon.

*/
void Video::readAndDrawPolygonHierarchy(uint16 zoom, const Point &pgc) {

	char nullChar;

	Point pt(pgc);
	pt.x -= _pData.fetchByte() * zoom / 64;
	pt.y -= _pData.fetchByte() * zoom / 64;

	int16 childs = _pData.fetchByte();
	debug(DBG_VIDEO, "Video::readAndDrawPolygonHierarchy childs=%d", childs);

	for ( ; childs >= 0; --childs) {

		uint16 off = _pData.fetchWord();

		Point po(pt);
		po.x += _pData.fetchByte() * zoom / 64;
		po.y += _pData.fetchByte() * zoom / 64;

		uint16 color = 0xFF;
		uint16 _bp = off;
		off &= 0x7FFF;

		if (_bp & 0x8000) {
			color = *_pData.pc & 0x7F;
			_pData.pc += 2;
		}

		uint8 *bak = _pData.pc;
		_pData.pc = _dataBuf + off * 2;


		readAndDrawPolygon(color, zoom, po);


		_pData.pc = bak;
	}

			#if TRACE_FRAMEBUFFER
		//	dumpFrameBuffers();
#endif
}

int32 Video::calcStep(const Point &p1, const Point &p2, uint16 &dy) {
	dy = p2.y - p1.y;
	return (p2.x - p1.x) * _interpTable[dy] * 4;
}

void Video::drawString(uint8 color, uint16 x, uint16 y, uint16 stringId) {

	const StrEntry *se = _stringsTableEng;

	//Search for the location where the string is located.
	while (se->id != END_OF_STRING_DICTIONARY && se->id != stringId) 
		++se;
	
	debug(DBG_VIDEO, "drawString(%d, %d, %d, '%s')", color, x, y, se->str);

	//Not found
	if (se->id == END_OF_STRING_DICTIONARY)
		return;
	

    //Used if the string contains a return carriage.
	uint16 xOrigin = x;
	int len = strlen(se->str);
	for (int i = 0; i < len; ++i) {

		if (se->str[i] == '\n') {
			y += 8;
			x = xOrigin;
			continue;
		} 
		
		drawChar(se->str[i], x, y, color, _curPagePtr1);
		x++;
		
	}
#if TRACE_FRAMEBUFFER
//			dumpFrameBuffers();
#endif
}

void Video::drawChar(uint8 character, uint16 x, uint16 y, uint8 color, uint8 *buf) {
	if (x <= 39 && y <= 192) {
		
		const uint8 *ft = _font + (character - ' ') * 8;

		uint8 *p = buf + x * 4 + y * 160;

		for (int j = 0; j < 8; ++j) {
			uint8 ch = *(ft + j);
			for (int i = 0; i < 4; ++i) {
				uint8 b = *(p + i);
				uint8 cmask = 0xFF;
				uint8 colb = 0;
				if (ch & 0x80) {
					colb |= color << 4;
					cmask &= 0x0F;
				}
				ch <<= 1;
				if (ch & 0x80) {
					colb |= color;
					cmask &= 0xF0;
				}
				ch <<= 1;
				*(p + i) = (b & cmask) | colb;
			}
			p += 160;
		}
	}
}

void Video::drawPoint(uint8 color, int16 x, int16 y) {
	debug(DBG_VIDEO, "drawPoint(%d, %d, %d)", color, x, y);
	if (x >= 0 && x <= 319 && y >= 0 && y <= 199) {
		uint16 off = y * 160 + x / 2;
	
		uint8 cmasko, cmaskn;
		if (x & 1) {
			cmaskn = 0x0F;
			cmasko = 0xF0;
		} else {
			cmaskn = 0xF0;
			cmasko = 0x0F;
		}

		uint8 colb = (color << 4) | color;
		if (color == 0x10) {
			cmaskn &= 0x88;
			cmasko = ~cmaskn;
			colb = 0x88;		
		} else if (color == 0x11) {
			colb = *(_pagePtrs[0] + off);
		}
		uint8 b = *(_curPagePtr1 + off);
		*(_curPagePtr1 + off) = (b & cmasko) | (colb & cmaskn);
	}
}

/* Blend a line in the current framebuffer (_curPagePtr1)
*/
void Video::drawLineBlend(int16 x1, int16 x2, uint8 color) {
	debug(DBG_VIDEO, "drawLineBlend(%d, %d, %d)", x1, x2, color);
	int16 xmax = MAX(x1, x2);
	int16 xmin = MIN(x1, x2);
	uint8 *p = _curPagePtr1 + _hliney * 160 + xmin / 2;

	uint16 w = xmax / 2 - xmin / 2 + 1;
	uint8 cmaske = 0;
	uint8 cmasks = 0;	
	if (xmin & 1) {
		--w;
		cmasks = 0xF7;
	}
	if (!(xmax & 1)) {
		--w;
		cmaske = 0x7F;
	}

	if (cmasks != 0) {
		*p = (*p & cmasks) | 0x08;
		++p;
	}
	while (w--) {
		*p = (*p & 0x77) | 0x88;
		++p;
	}
	if (cmaske != 0) {
		*p = (*p & cmaske) | 0x80;
		++p;
	}

	#if TRACE_FRAMEBUFFER
	//	dumpFrameBuffers();
#endif
}

void Video::drawLineN(int16 x1, int16 x2, uint8 color) {
	debug(DBG_VIDEO, "drawLineN(%d, %d, %d)", x1, x2, color);
	int16 xmax = MAX(x1, x2);
	int16 xmin = MIN(x1, x2);
	uint8 *p = _curPagePtr1 + _hliney * 160 + xmin / 2;

	uint16 w = xmax / 2 - xmin / 2 + 1;
	uint8 cmaske = 0;
	uint8 cmasks = 0;	
	if (xmin & 1) {
		--w;
		cmasks = 0xF0;
	}
	if (!(xmax & 1)) {
		--w;
		cmaske = 0x0F;
	}

	uint8 colb = ((color & 0xF) << 4) | (color & 0xF);	
	if (cmasks != 0) {
		*p = (*p & cmasks) | (colb & 0x0F);
		++p;
	}
	while (w--) {
		*p++ = colb;
	}
	if (cmaske != 0) {
		*p = (*p & cmaske) | (colb & 0xF0);
		++p;		
	}

		#if TRACE_FRAMEBUFFER
	//	dumpFrameBuffers();
#endif
}

void Video::drawLineP(int16 x1, int16 x2, uint8 color) {
	debug(DBG_VIDEO, "drawLineP(%d, %d, %d)", x1, x2, color);
	int16 xmax = MAX(x1, x2);
	int16 xmin = MIN(x1, x2);
	uint16 off = _hliney * 160 + xmin / 2;
	uint8 *p = _curPagePtr1 + off;
	uint8 *q = _pagePtrs[0] + off;

	uint8 w = xmax / 2 - xmin / 2 + 1;
	uint8 cmaske = 0;
	uint8 cmasks = 0;	
	if (xmin & 1) {
		--w;
		cmasks = 0xF0;
	}
	if (!(xmax & 1)) {
		--w;
		cmaske = 0x0F;
	}

	if (cmasks != 0) {
		*p = (*p & cmasks) | (*q & 0x0F);
		++p;
		++q;
	}
	while (w--) {
		*p++ = *q++;			
	}
	if (cmaske != 0) {
		*p = (*p & cmaske) | (*q & 0xF0);
		++p;
		++q;
	}

		#if TRACE_FRAMEBUFFER
	//		dumpFrameBuffers();
#endif
}

uint8 *Video::getPagePtr(uint8 page) {
	uint8 *p;
	if (page <= 3) {
		p = _pagePtrs[page];
	} else {
		switch (page) {
		case 0xFF:
			p = _curPagePtr3;
			break;
		case 0xFE:
			p = _curPagePtr2;
			break;
		default:
			p = _pagePtrs[0]; // XXX check
			warning("Video::getPagePtr() p != [0,1,2,3,0xFF,0xFE] == 0x%X", page);
			break;
		}
	}
	return p;
}



void Video::changePagePtr1(uint8 page) {
	debug(DBG_VIDEO, "Video::changePagePtr1(%d)", page);
	_curPagePtr1 = getPagePtr(page);
}



void Video::fillPage(uint8 pageId, uint8 color) {
	debug(DBG_VIDEO, "Video::fillPage(%d, %d)", pageId, color);
	uint8 *p = getPagePtr(pageId);

	// Since a palette indice is coded on 4 bits, we need to duplicate the
	// clearing color to the upper part of the byte.
	uint8 c = (color << 4) | color;


	memset(p, c, VID_PAGE_SIZE);

#if TRACE_FRAMEBUFFER
			dumpFrameBuffers("-fillPage");
#endif
}





#if TRACE_FRAMEBUFFER
		#define SCREENSHOT_BPP 3
int traceFrameBufferCounter=0;
uint8 allFrameBuffers[640*400*SCREENSHOT_BPP];

#endif






/*  This opcode is used once the background of a scene has been drawn in one of the framebuffer:
	   it is copied in the current framebuffer at the start of a new frame in order to improve performances. */
void Video::copyPage(uint8 srcPageId, uint8 dstPageId, int16 vscroll) {

	debug(DBG_VIDEO, "Video::copyPage(%d, %d)", srcPageId, dstPageId);

	if (srcPageId == dstPageId)
		return;

	uint8 *p;
	uint8 *q;

	if (srcPageId >= 0xFE || !((srcPageId &= 0xBF) & 0x80)) {
		p = getPagePtr(srcPageId);
		q = getPagePtr(dstPageId);
		memcpy(q, p, VID_PAGE_SIZE);
			
	} else {
		p = getPagePtr(srcPageId & 3);
		q = getPagePtr(dstPageId);
		if (vscroll >= -199 && vscroll <= 199) {
			uint16 h = 200;
			if (vscroll < 0) {
				h += vscroll;
				p += -vscroll * 160;
			} else {
				h -= vscroll;
				q += vscroll * 160;
			}
			memcpy(q, p, h * 160);
		}
	}

	
	#if TRACE_FRAMEBUFFER
	char name[256];
	memset(name,0,sizeof(name));
	sprintf(name,"copyPage_0x%X_to_0x%X",(p-_pagePtrs[0])/VID_PAGE_SIZE,(q-_pagePtrs[0])/VID_PAGE_SIZE);
	dumpFrameBuffers(name);
	#endif
}




void Video::copyPagePtr(const uint8 *src) {
	debug(DBG_VIDEO, "Video::copyPagePtr()");
	uint8 *dst = _pagePtrs[0];
	int h = 200;
	while (h--) {
		int w = 40;
		while (w--) {
			uint8 p[] = {
				*(src + 8000 * 3),
				*(src + 8000 * 2),
				*(src + 8000 * 1),
				*(src + 8000 * 0)
			};
			for(int j = 0; j < 4; ++j) {
				uint8 acc = 0;
				for (int i = 0; i < 8; ++i) {
					acc <<= 1;
					acc |= (p[i & 3] & 0x80) ? 1 : 0;
					p[i & 3] <<= 1;
				}
				*dst++ = acc;
			}
			++src;
		}
	}


}

/*
uint8 *Video::allocPage() {
	uint8 *buf = (uint8 *)malloc(VID_PAGE_SIZE);
	memset(buf, 0, VID_PAGE_SIZE);
	return buf;
}
*/



#if TRACE_FRAMEBUFFER
	int dumpPaletteCursor=0;
#endif

/*
Note: The palettes set used to be allocated on the stack but I moved it to
      the heap so I could dump the four framebuffer and follow how
	  frames are generated.
*/
uint8 pal[NUM_COLORS * 3]; //3 = BYTES_PER_PIXEL
void Video::changePal(uint8 palNum) {

	if (palNum >= 32)
		return;
	
	uint8 *p = res->segPalettes + palNum * 32; //colors are coded on 2bytes (565) for 16 colors = 32

	// Moved to the heap, legacy code used to allocate the palette
	// on the stack.
	//uint8 pal[NUM_COLORS * 3]; //3 = BYTES_PER_PIXEL

	for (int i = 0; i < NUM_COLORS; ++i) 
	{
		uint8 c1 = *(p + 0);
		uint8 c2 = *(p + 1);
		p += 2;
		pal[i * 3 + 0] = ((c1 & 0x0F) << 2) | ((c1 & 0x0F) >> 2); // r
		pal[i * 3 + 1] = ((c2 & 0xF0) >> 2) | ((c2 & 0xF0) >> 6); // g
		pal[i * 3 + 2] = ((c2 & 0x0F) >> 2) | ((c2 & 0x0F) << 2); // b
	}

	sys->setPalette(0, NUM_COLORS, pal);
	currentPaletteId = palNum;


	#if TRACE_PALETTE
	printf("\nuint8 dumpPalette[48] = {\n");
	for (int i = 0; i < NUM_COLORS; ++i) 
	{
		printf("0x%X,0x%X,0x%X,",pal[i * 3 + 0],pal[i * 3 + 1],pal[i * 3 + 2]);
	}
	printf("\n};\n");
	#endif

	dumpPaletteCursor++;
}

void Video::updateDisplay(uint8 pageId) {

	debug(DBG_VIDEO, "Video::updateDisplay(%d)", pageId);

	if (pageId != 0xFE) {
		if (pageId == 0xFF) {
			SWAP(_curPagePtr2, _curPagePtr3);
		} else {
			_curPagePtr2 = getPagePtr(pageId);
		}
	}

	//Check if we need to change the palette
	if (paletteIdRequested != NO_PALETTE_CHANGE_REQUESTED) {
		changePal(paletteIdRequested);
		paletteIdRequested = NO_PALETTE_CHANGE_REQUESTED;
	}

	//Q: Why 160 ?
	//A: Because one byte gives two palette indices so
	//   we only need to move 320/2 per line.
	sys->copyRect(0, 0, 320, 200, _curPagePtr2, 160);

		#if TRACE_FRAMEBUFFER
	      dumpFrameBuffer(_curPagePtr2,allFrameBuffers,320,200);
#endif
}

void Video::saveOrLoad(Serializer &ser) {
	uint8 mask = 0;
	if (ser._mode == Serializer::SM_SAVE) {
		for (int i = 0; i < 4; ++i) {
			if (_pagePtrs[i] == _curPagePtr1)
				mask |= i << 4;
			if (_pagePtrs[i] == _curPagePtr2)
				mask |= i << 2;
			if (_pagePtrs[i] == _curPagePtr3)
				mask |= i << 0;
		}		
	}
	Serializer::Entry entries[] = {
		SE_INT(&currentPaletteId, Serializer::SES_INT8, VER(1)),
		SE_INT(&paletteIdRequested, Serializer::SES_INT8, VER(1)),
		SE_INT(&mask, Serializer::SES_INT8, VER(1)),
		SE_ARRAY(_pagePtrs[0], Video::VID_PAGE_SIZE, Serializer::SES_INT8, VER(1)),
		SE_ARRAY(_pagePtrs[1], Video::VID_PAGE_SIZE, Serializer::SES_INT8, VER(1)),
		SE_ARRAY(_pagePtrs[2], Video::VID_PAGE_SIZE, Serializer::SES_INT8, VER(1)),
		SE_ARRAY(_pagePtrs[3], Video::VID_PAGE_SIZE, Serializer::SES_INT8, VER(1)),
		SE_END()
	};
	ser.saveOrLoadEntries(entries);

	if (ser._mode == Serializer::SM_LOAD) {
		_curPagePtr1 = _pagePtrs[(mask >> 4) & 0x3];
		_curPagePtr2 = _pagePtrs[(mask >> 2) & 0x3];
		_curPagePtr3 = _pagePtrs[(mask >> 0) & 0x3];
		changePal(currentPaletteId);
	}
}



#if TRACE_FRAMEBUFFER


uint8 allPalettesDump[][48] = {


{
0x4,0x4,0x4,0x22,0x0,0x0,0x4,0x8,0x11,0x4,0xC,0x15,0x8,0x11,0x19,0xC,0x15,0x1D,0x15,0x1D,0x26,0x1D,0x2A,0x2E,0x2E,0x22,0x0,0x3F,0x0,0x0,0x33,0x26,0x0,0x37,0x2A,0x0,0x3B,0x33,0x0,0x3F,0x3B,0x0,0x3F,0x3F,0x1D,0x3F,0x3F,0x2A,
},

{
0x4,0x4,0x4,0xC,0xC,0x11,0x4,0x8,0x11,0x4,0xC,0x15,0x8,0x11,0x19,0xC,0x15,0x1D,0x15,0x1D,0x26,0x1D,0x2A,0x2E,0x2E,0x22,0x0,0x15,0x11,0x11,0x33,0x26,0x0,0x37,0x2A,0x0,0x3B,0x33,0x0,0x3F,0x3B,0x0,0x3F,0x3F,0x1D,0x3F,0x3F,0x2A,
}

,{
0x0,0x0,0x0,0x22,0x0,0x0,0x4,0x8,0x11,0x4,0xC,0x15,0x8,0x11,0x19,0xC,0x15,0x1D,0x15,0x1D,0x26,0x1D,0x2A,0x2E,0x1D,0x1D,0x1D,0x15,0x15,0x15,0xC,0x8,0xC,0x11,0x11,0x15,0x1D,0x15,0x15,0x15,0x0,0x0,0x0,0x4,0xC,0x3F,0x3F,0x2A,
}

,{
0x0,0x0,0x0,0x22,0x0,0x0,0x4,0x8,0x11,0x4,0xC,0x15,0x8,0x11,0x19,0xC,0x15,0x1D,0x15,0x1D,0x26,0x1D,0x2A,0x2E,0x1D,0x1D,0x1D,0x15,0x15,0x15,0xC,0x8,0xC,0x11,0x11,0x15,0x1D,0x15,0x15,0x15,0x0,0x0,0x0,0x4,0xC,0x3F,0x3F,0x2A,
}

,{
0x0,0x0,0x0,0x1D,0x0,0x0,0x4,0x8,0x11,0x4,0xC,0x15,0x8,0x11,0x19,0xC,0x15,0x1D,0x15,0x1D,0x26,0x1D,0x2A,0x2E,0x1D,0x1D,0x1D,0x15,0x15,0x15,0xC,0x8,0xC,0x15,0x11,0x19,0x1D,0x15,0x15,0x15,0x0,0x0,0x0,0x4,0xC,0x3F,0x3F,0x2A,
}

,{
0x0,0x4,0x8,0x15,0x1D,0x1D,0x0,0x8,0x11,0x4,0xC,0x15,0x8,0x11,0x19,0xC,0x15,0x1D,0xC,0x19,0x22,0x11,0x1D,0x26,0x8,0x8,0x8,0x0,0x0,0x0,0x2E,0x2E,0x2E,0xC,0xC,0xC,0x15,0xC,0x15,0xC,0x15,0x15,0x11,0x19,0x19,0x1D,0x26,0x26,
}

,{
0x0,0x0,0x0,0x0,0x4,0xC,0x4,0x8,0x11,0x4,0xC,0x15,0x8,0x11,0x19,0xC,0x15,0x1D,0x15,0x1D,0x26,0x1D,0x2A,0x2E,0x15,0x0,0x0,0xC,0xC,0xC,0xC,0xC,0xC,0xC,0x11,0x11,0x11,0x11,0x15,0x26,0x0,0x0,0x33,0x26,0x0,0x3B,0x33,0x11,
}

,{
0x0,0x0,0x0,0x0,0x4,0xC,0x0,0x8,0x11,0x4,0xC,0x15,0x8,0x11,0x19,0xC,0x15,0x1D,0x15,0x1D,0x26,0x1D,0x2A,0x2E,0x15,0x0,0x0,0xC,0xC,0xC,0xC,0xC,0xC,0xC,0x11,0x11,0x11,0x11,0x15,0x26,0x0,0x0,0x26,0x15,0x0,0x26,0x1D,0x0,
}

,{
0x0,0x0,0x0,0x0,0x4,0xC,0x0,0x8,0x11,0x4,0xC,0x15,0x8,0x11,0x19,0xC,0x15,0x1D,0x15,0x1D,0x26,0x1D,0x2A,0x2E,0x15,0x0,0x0,0xC,0xC,0xC,0xC,0xC,0xC,0xC,0x11,0x11,0x11,0x11,0x15,0x26,0x0,0x0,0x3F,0x0,0x0,0x26,0x1D,0x0,
}

,{
0x0,0x0,0x0,0x8,0x4,0xC,0x15,0xC,0x11,0x1D,0x11,0x0,0xC,0x8,0x11,0x2E,0x1D,0x0,0x37,0x26,0x8,0x3F,0x2E,0x0,0x0,0x0,0x0,0x11,0xC,0x15,0x26,0x15,0x0,0x15,0x11,0x19,0x1D,0x15,0x1D,0x26,0x19,0x19,0x0,0x0,0x0,0x3F,0x3F,0x3F,
}

,{
0x0,0x0,0x0,0x8,0x4,0xC,0x37,0x1D,0x1D,0x3B,0x2A,0x22,0x11,0xC,0x15,0x2A,0x0,0x0,0x33,0x11,0x0,0x3F,0x33,0x1D,0x3B,0x19,0x0,0x11,0x11,0x19,0x19,0x15,0x1D,0x22,0x19,0x22,0x2A,0x1D,0x26,0x33,0x22,0x26,0x37,0x26,0x22,0x1D,0x37,0x3F,
}

,{
0x0,0x0,0x0,0x0,0x0,0x1D,0x4,0x8,0xC,0x2A,0x1D,0xC,0x3F,0x3B,0x26,0x3B,0x2A,0x11,0x2A,0x0,0x0,0x0,0x11,0x15,0x2A,0x1D,0x26,0xC,0x8,0xC,0x8,0x15,0x1D,0x37,0x26,0x22,0x33,0x11,0x0,0x2E,0x1D,0x19,0x22,0x0,0x0,0x3F,0x33,0x1D,
}

,{
0x0,0x0,0x0,0x0,0x15,0x0,0x4,0x8,0xC,0x0,0xC,0x11,0x8,0x11,0x19,0x0,0x19,0x22,0x2A,0x0,0x0,0x19,0x15,0x22,0x2E,0x26,0x2E,0xC,0x8,0xC,0x0,0x2E,0x0,0x37,0x26,0x22,0x33,0x11,0x0,0x2E,0x1D,0x19,0x1D,0x0,0x0,0x3F,0x3F,0x19,
}

,{
0x0,0x0,0x0,0x8,0xC,0x11,0x11,0x11,0x15,0x19,0x15,0x22,0x26,0x19,0x2A,0x2E,0x26,0x2E,0x4,0x4,0xC,0x0,0xC,0x15,0x2A,0x1D,0x26,0x0,0x19,0x0,0x0,0x2A,0x0,0x37,0x26,0x22,0x0,0x15,0x1D,0x37,0x2E,0x1D,0x3F,0x3F,0x2E,0x37,0x37,0x26,
}

,{
0x0,0x0,0x0,0x0,0x15,0x0,0x4,0x8,0xC,0x0,0xC,0x11,0x8,0x11,0x19,0x0,0x19,0x22,0x2A,0x0,0x0,0x19,0x15,0x22,0x2E,0x26,0x2E,0xC,0x8,0xC,0x0,0x2E,0x0,0x37,0x26,0x22,0x33,0x11,0x0,0x2E,0x1D,0x19,0x1D,0x0,0x0,0x3F,0x3F,0x19,
}

,{
0x0,0x0,0x0,0x0,0xC,0x0,0x8,0x0,0x4,0xC,0x8,0xC,0x19,0x11,0x11,0x3F,0x3F,0x19,0x0,0x1D,0x0,0x0,0x3F,0x0,0x0,0x4,0x8,0x11,0x19,0x11,0x19,0x0,0x0,0x2E,0x0,0x0,0x3F,0x0,0x0,0x37,0x1D,0x15,0x0,0x11,0x11,0x0,0x3F,0x2A,
}

,{
0x0,0x0,0x0,0x0,0xC,0x0,0x0,0x11,0x0,0x0,0x19,0x0,0xC,0x26,0x0,0x15,0x2E,0x0,0x4,0x8,0x0,0x26,0x11,0x0,0x2E,0x3F,0x0,0x0,0x15,0x0,0xC,0x1D,0x0,0x15,0x2E,0x0,0x15,0x37,0x0,0x1D,0x3F,0x0,0xC,0x1D,0xC,0x\
1D,0x26,0x15,
}
};



#include "png.h"
int GL_FCS_SaveAsSpecifiedPNG(char* path, uint8* pixels, int depth=8, int format=PNG_COLOR_TYPE_RGB)
{
	FILE * fp;
	png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_byte ** row_pointers = NULL;
	int status = -1;
	int bytePerPixel=0;
    int y;

    fp = fopen (path, "wb");
    if (! fp) {
        goto fopen_failed;
    }

	png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        goto png_create_write_struct_failed;
    }
    
    info_ptr = png_create_info_struct (png_ptr);
    if (info_ptr == NULL) {
        goto png_create_info_struct_failed;
    }

	if (setjmp (png_jmpbuf (png_ptr))) {
        goto png_failure;
    }
    
    /* Set image attributes. */

    png_set_IHDR (png_ptr,
                  info_ptr,
                  640,
                  400,
                  depth,
                  format,
                  PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT);

	if (format == PNG_COLOR_TYPE_GRAY	)
		bytePerPixel = depth/8 * 1;
	else
		bytePerPixel = depth/8 * 3;

	row_pointers = (png_byte **)png_malloc (png_ptr, 400 * sizeof (png_byte *));
	//for (y = vid.height-1; y >=0; --y) 
	for (y = 0; y < 400; y++) 
    {
		row_pointers[y] = (png_byte*)&pixels[640*(400-y)*bytePerPixel];
	}

	png_init_io (png_ptr, fp);
    png_set_rows (png_ptr, info_ptr, row_pointers);
    png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	//png_read_image (png_ptr, info_ptr);//

	status = 0;
    
    png_free (png_ptr, row_pointers);
    

	png_failure:   
    png_create_info_struct_failed:
			png_destroy_write_struct (&png_ptr, &info_ptr); 

    png_create_write_struct_failed:
		fclose (fp);
	fopen_failed:
		return status;
}








void writeLine(uint8 *dst,uint8 *src,int size)
{
	uint8* dumpPalette; 

	if (!dumpPaletteCursor)
		dumpPalette = allPalettesDump[dumpPaletteCursor];
	else
	  dumpPalette = pal;

	for( uint8 twoPixels = 0 ; twoPixels < size ; twoPixels++)
	{
		int pixelIndex0 = (*src & 0xF0) >> 4;
		pixelIndex0 &= 0x10 -1;

		int pixelIndex1 = (*src & 0xF);
		pixelIndex1 &= 0x10 -1;

		//We need to write those two pixels
		dst[0] = dumpPalette[pixelIndex0*3] << 2 | dumpPalette[pixelIndex0*3];
		dst[1] = dumpPalette[pixelIndex0*3+1] << 2 | dumpPalette[pixelIndex0*3+1];
		dst[2] = dumpPalette[pixelIndex0*3+2] << 2 | dumpPalette[pixelIndex0*3+2];
		//dst[3] = 0xFF;
		dst+=SCREENSHOT_BPP;

		dst[0] = dumpPalette[pixelIndex1*3] << 2 | dumpPalette[pixelIndex1*3];
		dst[1] = dumpPalette[pixelIndex1*3+1] << 2 | dumpPalette[pixelIndex1*3+1];
		dst[2] = dumpPalette[pixelIndex1*3+2] << 2 | dumpPalette[pixelIndex1*3+2];
		//dst[3] = 0xFF;
		dst+=SCREENSHOT_BPP;

		src++;
	}
}

void Video::dumpFrameBuffer(uint8 *src,uint8 *dst, int x,int y)
{

	for (int line=199 ; line >= 0 ; line--)
	{
		writeLine(dst + x*SCREENSHOT_BPP + y*640*SCREENSHOT_BPP  ,src+line*160,160);
		dst+= 640*SCREENSHOT_BPP;
	}
}

void Video::dumpFrameBuffers(char* comment)
{
	
	if (!traceFrameBufferCounter)
	{
		memset(allFrameBuffers,0,sizeof(allFrameBuffers));
	}


	dumpFrameBuffer(_pagePtrs[1],allFrameBuffers,0,0);
	dumpFrameBuffer(_pagePtrs[0],allFrameBuffers,0,200);
	dumpFrameBuffer(_pagePtrs[2],allFrameBuffers,320,0);
	//dumpFrameBuffer(_pagePtrs[3],allFrameBuffers,320,200);


	//if (_curPagePtr1 == _pagePtrs[3])
	//
	
	/*
	uint8* offScreen = sys->getOffScreenFramebuffer();
	for(int i=0 ; i < 200 ; i++)
		writeLine(allFrameBuffers+320*3+640*i*3 + 200*640*3,  offScreen+320*i/2  ,  160);
		*/
    
	  
	int frameId = traceFrameBufferCounter++;
	//Write bitmap to disk.

	

	// Filling TGA header information
	/*
	char path[256];
	sprintf(path,"test%d.tga",traceFrameBufferCounter);

#define IMAGE_WIDTH 640
#define IMAGE_HEIGHT 400

    uint8 tga_header[18];
    memset(tga_header, 0, 18);
    tga_header[2] = 2;
    tga_header[12] = (IMAGE_WIDTH & 0x00FF);
    tga_header[13] = (IMAGE_WIDTH  & 0xFF00) / 256;
    tga_header[14] = (IMAGE_HEIGHT  & 0x00FF) ;
    tga_header[15] =(IMAGE_HEIGHT & 0xFF00) / 256;
    tga_header[16] = 32 ;


	
    // Open the file, write both header and payload, close, done.
	char path[256];
	sprintf(path,"test%d.tga",traceFrameBufferCounter);
    FILE* pScreenshot = fopen(path, "wb");
    fwrite(&tga_header, 18, sizeof(uint8), pScreenshot);
    fwrite(allFrameBuffers, IMAGE_WIDTH * IMAGE_HEIGHT,SCREENSHOT_BPP * sizeof(uint8),pScreenshot);
    fclose(pScreenshot);
	*/

	
	char path[256];
	//sprintf(path,"%4d%s.png",traceFrameBufferCounter,comment);
	sprintf(path,"%4d.png",traceFrameBufferCounter);

	GL_FCS_SaveAsSpecifiedPNG(path,allFrameBuffers);
}


#endif