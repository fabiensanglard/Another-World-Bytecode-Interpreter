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

#include "resource.h"
#include "bank.h"
#include "file.h"
#include "serializer.h"
#include "video.h"
#include "util.h"
#include "parts.h"

Resource::Resource(Video *vid, const char *dataDir) 
	: video(vid), _dataDir(dataDir), currentPartId(0) {
}

void Resource::readBank(const MemEntry *me, uint8_t *dstBuf) {
	uint16_t n = me - _memList;
	debug(DBG_BANK, "Resource::readBank(%d)", n);

	Bank bk(_dataDir);
	if (!bk.read(me, dstBuf)) {
		error("Resource::readBank() unable to unpack entry %d\n", n);
	}

}

static const char *resTypeToString(unsigned int type)
{
	static const char* resTypes[]=
	{
		"RT_SOUND",
		"RT_MUSIC",
		"RT_POLY_ANIM",
		"RT_PALETTE",
		"RT_BYTECODE",
		"RT_POLY_CINEMATIC"
	};
	if (type >= (sizeof(resTypes) / sizeof(const char *)))
		return "RT_UNKNOWN";
	return resTypes[type];
}

#define RES_SIZE 0
#define RES_COMPRESSED 1
int resourceSizeStats[7][2];
#define STATS_TOTAL_SIZE 6
int resourceUnitStats[7][2];

/*
	Read all entries from memlist.bin. Do not load anything in memory,
	this is just a fast way to access the data later based on their id.
*/
void Resource::readEntries() {	
	File f;
	int resourceCounter = 0;
	

	if (!f.open("memlist.bin", _dataDir)) {
		error("Resource::readEntries() unable to open 'memlist.bin' file\n");
		//Error will exit() no need to return or do anything else.
	}

	//Prepare stats array
	memset(resourceSizeStats,0,sizeof(resourceSizeStats));
	memset(resourceUnitStats,0,sizeof(resourceUnitStats));

	_numMemList = 0;
	MemEntry *memEntry = _memList;
	while (1) {
		assert(_numMemList < ARRAYSIZE(_memList));
		memEntry->state = f.readByte();
		memEntry->type = f.readByte();
		memEntry->bufPtr = 0; f.readUint16_tBE();
		memEntry->unk4 = f.readUint16_tBE();
		memEntry->rankNum = f.readByte();
		memEntry->bankId = f.readByte();
		memEntry->bankOffset = f.readUint32_tBE();
		memEntry->unkC = f.readUint16_tBE();
		memEntry->packedSize = f.readUint16_tBE();
		memEntry->unk10 = f.readUint16_tBE();
		memEntry->size = f.readUint16_tBE();

		//Memory tracking
		if (memEntry->packedSize==memEntry->size)
		{
			resourceUnitStats[memEntry->type][RES_SIZE] ++;
			resourceUnitStats[STATS_TOTAL_SIZE][RES_SIZE] ++;
		}
		else
		{
			resourceUnitStats[memEntry->type][RES_COMPRESSED] ++;
			resourceUnitStats[STATS_TOTAL_SIZE][RES_COMPRESSED] ++;
		}

		resourceSizeStats[memEntry->type][RES_SIZE] += memEntry->size;
		resourceSizeStats[STATS_TOTAL_SIZE][RES_SIZE] += memEntry->size;
		resourceSizeStats[memEntry->type][RES_COMPRESSED] += memEntry->packedSize;
		resourceSizeStats[STATS_TOTAL_SIZE][RES_COMPRESSED] += memEntry->packedSize;


		if (memEntry->state == MEMENTRY_STATE_END_OF_MEMLIST) {
			break;
		}

		debug(DBG_RES, "R:0x%02X, %-17s size=%5d (compacted gain=%2.0f%%)",
				resourceCounter,
				resTypeToString(memEntry->type),
				memEntry->size,
				memEntry->size ? (memEntry->size-memEntry->packedSize) / (float)memEntry->size * 100.0f : 0.0f);

		resourceCounter++;

		_numMemList++;
		memEntry++;
	}

	debug(DBG_RES,"\n");
	debug(DBG_RES,"Total # resources: %d",resourceCounter);
	debug(DBG_RES,"Compressed       : %d",resourceUnitStats[STATS_TOTAL_SIZE][RES_COMPRESSED]);
	debug(DBG_RES,"Uncompressed     : %d",resourceUnitStats[STATS_TOTAL_SIZE][RES_SIZE]);
	debug(DBG_RES,"Note: %2.0f%% of resources are compressed.",100*resourceUnitStats[STATS_TOTAL_SIZE][RES_COMPRESSED]/(float)resourceCounter);

	debug(DBG_RES,"\n");
	debug(DBG_RES,"Total size (uncompressed) : %7d bytes.",resourceSizeStats[STATS_TOTAL_SIZE][RES_SIZE]);
	debug(DBG_RES,"Total size (compressed)   : %7d bytes.",resourceSizeStats[STATS_TOTAL_SIZE][RES_COMPRESSED]);
	debug(DBG_RES,"Note: Overall compression gain is : %2.0f%%.",
		(resourceSizeStats[STATS_TOTAL_SIZE][RES_SIZE] - resourceSizeStats[STATS_TOTAL_SIZE][RES_COMPRESSED])/(float)resourceSizeStats[STATS_TOTAL_SIZE][RES_SIZE]*100);

	debug(DBG_RES,"\n");
	for(int i=0 ; i < 6 ; i++)
		debug(DBG_RES,"Total %-17s unpacked size: %7d (%2.0f%% of total unpacked size) packedSize %7d (%2.0f%% of floppy space) gain:(%2.0f%%)",
			resTypeToString(i),
			resourceSizeStats[i][RES_SIZE],
			resourceSizeStats[i][RES_SIZE] / (float)resourceSizeStats[STATS_TOTAL_SIZE][RES_SIZE] * 100.0f,
			resourceSizeStats[i][RES_COMPRESSED],
			resourceSizeStats[i][RES_COMPRESSED] / (float)resourceSizeStats[STATS_TOTAL_SIZE][RES_COMPRESSED] * 100.0f,
			(resourceSizeStats[i][RES_SIZE] - resourceSizeStats[i][RES_COMPRESSED]) / (float)resourceSizeStats[i][RES_SIZE] * 100.0f);

	debug(DBG_RES,"Note: Damn you sound compression rate!");

	debug(DBG_RES,"\nTotal bank files:              %d",resourceUnitStats[STATS_TOTAL_SIZE][RES_SIZE]+resourceUnitStats[STATS_TOTAL_SIZE][RES_COMPRESSED]);
	for(int i=0 ; i < 6 ; i++)
		debug(DBG_RES,"Total %-17s files: %3d",resTypeToString(i),resourceUnitStats[i][RES_SIZE]+resourceUnitStats[i][RES_COMPRESSED]);

}

