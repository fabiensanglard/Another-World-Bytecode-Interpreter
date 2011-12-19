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

#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "intern.h"


#define MEMENTRY_STATE_END_OF_MEMLIST 0xFF
#define MEMENTRY_STATE_NOT_NEEDED 0 
#define MEMENTRY_STATE_LOADED 1
#define MEMENTRY_STATE_LOAD_ME 2

/*
    This is a directory entry. When the game starts, it loads memlist.bin and 
	populate and array of MemEntry
*/
struct MemEntry {
	uint8 state;         // 0x0
	uint8 type;          // 0x1, Resource::ResType
	uint8 *bufPtr;       // 0x2
	uint16 unk4;         // 0x4, unused
	uint8 rankNum;       // 0x6
	uint8 bankId;       // 0x7
	uint32 bankOffset;      // 0x8 0xA
	uint16 unkC;         // 0xC, unused
	uint16 packedSize;   // 0xE
	                     // All ressources are packed (for a gain of 28% according to Chahi)

	uint16 unk10;        // 0x10, unused
	uint16 size; // 0x12
};
/*
     Note: state is not a boolean, it can have value 0, 1 or 2. WTF ?!

*/

struct Serializer;
struct Video;

struct Resource {

	enum ResType {
		RT_SOUND  = 0,
		RT_MUSIC  = 1,
		RT_VIDBUF = 2, // full screen video buffer, size=0x7D00 

		               // FCS: 0x7D00=32000...but 320x200 = 64000 ??
					   // Since the game is 16 colors, two pixels palette indices can be stored in one byte
					   // that's why we can store two pixels palette indice in one byte and we only need 320*200/2 bytes for 
					   // an entire screen.

		RT_PAL    = 3, // palette (1024=vga + 1024=ega), size=2048
		RT_SCRIPT = 4,
		RT_VBMP   = 5
	};
	
	enum {
		MEM_BLOCK_SIZE = 600 * 1024   //600kb total memory consumed (not taking into account stack and static heap)
	};
	
	
	Video *video;
	const char *_dataDir;
	MemEntry _memList[150];
	uint16 _numMemList;
	uint16 currentPartId, requestedNextPart;
	uint8 *_memPtrStart, *_scriptBakPtr, *_scriptCurPtr, *_vidBakPtr, *_vidCurPtr;
	bool _useSegVideo2;

	uint8 *segPalettes;
	uint8 *segBytecode;
	uint8 *segCinematic;
	uint8 *_segVideo2;

	Resource(Video *vid, const char *dataDir);
	
	void readBank(const MemEntry *me, uint8 *dstBuf);
	void readEntries();
	void loadMarkedAsNeeded();
	void invalidateAll();
	void invalidateRes();	
	void loadPartsOrMemoryEntry(uint16 num);
	void setupPart(uint16 ptrId);
	void allocMemBlock();
	void freeMemBlock();
	
	void saveOrLoad(Serializer &ser);

	const char* getDataDir();
};

#endif
