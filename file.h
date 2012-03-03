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

#ifndef __FILE_H__
#define __FILE_H__

#include "intern.h"

struct File_impl;

struct File {
	File_impl *_impl;

	File(bool gzipped = false);
	~File();

	bool open(const char *filename, const char *directory, const char *mode="rb");
	void close();
	bool ioErr() const;
	void seek(int32_t off);
	void read(void *ptr, uint32_t size);
	uint8_t readByte();
	uint16_t readUint16BE();
	uint32_t readUint32BE();
	void write(void *ptr, uint32_t size);
	void writeByte(uint8_t b);
	void writeUint16BE(uint16_t n);
	void writeUint32BE(uint32_t n);
};

#endif
