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
#include "vm.h"

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
		memEntry->bufPtr = 0; f.readUint16BE();
		memEntry->unk4 = f.readUint16BE();
		memEntry->rankNum = f.readByte();
		memEntry->bankId = f.readByte();
		memEntry->bankOffset = f.readUint32BE();
		memEntry->unkC = f.readUint16BE();
		memEntry->packedSize = f.readUint16BE();
		memEntry->unk10 = f.readUint16BE();
		memEntry->size = f.readUint16BE();

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

void Resource::dumpBinary(MemEntry* me, uint8_t* buffer, char* filename) {
  FILE* f = fopen(filename, "wb");

  //dump it!
  for (int p=0; p < me->size; p++){
    fprintf(f, "%c", buffer[p]);
  }
  fclose(f);  
}

static uint8_t fetchByte(uint8_t* buffer, int& pc){
  uint8_t ret = buffer[pc];
  pc++;
  return ret;
}

static uint16_t fetchWord(uint8_t* buffer, int& pc){
  uint16_t ret = 256*buffer[pc] + buffer[pc+1];//TODO: check endianess!
  pc+=2;
  return ret;
}

static void printVariableName(FILE* f, uint8_t id){
  switch(id){
    case VM_VARIABLE_RANDOM_SEED:
      fprintf(f, "RANDOM_SEED"); break;
    case VM_VARIABLE_LAST_KEYCHAR:
      fprintf(f, "LAST_KEYCHAR"); break;
    case VM_VARIABLE_HERO_POS_UP_DOWN:
      fprintf(f, "HERO_POS_UP_DOWN"); break;
    case VM_VARIABLE_MUS_MARK:
      fprintf(f, "MUS_MARK"); break;
    case VM_VARIABLE_SCROLL_Y:
      fprintf(f, "SCROLL_Y"); break;
    case VM_VARIABLE_HERO_ACTION:
      fprintf(f, "HERO_ACTION"); break;
    case VM_VARIABLE_HERO_POS_JUMP_DOWN:
      fprintf(f, "HERO_POS_JUMP_DOWN"); break;
    case VM_VARIABLE_HERO_POS_LEFT_RIGHT:
      fprintf(f, "HERO_POS_LEFT_RIGHT"); break;
    case VM_VARIABLE_HERO_POS_MASK:
      fprintf(f, "HERO_POS_MASK"); break;
    case VM_VARIABLE_HERO_ACTION_POS_MASK:
      fprintf(f, "HERO_ACTION_POS_MASK"); break;
    case VM_VARIABLE_PAUSE_SLICES:
      fprintf(f, "PAUSE_SLICES"); break;
    default:
      fprintf(f, "0x%02X", id); break;
  }
}

static void dump_movConst(FILE* f, uint8_t* buffer, int& pc) {
	uint8_t variableId = fetchByte(buffer, pc);
	uint16_t value = fetchWord(buffer, pc);
	fprintf(f, "  mov [");
  printVariableName(f, variableId);
	fprintf(f, "], 0x%04X\n", value);
}

static void dump_mov(FILE* f, uint8_t* buffer, int& pc) {
	uint8_t dstVariableId = fetchByte(buffer, pc);
	uint8_t srcVariableId = fetchByte(buffer, pc);	
	fprintf(f, "  mov [");
  printVariableName(f, dstVariableId);
	fprintf(f, "], [");
  printVariableName(f, srcVariableId);
	fprintf(f, "]\n");
}

static void dump_add(FILE* f, uint8_t* buffer, int& pc) {
	uint8_t dstVariableId = fetchByte(buffer, pc);
	uint8_t srcVariableId = fetchByte(buffer, pc);
	fprintf(f, "  add [");
  printVariableName(f, dstVariableId);
	fprintf(f, "], [");
  printVariableName(f, srcVariableId);
	fprintf(f, "]\n");
}

static void dump_addConst(FILE* f, uint8_t* buffer, int& pc) {
	uint8_t variableId = fetchByte(buffer, pc);
	uint16_t value = fetchWord(buffer, pc);
	fprintf(f, "  add [");
  printVariableName(f, variableId);
	fprintf(f, "], 0x%04X\n", value);
}

static void dump_call(FILE* f, uint8_t* buffer, int& pc) {
	uint16_t offset = fetchWord(buffer, pc);
	fprintf(f, "  call 0x%04X\n", offset);
}

static void dump_ret(FILE* f, uint8_t* buffer, int& pc) {
	fprintf(f, "  ret\n");
}