/*
	Go over every resource and check if they are marked at "MEMENTRY_STATE_LOAD_ME".
	Load them in memory and mark them are MEMENTRY_STATE_LOADED
*/
void Resource::loadMarkedAsNeeded() {

	while (1) {
		
		MemEntry *me = NULL;

		// get resource with max rankNum
		uint8_t maxNum = 0;
		uint16_t i = _numMemList;
		MemEntry *it = _memList;
		while (i--) {
			if (it->state == MEMENTRY_STATE_LOAD_ME && maxNum <= it->rankNum) {
				maxNum = it->rankNum;
				me = it;
			}
			it++;
		}
		
		if (me == NULL) {
			break; // no entry found
		}

		
		// At this point the resource descriptor should be pointed to "me"
		// "That's what she said"

		uint8_t *loadDestination = NULL;
		if (me->type == RT_POLY_ANIM) {
			loadDestination = _vidCurPtr;
		} else {
			loadDestination = _scriptCurPtr;
			if (me->size > _vidBakPtr - _scriptCurPtr) {
				warning("Resource::load() not enough memory");
				me->state = 0;
				continue;
			}
		}


		if (me->bankId == 0) {
			warning("Resource::load() ec=0x%X (me->bankId == 0)", 0xF00);
			me->state = MEMENTRY_STATE_NOT_NEEDED;
		} else {
			debug(DBG_BANK, "Resource::load() bufPos=%X size=%X type=%X pos=%X bankId=%X", loadDestination - _memPtrStart, me->packedSize, me->type, me->bankOffset, me->bankId);
			readBank(me, loadDestination);
			if(me->type == RT_POLY_ANIM) {
				video->copyPagePtr(_vidCurPtr);
				me->state = 0;
			} else {
				me->bufPtr = loadDestination;
				me->state = MEMENTRY_STATE_LOADED;
				_scriptCurPtr += me->size;
			}
		}

	}


}

void Resource::invalidateRes() {
	MemEntry *me = _memList;
	uint16_t i = _numMemList;
	while (i--) {
		if (me->type <= RT_POLY_ANIM || me->type > 6) {  // 6 WTF ?!?! ResType goes up to 5 !!
			me->state = MEMENTRY_STATE_NOT_NEEDED;
		}
		++me;
	}
	_scriptCurPtr = _scriptBakPtr;
}

void Resource::invalidateAll() {
	MemEntry *me = _memList;
	uint16_t i = _numMemList;
	while (i--) {
		me->state = 0;
		++me;
	}
	_scriptCurPtr = _memPtrStart;
}

/* This method serves two purpose: 
    - Load parts in memory segments (palette,code,video1,video2)
	           or
    - Load a resource in memory

	This is decided based on the resourceId. If it does not match a mementry id it is supposed to 
	be a part id. */
void Resource::loadPartsOrMemoryEntry(uint16_t resourceId) {

	if (resourceId > _numMemList) {

		requestedNextPart = resourceId;

	} else {

		MemEntry *me = &_memList[resourceId];

		if (me->state == MEMENTRY_STATE_NOT_NEEDED) {
			me->state = MEMENTRY_STATE_LOAD_ME;
			loadMarkedAsNeeded();
		}
	}

}

/* Protection screen and cinematic don't need the player and enemies polygon data
   so _memList[video2Index] is never loaded for those parts of the game. When 
   needed (for action phrases) _memList[video2Index] is always loaded with 0x11 
   (as seen in memListParts). */
