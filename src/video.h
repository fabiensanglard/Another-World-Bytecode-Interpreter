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
	uint16_t id;
	const char *str;
};

struct Polygon {
	enum {
		MAX_POINTS = 50
	};

	uint16_t bbw, bbh;
	uint8_t numPoints;
	Point points[MAX_POINTS];

	void readVertices(const uint8_t *p, uint16_t zoom);
};

struct Resource;
struct Serializer;
struct System;

// This is used to detect the end of  _stringsTableEng and _stringsTableDemo
#define END_OF_STRING_DICTIONARY 0xFFFF 

// Special value when no palette change is necessary
#define NO_PALETTE_CHANGE_REQUESTED 0xFF 



struct Video {
	typedef void (Video::*drawLine)(int16_t x1, int16_t x2, uint8_t col);

	enum {
		VID_PAGE_SIZE  = 320 * 200 / 2
	};

	static const uint8_t _font[];
	static const StrEntry _stringsTableEng[];
	static const StrEntry _stringsTableDemo[];

	Resource *res;
	System *sys;
	


	uint8_t paletteIdRequested, currentPaletteId;
	uint8_t *_pages[4];

	// I am almost sure that:
	// _curPagePtr1 is the work buffer
	// _curPagePtr2 is the background buffer1
	// _curPagePtr3 is the background buffer2
	uint8_t *_curPagePtr1, *_curPagePtr2, *_curPagePtr3;

	Polygon polygon;
	int16_t _hliney;

	//Precomputer division lookup table
	uint16_t _interpTable[0x400];

	Ptr _pData;
	uint8_t *_dataBuf;

	Video(Resource *res, System *stub);
	void init();

	void setDataBuffer(uint8_t *dataBuf, uint16_t offset);
	void readAndDrawPolygon(uint8_t color, uint16_t zoom, const Point &pt);
	void fillPolygon(uint16_t color, uint16_t zoom, const Point &pt);
	void readAndDrawPolygonHierarchy(uint16_t zoom, const Point &pt);
	int32_t calcStep(const Point &p1, const Point &p2, uint16_t &dy);

	void drawString(uint8_t color, uint16_t x, uint16_t y, uint16_t strId);
	void drawChar(uint8_t c, uint16_t x, uint16_t y, uint8_t color, uint8_t *buf);
	void drawPoint(uint8_t color, int16_t x, int16_t y);
	void drawLineBlend(int16_t x1, int16_t x2, uint8_t color);
	void drawLineN(int16_t x1, int16_t x2, uint8_t color);
	void drawLineP(int16_t x1, int16_t x2, uint8_t color);
	uint8_t *getPage(uint8_t page);
	void changePagePtr1(uint8_t page);
	void fillPage(uint8_t page, uint8_t color);
	void copyPage(uint8_t src, uint8_t dst, int16_t vscroll);
	void copyPage(const uint8_t *src);
	uint8_t *allocPage();
	void changePal(uint8_t pal);
	void updateDisplay(uint8_t page);
	
	void saveOrLoad(Serializer &ser);
};

#endif