static void dump_pauseThread(FILE* f, uint8_t* buffer, int& pc) {
  // From Eric Chahi notes found
  // at (at http://www.anotherworld.fr/anotherworld_uk/page_realisation.htm):
  //
  //  Break
  //  Temporarily stops the executing channel and goes to the next.
	fprintf(f, "  break\n");
}

static void dump_jmp (FILE* f, uint8_t* buffer, int& pc) {
	uint16_t pcOffset = fetchWord(buffer, pc);
	fprintf(f, "  jmp 0x%04X\n", pcOffset);
}

static void dump_setSetVect (FILE* f, uint8_t* buffer, int& pc) {
  // From Eric Chahi notes found
  // at (at http://www.anotherworld.fr/anotherworld_uk/page_realisation.htm):
  //
  //  Setvec "numéro de canal", adresse
  //  Initialises a channel with a code address to execute

	uint8_t threadId = fetchByte(buffer, pc);
	uint16_t pcOffsetRequested = fetchWord(buffer, pc);
	fprintf(f, "  setvec channel:0x%02X, address:0x%04X\n", threadId, pcOffsetRequested);
}

static void dump_djnz (FILE* f, uint8_t* buffer, int& pc) {
  // djnz: 'D'ecrement variable value and 'J'ump if 'N'ot 'Z'ero
	uint8_t i = fetchByte(buffer, pc);
	uint16_t pcOffset = fetchWord(buffer, pc);
	fprintf(f, "  djnz [");
  printVariableName(f, i);
	fprintf(f, "], 0x%04X\n", pcOffset);
}

static void dump_condJmp (FILE* f, uint8_t* buffer, int& pc) {
	uint8_t subopcode = fetchByte(buffer, pc);
	uint8_t b = fetchByte(buffer, pc); //[b]
	uint8_t c = fetchByte(buffer, pc);	

	switch (subopcode & 7) {
	case 0:	// jz
//		expr = (b == a);
    fprintf(f, "  je [");
		break;
	case 1: // jnz
//		expr = (b != a);
    fprintf(f, "  jne [");
		break;
	case 2: // jg
//		expr = (b > a);
    fprintf(f, "  jg [");
		break;
	case 3: // jge
//		expr = (b >= a);
    fprintf(f, "  jge [");
		break;
	case 4: // jl
//		expr = (b < a);
    fprintf(f, "  jl [");
		break;
	case 5: // jle
//		expr = (b <= a);
    fprintf(f, "  jle [");
		break;
	default:
    fprintf(f, "< conditional jmp: invalid condition %d >\n", (subopcode & 7));
		break;
    return;
	}

  printVariableName(f, b);
  fprintf(f, "], ");

	if (subopcode & 0x80) {
		fprintf(f, "[");
    printVariableName(f, c);
		fprintf(f, "]");
	} else if (subopcode & 0x40) {
    fprintf(f, "0x%04X", c * 256 + fetchByte(buffer, pc));
	} else {
		fprintf(f, "0x%02X", c);
	}

	uint16_t offset = fetchWord(buffer, pc);
  fprintf(f, ", 0x%04X\n", offset);
}

static void dump_setPalette (FILE* f, uint8_t* buffer, int& pc) {
	uint16_t paletteId = fetchWord(buffer, pc);
	fprintf(f, "  setPalette 0x%04X\n", paletteId);
}

static void dump_resetThread (FILE* f, uint8_t* buffer, int& pc) {
  // From Eric Chahi notes found
  // at (at http://www.anotherworld.fr/anotherworld_uk/page_realisation.htm):
  //
  //  Vec début, fin, type
  //  Deletes, freezes or unfreezes a series of channels.

	uint8_t first = fetchByte(buffer, pc);
	uint8_t last = fetchByte(buffer, pc);
	uint8_t type = fetchByte(buffer, pc);

  switch (type){
    case 0: fprintf (f, "  freezeChannels"); break;
    case 1: fprintf (f, "  unfreezeChannels"); break;
    case 2: fprintf (f, "  deleteChannels"); break;
    default:
      fprintf (f, " <error: invalid type for resetThread opcode>\n");
      return;
      break;
  }
	fprintf(f, " first:0x%02X, last:0x%02X\n", first, last);
}

static void dump_selectVideoPage (FILE* f, uint8_t* buffer, int& pc) {
	uint8_t frameBufferId = fetchByte(buffer, pc);
	fprintf(f, "  selectVideoPage 0x%02X\n", frameBufferId);
}