void Resource::setupPart(uint16_t partId) {

	

	if (partId == currentPartId)
		return;

	if (partId < GAME_PART_FIRST || partId > GAME_PART_LAST)
		error("Resource::setupPart() ec=0x%X invalid partId", partId);

	uint16_t memListPartIndex = partId - GAME_PART_FIRST;

	uint8_t paletteIndex = memListParts[memListPartIndex][MEMLIST_PART_PALETTE];
	uint8_t codeIndex    = memListParts[memListPartIndex][MEMLIST_PART_CODE];
	uint8_t videoCinematicIndex  = memListParts[memListPartIndex][MEMLIST_PART_POLY_CINEMATIC];
	uint8_t video2Index  = memListParts[memListPartIndex][MEMLIST_PART_VIDEO2];

	// Mark all resources as located on harddrive.
	invalidateAll();

	_memList[paletteIndex].state = 2;
	_memList[codeIndex].state = 2;
	_memList[videoCinematicIndex].state = 2;

	// This is probably a cinematic or a non interactive part of the game.
	// Player and enemy polygons are not needed.
	if (video2Index != MEMLIST_PART_NONE) 
		_memList[video2Index].state = 2;
	

	loadMarkedAsNeeded();

	segPalettes = _memList[paletteIndex].bufPtr;
	segBytecode     = _memList[codeIndex].bufPtr;
	segCinematic   = _memList[videoCinematicIndex].bufPtr;



	// This is probably a cinematic or a non interactive part of the game.
	// Player and enemy polygons are not needed.
	if (video2Index != MEMLIST_PART_NONE) 
		_segVideo2 = _memList[video2Index].bufPtr;
	
	debug(DBG_RES,"");
	debug(DBG_RES,"setupPart(%d)",partId-GAME_PART_FIRST);
	debug(DBG_RES,"Loaded resource %d (%s) in segPalettes.",paletteIndex,resTypeToString(_memList[paletteIndex].type));
	debug(DBG_RES,"Loaded resource %d (%s) in segBytecode.",codeIndex,resTypeToString(_memList[codeIndex].type));
	debug(DBG_RES,"Loaded resource %d (%s) in segCinematic.",videoCinematicIndex,resTypeToString(_memList[videoCinematicIndex].type));

	if (video2Index != MEMLIST_PART_NONE) 
		debug(DBG_RES,"Loaded resource %d (%s) in _segVideo2.",video2Index,resTypeToString(_memList[video2Index].type));



	currentPartId = partId;
	

	// _scriptCurPtr is changed in this->load();
	_scriptBakPtr = _scriptCurPtr;	
}

void Resource::allocMemBlock() {
	_memPtrStart = (uint8_t *)malloc(MEM_BLOCK_SIZE);
	_scriptBakPtr = _scriptCurPtr = _memPtrStart;
	_vidBakPtr = _vidCurPtr = _memPtrStart + MEM_BLOCK_SIZE - 0x800 * 16; //0x800 = 2048, so we have 32KB free for vidBack and vidCur
	_useSegVideo2 = false;
}

void Resource::freeMemBlock() {
	free(_memPtrStart);
}

void Resource::saveOrLoad(Serializer &ser) {
	uint8_t loadedList[64];
	if (ser._mode == Serializer::SM_SAVE) {
		memset(loadedList, 0, sizeof(loadedList));
		uint8_t *p = loadedList;
		uint8_t *q = _memPtrStart;
		while (1) {
			MemEntry *it = _memList;
			MemEntry *me = 0;
			uint16_t num = _numMemList;
			while (num--) {
				if (it->state == 1 && it->bufPtr == q) {
					me = it;
				}
				++it;
			}
			if (me == 0) {
				break;
			} else {
				assert(p < loadedList + 64);
				*p++ = me - _memList;
				q += me->size;
			}
		}
	}

	Serializer::Entry entries[] = {
		SE_ARRAY(loadedList, 64, Serializer::SES_INT8, VER(1)),
		SE_INT(&currentPartId, Serializer::SES_INT16, VER(1)),
		SE_PTR(&_scriptBakPtr, VER(1)),
		SE_PTR(&_scriptCurPtr, VER(1)),
		SE_PTR(&_vidBakPtr, VER(1)),
		SE_PTR(&_vidCurPtr, VER(1)),
		SE_INT(&_useSegVideo2, Serializer::SES_BOOL, VER(1)),
		SE_PTR(&segPalettes, VER(1)),
		SE_PTR(&segBytecode, VER(1)),
		SE_PTR(&segCinematic, VER(1)),
		SE_PTR(&_segVideo2, VER(1)),
		SE_END()
	};

	ser.saveOrLoadEntries(entries);
	if (ser._mode == Serializer::SM_LOAD) {
		uint8_t *p = loadedList;
		uint8_t *q = _memPtrStart;
		while (*p) {
			MemEntry *me = &_memList[*p++];
			readBank(me, q);
			me->bufPtr = q;
			me->state = 1;
			q += me->size;
		}
	}	
}

const char* Resource::getDataDir()
{
	return this->_dataDir;
}
