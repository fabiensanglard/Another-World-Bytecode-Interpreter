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

#include "serializer.h"
#include "file.h"


Serializer::Serializer(File *stream, Mode mode, uint8 *ptrBlock, uint16 saveVer)
	: _stream(stream), _mode(mode), _ptrBlock(ptrBlock), _saveVer(saveVer) {
}

void Serializer::saveOrLoadEntries(Entry *entry) {
	debug(DBG_SER, "Serializer::saveOrLoadEntries() _mode=%d", _mode);
	_bytesCount = 0;
	switch (_mode) {
	case SM_SAVE:
		saveEntries(entry);
		break;
	case SM_LOAD:
		loadEntries(entry);
		break;	
	}
	debug(DBG_SER, "Serializer::saveOrLoadEntries() _bytesCount=%d", _bytesCount);
}

void Serializer::saveEntries(Entry *entry) {
	debug(DBG_SER, "Serializer::saveEntries()");
	for (; entry->type != SET_END; ++entry) {
		if (entry->maxVer == CUR_VER) {
			switch (entry->type) {
			case SET_INT:
				saveInt(entry->size, entry->data);
				_bytesCount += entry->size;
				break;
			case SET_ARRAY:
				if (entry->size == Serializer::SES_INT8) {
					_stream->write(entry->data, entry->n);
					_bytesCount += entry->n;
				} else {
					uint8 *p = (uint8 *)entry->data;
					for (int i = 0; i < entry->n; ++i) {
						saveInt(entry->size, p);
						p += entry->size;
						_bytesCount += entry->size;
					}
				}
				break;
			case SET_PTR:
				_stream->writeUint32BE(*(uint8 **)(entry->data) - _ptrBlock);
				_bytesCount += 4;
				break;
			case SET_END:
				break;
			}
		}
	}
}

void Serializer::loadEntries(Entry *entry) {
	debug(DBG_SER, "Serializer::loadEntries()");
	for (; entry->type != SET_END; ++entry) {
		if (_saveVer >= entry->minVer && _saveVer <= entry->maxVer) {
			switch (entry->type) {
			case SET_INT:
				loadInt(entry->size, entry->data);
				_bytesCount += entry->size;
				break;
			case SET_ARRAY:
				if (entry->size == Serializer::SES_INT8) {
					_stream->read(entry->data, entry->n);
					_bytesCount += entry->n;
				} else {
					uint8 *p = (uint8 *)entry->data;
					for (int i = 0; i < entry->n; ++i) {
						loadInt(entry->size, p);
						p += entry->size;
						_bytesCount += entry->size;
					}
				}
				break;
			case SET_PTR:
				*(uint8 **)(entry->data) = _ptrBlock + _stream->readUint32BE();
				_bytesCount += 4;
				break;
			case SET_END:
				break;				
			}
		}
	}
}

void Serializer::saveInt(uint8 es, void *p) {
	switch (es) {
	case 1:
		_stream->writeByte(*(uint8 *)p);
		break;
	case 2:
		_stream->writeUint16BE(*(uint16 *)p);
		break;
	case 4:
		_stream->writeUint32BE(*(uint32 *)p);
		break;
	}
}

void Serializer::loadInt(uint8 es, void *p) {
	switch (es) {
	case 1:
		*(uint8 *)p = _stream->readByte();
		break;
	case 2:
		*(uint16 *)p = _stream->readUint16BE();
		break;
	case 4:
		*(uint32 *)p = _stream->readUint32BE();
		break;
	}
}