static void dump_fillVideoPage (FILE* f, uint8_t* buffer, int& pc) {
	uint8_t pageId = fetchByte(buffer, pc);
	uint8_t color = fetchByte(buffer, pc);
	fprintf(f, "  fillVideoPage 0x%02X, color:0x%02X\n", pageId, color);
}

static void dump_copyVideoPage (FILE* f, uint8_t* buffer, int& pc) {
	uint8_t srcPageId = fetchByte(buffer, pc);
	uint8_t dstPageId = fetchByte(buffer, pc);
	fprintf(f, "  copyVideoPage src:0x%02X, dst:0x%02X\n", srcPageId, dstPageId);
}

static void dump_blitFramebuffer (FILE* f, uint8_t* buffer, int& pc) {
	uint8_t pageId = fetchByte(buffer, pc);
	fprintf(f, "  blitFramebuffer 0x%02X\n", pageId);
}

static void dump_killThread (FILE* f, uint8_t* buffer, int& pc) {
	fprintf(f, "  killChannel\n");
}

static void dump_drawString (FILE* f, uint8_t* buffer, int& pc) {
  // From Eric Chahi notes found
  // at (at http://www.anotherworld.fr/anotherworld_uk/page_realisation.htm):
  //
  //  Text "text number", x, y, color
  //  Displays in the work screen the specified text for the coordinates x,y.

	uint16_t stringId = fetchWord(buffer, pc);
	uint16_t x = fetchByte(buffer, pc);
	uint16_t y = fetchByte(buffer, pc);
	uint16_t color = fetchByte(buffer, pc);

	const StrEntry *se = Video::_stringsTableEng;

	//Search for the location where the string is located.
	while (se->id != END_OF_STRING_DICTIONARY && se->id != stringId)
		++se;

	fprintf(f, "  text id:0x%04X, x:%d, y:%d, color:0x%02X\t;\"%s\"\n", stringId, x, y, color, se->str);
}

static void dump_sub (FILE* f, uint8_t* buffer, int& pc) {
	uint8_t i = fetchByte(buffer, pc);
	uint8_t j = fetchByte(buffer, pc);
	fprintf(f, "  sub [");
  printVariableName(f, i);
	fprintf(f, "], [");
  printVariableName(f, j);
	fprintf(f, "]\n");
}

static void dump_and (FILE* f, uint8_t* buffer, int& pc) {
	uint8_t variableId = fetchByte(buffer, pc);
	uint16_t n = fetchWord(buffer, pc);
	fprintf(f, "  and [");
  printVariableName(f, variableId);
	fprintf(f, "], 0x%04X\n", n);
}

static void dump_or (FILE* f, uint8_t* buffer, int& pc) {
	uint8_t variableId = fetchByte(buffer, pc);
	uint16_t value = fetchWord(buffer, pc);
	fprintf(f, "  or [");
  printVariableName(f, variableId);
	fprintf(f, "], 0x%04X\n", value);
}

static void dump_shl (FILE* f, uint8_t* buffer, int& pc) {
	uint8_t variableId = fetchByte(buffer, pc);
	uint16_t leftShiftValue = fetchWord(buffer, pc);
	fprintf(f, "  shl [");
  printVariableName(f, variableId);
	fprintf(f, "], 0x%04X\n", leftShiftValue);
}

static void dump_shr (FILE* f, uint8_t* buffer, int& pc) {
	uint8_t variableId = fetchByte(buffer, pc);
	uint16_t rightShiftValue = fetchWord(buffer, pc);
	fprintf(f, "  shr [");
  printVariableName(f, variableId);
	fprintf(f, "], 0x%04X\n", rightShiftValue);
}

static void dump_playSound (FILE* f, uint8_t* buffer, int& pc) {
  // From Eric Chahi notes found
  // at (at http://www.anotherworld.fr/anotherworld_uk/page_realisation.htm):
  //
  //  Play "file number" note, volume, channel
  //  Plays the sound file on one of the four game audio channels with
  //  specific height and volume.

	uint16_t resourceId = fetchWord(buffer, pc);
	uint8_t freq = fetchByte(buffer, pc);
	uint8_t vol = fetchByte(buffer, pc);
	uint8_t channel = fetchByte(buffer, pc);
	fprintf(f, "  play id:0x%04X, freq:0x%02X, vol:0x%02X, channel:0x%02X\n", resourceId, freq, vol, channel);
}

