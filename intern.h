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

#ifndef __INTERN_H__
#define __INTERN_H__

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>

#include "endian.h"
#include "util.h"

#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

template<typename T>
inline void SWAP(T &a, T &b) {
	T tmp = a; 
	a = b;
	b = tmp;
}

struct Ptr {
	uint8_t *pc;
	
	uint8_t fetchByte() {
		return *pc++;
	}
	
	uint16_t fetchWord() {
		uint16_t i = READ_BE_UINT16(pc);
		pc += 2;
		return i;
	}
};

struct Point {
	int16_t x, y;

	Point() : x(0), y(0) {}
	Point(int16_t xx, int16_t yy) : x(xx), y(yy) {}
	Point(const Point &p) : x(p.x), y(p.y) {}
};

#endif
