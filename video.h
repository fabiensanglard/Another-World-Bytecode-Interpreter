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

#ifndef __VIDEO_H__
#define __VIDEO_H__

#include "intern.h"

struct StrEntry {
	uint16 id;
	const char *str;
};

struct Polygon {
	enum {
		MAX_POINTS = 50
	};

	uint16 bbw, bbh;
	uint8 numPoints;
	Point points[MAX_POINTS];

	void readVertices(const uint8 *p, uint16 zoom);
};

struct Resource;
struct Serializer;
struct System;

// This is used to detect the end of  _stringsTableEng and _stringsTableDemo
#define END_OF_STRING_DICTIONARY 0xFFFF 

// Special value when no palette change is necessary
#define NO_PALETTE_CHANGE_REQUESTED 0xFF 



struct Video {
	typedef void (Video::*drawLine)(int16 x1, int16 x2, uint8 col);

	enum {
		VID_PAGE_SIZE  = 320 * 200 / 2
	};

	static const uint8 _font[];
	static const StrEntry _stringsTableEng[];
	static const StrEntry _stringsTableDemo[];

	Resource *res;
	System *sys;
	


	uint8 paletteIdRequested, currentPaletteId;
	uint8 *_pagePtrs[4];

	// I am almost sure that:
	// _curPagePtr1 is the backbuffer 
	// _curPagePtr2 is the frontbuffer
	// _curPagePtr3 is the background builder.
	uint8 *_curPagePtr1, *_curPagePtr2, *_curPagePtr3;

	Polygon polygon;
	int16 _hliney;

	//Precomputer division lookup table
	uint16 _interpTable[0x400];

	Ptr _pData;
	uint8 *_dataBuf;

	Video(Resource *res, System *stub);
	void init();

	void setDataBuffer(uint8 *dataBuf, uint16 offset);
	void readAndDrawPolygon(uint8 color, uint16 zoom, const Point &pt);
	void fillPolygon(uint16 color, uint16 zoom, const Point &pt);
	void readAndDrawPolygonHierarchy(uint16 zoom, const Point &pt);
	int32 calcStep(const Point &p1, const Point &p2, uint16 &dy);

	void drawString(uint8 color, uint16 x, uint16 y, uint16 strId);
	void drawChar(uint8 c, uint16 x, uint16 y, uint8 color, uint8 *buf);
	void drawPoint(uint8 color, int16 x, int16 y);
	void drawLineBlend(int16 x1, int16 x2, uint8 color);
	void drawLineN(int16 x1, int16 x2, uint8 color);
	void drawLineP(int16 x1, int16 x2, uint8 color);
	uint8 *getPagePtr(uint8 page);
	void changePagePtr1(uint8 page);
	void fillPage(uint8 page, uint8 color);
	void copyPage(uint8 src, uint8 dst, int16 vscroll);
	void copyPagePtr(const uint8 *src);
	uint8 *allocPage();
	void changePal(uint8 pal);
	void updateDisplay(uint8 page);
	
	void saveOrLoad(Serializer &ser);

	#define TRACE_PALETTE 1
	#define TRACE_FRAMEBUFFER 0
	#if TRACE_FRAMEBUFFER
		void Video::dumpFrameBuffers(char* comment);
	#endif
};

#endif