static void dump_updateMemList (FILE* f, uint8_t* buffer, int& pc) {
  // From Eric Chahi notes found
  // at (at http://www.anotherworld.fr/anotherworld_uk/page_realisation.htm):
  //
  //  Load "file number"
  //  Loads a file in memory, such as sound, level or image.

	uint16_t resourceId = fetchWord(buffer, pc);
	fprintf(f, "  load id:0x%04X\n", resourceId);
}

static void dump_playMusic (FILE* f, uint8_t* buffer, int& pc) {
  // From Eric Chahi notes found
  // at (at http://www.anotherworld.fr/anotherworld_uk/page_realisation.htm):
  //
  //  Song "file number" Tempo, Position
  //  Initialises a song.

	uint16_t resNum = fetchWord(buffer, pc);
	uint16_t delay = fetchWord(buffer, pc);
	uint8_t pos = fetchByte(buffer, pc);
	fprintf(f, "  song id:0x%04X, delay:0x%04X, pos:0x%02X\n", resNum, delay, pos);
}

static void dump_video_opcodes (FILE* f, uint8_t* buffer, int& pc) {
	uint8_t opcode = buffer[pc-1];

	fprintf(f, "DEBUG  video: [0x%X 0x%X 0x%X 0x%X 0x%X]\n", buffer[pc-1], buffer[pc], buffer[pc+1], buffer[pc+2], buffer[pc+3]);

	if (opcode & 0x80)
	{
		uint16_t off = ((opcode << 8) | fetchByte(buffer, pc)) * 2;
		int16_t x = fetchByte(buffer, pc);
		int16_t y = fetchByte(buffer, pc);
		int16_t h = y - 199;
		if (h > 0) {
			y = 199;
			x += h;
		}
		fprintf(f, "  video: off=0x%X x=%d y=%d\n", off, x, y);
		return;
	}

	if (opcode & 0x40)
	{
		uint16_t x, y;
		uint16_t off = fetchWord(buffer, pc) * 2;
		char x_str[20];
		char y_str[20];
		char zoom_str[20];

		x = fetchByte(buffer, pc);
		if (!(opcode & 0x20))
		{
			if (!(opcode & 0x10))
			{
				x = (x << 8) | fetchByte(buffer, pc);
				sprintf(x_str, "%d", x);
			} else {
				sprintf(x_str, "[0x%02x]", x);
			}
		}
		else
		{
			if (opcode & 0x10) {
				x += 0x100;
				sprintf(x_str, "%d", x);
			}
		}

		if (!(opcode & 8))
		{
			if (!(opcode & 4)) {
				y = fetchByte(buffer, pc);
				y = (y << 8) | fetchByte(buffer, pc);
				sprintf(y_str, "%d", (int) y);
			} else {
				y = fetchByte(buffer, pc);
				sprintf(y_str, "[0x%02x]", y);
			}
		}

		uint16_t zoom;
		if (!(opcode & 2))
		{
			if (!(opcode & 1))
			{
				sprintf(zoom_str, "0x40");
			}
			else
			{
				zoom = fetchByte(buffer, pc);
				sprintf(zoom_str, "[0x%02x]", zoom);
			}
		}
		else
		{
			if (opcode & 1) {
				zoom = 0x40;
				sprintf(zoom_str, "0x40");
			} else {
				zoom = fetchByte(buffer, pc);
				sprintf(zoom_str, "[0x%02x]", zoom);
			}
		}
		fprintf(f, "  video: off=0x%X x=%s y=%s zoom:%s\n", off, x_str, y_str, zoom_str);
	}
}

void Resource::dumpSource(MemEntry* me, uint8_t* buffer, char* filename) {
  FILE* f = fopen(filename, "wb");

  int pc=0;
  while (pc < me->size){
    fprintf(f, "%05X:\t", pc);
    uint8_t opcode = buffer[pc++];
    switch(opcode){
      case 0x00: dump_movConst(f, buffer, pc); break;
      case 0x01: dump_mov(f, buffer, pc); break;
      case 0x02: dump_add(f, buffer, pc); break;
      case 0x03: dump_addConst(f, buffer, pc); break;
      case 0x04: dump_call(f, buffer, pc); break;
      case 0x05: dump_ret(f, buffer, pc); break;
      case 0x06: dump_pauseThread(f, buffer, pc); break;
      case 0x07: dump_jmp(f, buffer, pc); break;
      case 0x08: dump_setSetVect(f, buffer, pc); break;
      case 0x09: dump_djnz(f, buffer, pc); break;
      case 0x0A: dump_condJmp(f, buffer, pc); break;
      case 0x0B: dump_setPalette(f, buffer, pc); break;
      case 0x0C: dump_resetThread(f, buffer, pc); break;
      case 0x0D: dump_selectVideoPage(f, buffer, pc); break;
      case 0x0E: dump_fillVideoPage(f, buffer, pc); break;
      case 0x0F: dump_copyVideoPage(f, buffer, pc); break;
      case 0x10: dump_blitFramebuffer(f, buffer, pc); break;
      case 0x11: dump_killThread(f, buffer, pc); break;
      case 0x12: dump_drawString(f, buffer, pc); break;
      case 0x13: dump_sub(f, buffer, pc); break;
      case 0x14: dump_and(f, buffer, pc); break;
      case 0x15: dump_or(f, buffer, pc); break;
      case 0x16: dump_shl(f, buffer, pc); break;
      case 0x17: dump_shr(f, buffer, pc); break;
      case 0x18: dump_playSound(f, buffer, pc); break;
      case 0x19: dump_updateMemList(f, buffer, pc); break;
      case 0x1A: dump_playMusic(f, buffer, pc); break;
      default:
		if (opcode & 0xC0){
			dump_video_opcodes(f, buffer, pc); break;
		} else {
	        fprintf(f, "<unknown opcode: %02X>\n", opcode); break;
		}
    }
  }
  fclose(f);
}

void Resource::dumpBytecode() {
  int i;
	for (i=0; i < _numMemList; i++) {
		MemEntry *me = &_memList[i];

    uint8_t* buffer = (uint8_t*) malloc(me->size * sizeof(uint8_t));
    char filename[18];
    sprintf(filename, "resource-0x%02x.txt", i);
	  FILE* f = fopen(filename, "wb");

    //dump it!
    readBank(me, buffer);
    fprintf(f, "bytecode chunk #%d:\n\n", i);
    for (int p=0; p < me->size; p++){
      fprintf(f, "%02x ", buffer[p]);
      if (p%8==3) fprintf(f, " ");
      if (p%8==7) fprintf(f, "\n");
    }
    fprintf(f, "\n\n");
    fclose(f);

    sprintf(filename, "resource-0x%02x.bin", i);
    dumpBinary(me, buffer, filename);

		if (me->type == RT_BYTECODE) {
      sprintf(filename, "resource-0x%02x.asm", i);
      dumpSource(me, buffer, filename);
    }

    free(buffer);
  }  	
}

void Resource::dumpUnknown() {
  int i;
	for (i=0; i < _numMemList; i++) {
		MemEntry *me = &_memList[i];
    uint8_t* buffer = (uint8_t*) malloc(me->size * sizeof(uint8_t));
		
		if (me->type > RT_POLY_CINEMATIC) {

      printf("unknown type: %d\n\n", me->type);

      char filename[14];
      FILE* f;
      sprintf(filename, "unknown%02X.dat", i);

      f = fopen(filename, "wb");

      //dump it!
      readBank(me, buffer);
      printf("unknown chunk #%d:\n", i);
      for (int p=0; p < me->size; p++){
        fprintf(f, "%c", buffer[p]);
      }
      fclose(f);
//      printf("\n\n");
		}

    free(buffer);
  }  	
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
				me->state = MEMENTRY_STATE_NOT_NEEDED;
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
				me->state = MEMENTRY_STATE_NOT_NEEDED;
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
		me->state = MEMENTRY_STATE_NOT_NEEDED;
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

	_memList[paletteIndex].state = MEMENTRY_STATE_LOAD_ME;
	_memList[codeIndex].state = MEMENTRY_STATE_LOAD_ME;
	_memList[videoCinematicIndex].state = MEMENTRY_STATE_LOAD_ME;

	// This is probably a cinematic or a non interactive part of the game.
	// Player and enemy polygons are not needed.
	if (video2Index != MEMLIST_PART_NONE) 
		_memList[video2Index].state = MEMENTRY_STATE_LOAD_ME;
	

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
				if (it->state == MEMENTRY_STATE_LOADED && it->bufPtr == q) {
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
			me->state = MEMENTRY_STATE_LOADED;
			q += me->size;
		}
	}	
}

const char* Resource::getDataDir()
{
	return this->_dataDir;
}
