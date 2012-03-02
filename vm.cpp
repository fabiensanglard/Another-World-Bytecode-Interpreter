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

#include <ctime>
#include "vm.h"
#include "mixer.h"
#include "resource.h"
#include "video.h"
#include "serializer.h"
#include "sfxplayer.h"
#include "sys.h"
#include "parts.h"
#include "file.h"

VirtualMachine::VirtualMachine(Mixer *mix, Resource *resParameter, SfxPlayer *ply, Video *vid, System *stub)
	: mixer(mix), res(resParameter), player(ply), video(vid), sys(stub) {
}

void VirtualMachine::init() {

	memset(vmVariables, 0, sizeof(vmVariables));
	vmVariables[0x54] = 0x81;
	vmVariables[VM_VARIABLE_RANDOM_SEED] = time(0);

	_fastMode = false;
	player->_markVar = &vmVariables[VM_VARIABLE_MUS_MARK];
}

void VirtualMachine::op_movConst() {
	uint8 variableId = _scriptPtr.fetchByte();
	int16 value = _scriptPtr.fetchWord();
	debug(DBG_VM, "VirtualMachine::op_movConst(0x%02X, %d)", variableId, value);
	vmVariables[variableId] = value;
}

void VirtualMachine::op_mov() {
	uint8 dstVariableId = _scriptPtr.fetchByte();
	uint8 srcVariableId = _scriptPtr.fetchByte();	
	debug(DBG_VM, "VirtualMachine::op_mov(0x%02X, 0x%02X)", dstVariableId, srcVariableId);
	vmVariables[dstVariableId] = vmVariables[srcVariableId];
}

void VirtualMachine::op_add() {
	uint8 dstVariableId = _scriptPtr.fetchByte();
	uint8 srcVariableId = _scriptPtr.fetchByte();
	debug(DBG_VM, "VirtualMachine::op_add(0x%02X, 0x%02X)", dstVariableId, srcVariableId);
	vmVariables[dstVariableId] += vmVariables[srcVariableId];
}

void VirtualMachine::op_addConst() {
	if (res->currentPartId == 0x3E86 && _scriptPtr.pc == res->segBytecode + 0x6D48) {
		warning("VirtualMachine::op_addConst() hack for non-stop looping gun sound bug");
		// the script 0x27 slot 0x17 doesn't stop the gun sound from looping, I 
		// don't really know why ; for now, let's play the 'stopping sound' like 
		// the other scripts do
		//  (0x6D43) jmp(0x6CE5)
		//  (0x6D46) break
		//  (0x6D47) VAR(6) += -50
		snd_playSound(0x5B, 1, 64, 1);
	}
	uint8 variableId = _scriptPtr.fetchByte();
	int16 value = _scriptPtr.fetchWord();
	debug(DBG_VM, "VirtualMachine::op_addConst(0x%02X, %d)", variableId, value);
	vmVariables[variableId] += value;
}

void VirtualMachine::op_call() {

	uint16 offset = _scriptPtr.fetchWord();
	uint8 sp = _stackPtr;

	debug(DBG_VM, "VirtualMachine::op_call(0x%X)", offset);
	_scriptStackCalls[sp] = _scriptPtr.pc - res->segBytecode;
	if (_stackPtr == 0xFF) {
		error("VirtualMachine::op_call() ec=0x%X stack overflow", 0x8F);
	}
	++_stackPtr;
	_scriptPtr.pc = res->segBytecode + offset ;
}

void VirtualMachine::op_ret() {
	debug(DBG_VM, "VirtualMachine::op_ret()");
	if (_stackPtr == 0) {
		error("VirtualMachine::op_ret() ec=0x%X stack underflow", 0x8F);
	}	
	--_stackPtr;
	uint8 sp = _stackPtr;
	_scriptPtr.pc = res->segBytecode + _scriptStackCalls[sp];
}

void VirtualMachine::op_pauseThread() {
	debug(DBG_VM, "VirtualMachine::op_pauseThread()");
	gotoNextThread = true;
}

void VirtualMachine::op_jmp() {
	uint16 pcOffset = _scriptPtr.fetchWord();
	debug(DBG_VM, "VirtualMachine::op_jmp(0x%02X)", pcOffset);
	_scriptPtr.pc = res->segBytecode + pcOffset;	
}

void VirtualMachine::op_setSetVect() {
	uint8 threadId = _scriptPtr.fetchByte();
	uint16 pcOffsetRequested = _scriptPtr.fetchWord();
	debug(DBG_VM, "VirtualMachine::op_setSetVect(0x%X, 0x%X)", threadId,pcOffsetRequested);
	threadsData[1][threadId] = pcOffsetRequested;
}

void VirtualMachine::op_jnz() {
	uint8 i = _scriptPtr.fetchByte();
	debug(DBG_VM, "VirtualMachine::op_jnz(0x%02X)", i);
	--vmVariables[i];
	if (vmVariables[i] != 0) {
		op_jmp();
	} else {
		_scriptPtr.fetchWord();
	}
}

#define BYPASS_PROTECTION
void VirtualMachine::op_condJmp() {
	
	//printf("Jump : %X \n",_scriptPtr.pc-res->segBytecode);
//FCS Whoever wrote this is patching the bytecode on the fly. This is ballzy !!
#ifdef BYPASS_PROTECTION
	
	if (res->currentPartId == GAME_PART_FIRST && _scriptPtr.pc == res->segBytecode + 0xCB9) {
		
		// (0x0CB8) condJmp(0x80, VAR(41), VAR(30), 0xCD3)
		*(_scriptPtr.pc + 0x00) = 0x81;
		*(_scriptPtr.pc + 0x03) = 0x0D;
		*(_scriptPtr.pc + 0x04) = 0x24;
		// (0x0D4E) condJmp(0x4, VAR(50), 6, 0xDBC)		
		*(_scriptPtr.pc + 0x99) = 0x0D;
		*(_scriptPtr.pc + 0x9A) = 0x5A;
		printf("VirtualMachine::op_condJmp() bypassing protection");
		printf("bytecode has been patched/n");
		
		//this->bypassProtection() ;
	}
	
	
#endif

	uint8 opcode = _scriptPtr.fetchByte();
	int16 b = vmVariables[_scriptPtr.fetchByte()];
	uint8 c = _scriptPtr.fetchByte();	
	int16 a;

	if (opcode & 0x80) {
		a = vmVariables[c];
	} else if (opcode & 0x40) {
		a = c * 256 + _scriptPtr.fetchByte();
	} else {
		a = c;
	}
	debug(DBG_VM, "VirtualMachine::op_condJmp(%d, 0x%02X, 0x%02X)", opcode, b, a);

	// Check if the conditional value is met.
	bool expr = false;
	switch (opcode & 7) {
	case 0:	// jz
		expr = (b == a);
		break;
	case 1: // jnz
		expr = (b != a);
		break;
	case 2: // jg
		expr = (b > a);
		break;
	case 3: // jge
		expr = (b >= a);
		break;
	case 4: // jl
		expr = (b < a);
		break;
	case 5: // jle
		expr = (b <= a);
		break;
	default:
		warning("VirtualMachine::op_condJmp() invalid condition %d", (opcode & 7));
		break;
	}

	if (expr) {
		op_jmp();
	} else {
		_scriptPtr.fetchWord();
	}

}

void VirtualMachine::op_setPalette() {
	uint16 paletteId = _scriptPtr.fetchWord();
	debug(DBG_VM, "VirtualMachine::op_changePalette(%d)", paletteId);
	video->paletteIdRequested = paletteId >> 8;
}

void VirtualMachine::op_resetThread() {

	uint8 threadId = _scriptPtr.fetchByte();
	uint8 i =        _scriptPtr.fetchByte();

	// FCS: WTF, this is cryptic as hell !!
	//int8 n = (i & 0x3F) - threadId;  //0x3F = 0011 1111
	// The following is so much clearer

	//Make sure i within [0-VM_NUM_THREADS-1]
	i = i & (VM_NUM_THREADS-1) ;
	int8 n = i - threadId;

	if (n < 0) {
		warning("VirtualMachine::op_resetThread() ec=0x%X (n < 0)", 0x880);
		return;
	}
	++n;
	uint8 a = _scriptPtr.fetchByte();

	debug(DBG_VM, "VirtualMachine::op_resetThread(%d, %d, %d)", threadId, i, a);

	if (a == 2) {
		uint16 *p = &threadsData[1][threadId];
		while (n--) {
			*p++ = 0xFFFE;
		}
	} else if (a < 2) {
		uint8 *p = &vmIsChannelActive[1][threadId];
		while (n--) {
			*p++ = a;
		}
	}
}

void VirtualMachine::op_selectVideoPage() {
	uint8 frameBufferId = _scriptPtr.fetchByte();
	debug(DBG_VM, "VirtualMachine::op_selectVideoPage(%d)", frameBufferId);
	video->changePagePtr1(frameBufferId);
}

void VirtualMachine::op_fillVideoPage() {
	uint8 pageId = _scriptPtr.fetchByte();
	uint8 color = _scriptPtr.fetchByte();
	debug(DBG_VM, "VirtualMachine::op_fillVideoPage(%d, %d)", pageId, color);
	video->fillPage(pageId, color);
}

void VirtualMachine::op_copyVideoPage() {
	uint8 srcPageId = _scriptPtr.fetchByte();
	uint8 dstPageId = _scriptPtr.fetchByte();
	debug(DBG_VM, "VirtualMachine::op_copyVideoPage(%d, %d)", srcPageId, dstPageId);
	video->copyPage(srcPageId, dstPageId, vmVariables[VM_VARIABLE_SCROLL_Y]);
}


uint32 lastTimeStamp = 0;
void VirtualMachine::op_blitFramebuffer() {

	uint8 pageId = _scriptPtr.fetchByte();
	debug(DBG_VM, "VirtualMachine::op_blitFramebuffer(%d)", pageId);
	inp_handleSpecialKeys();

	//Nasty hack....was this present in the original assembly  ??!!
	if (res->currentPartId == GAME_PART_FIRST && vmVariables[0x67] == 1) 
		vmVariables[0xDC] = 0x21;
	
	if (!_fastMode) {

		int32 delay = sys->getTimeStamp() - lastTimeStamp;
		int32 timeToSleep = vmVariables[VM_VARIABLE_PAUSE_SLICES] * 20 - delay;

		// The bytecode will set vmVariables[VM_VARIABLE_PAUSE_SLICES] from 1 to 5
		// The virtual machine hence indicate how long the image should be displayed.

		//printf("vmVariables[VM_VARIABLE_PAUSE_SLICES]=%d\n",vmVariables[VM_VARIABLE_PAUSE_SLICES]);


		if (timeToSleep > 0)
		{
		//	printf("Sleeping for=%d\n",timeToSleep);
			sys->sleep(timeToSleep);
		}

		lastTimeStamp = sys->getTimeStamp();
	}

	//WTF ?
	vmVariables[0xF7] = 0;

	video->updateDisplay(pageId);
}

void VirtualMachine::op_killThread() {
	debug(DBG_VM, "VirtualMachine::op_killThread()");
	_scriptPtr.pc = res->segBytecode + 0xFFFF;
	gotoNextThread = true;
}

void VirtualMachine::op_drawString() {
	uint16 stringId = _scriptPtr.fetchWord();
	uint16 x = _scriptPtr.fetchByte();
	uint16 y = _scriptPtr.fetchByte();
	uint16 color = _scriptPtr.fetchByte();

	debug(DBG_VM, "VirtualMachine::op_drawString(0x%03X, %d, %d, %d)", stringId, x, y, color);

	video->drawString(color, x, y, stringId);
}

void VirtualMachine::op_sub() {
	uint8 i = _scriptPtr.fetchByte();
	uint8 j = _scriptPtr.fetchByte();
	debug(DBG_VM, "VirtualMachine::op_sub(0x%02X, 0x%02X)", i, j);
	vmVariables[i] -= vmVariables[j];
}

void VirtualMachine::op_and() {
	uint8 variableId = _scriptPtr.fetchByte();
	uint16 n = _scriptPtr.fetchWord();
	debug(DBG_VM, "VirtualMachine::op_and(0x%02X, %d)", variableId, n);
	vmVariables[variableId] = (uint16)vmVariables[variableId] & n;
}

void VirtualMachine::op_or() {
	uint8 variableId = _scriptPtr.fetchByte();
	uint16 value = _scriptPtr.fetchWord();
	debug(DBG_VM, "VirtualMachine::op_or(0x%02X, %d)", variableId, value);
	vmVariables[variableId] = (uint16)vmVariables[variableId] | value;
}

void VirtualMachine::op_shl() {
	uint8 variableId = _scriptPtr.fetchByte();
	uint16 leftShiftValue = _scriptPtr.fetchWord();
	debug(DBG_VM, "VirtualMachine::op_shl(0x%02X, %d)", variableId, leftShiftValue);
	vmVariables[variableId] = (uint16)vmVariables[variableId] << leftShiftValue;
}

void VirtualMachine::op_shr() {
	uint8 variableId = _scriptPtr.fetchByte();
	uint16 rightShiftValue = _scriptPtr.fetchWord();
	debug(DBG_VM, "VirtualMachine::op_shr(0x%02X, %d)", variableId, rightShiftValue);
	vmVariables[variableId] = (uint16)vmVariables[variableId] >> rightShiftValue;
}

void VirtualMachine::op_playSound() {
	uint16 resourceId = _scriptPtr.fetchWord();
	uint8 freq = _scriptPtr.fetchByte();
	uint8 vol = _scriptPtr.fetchByte();
	uint8 channel = _scriptPtr.fetchByte();
	debug(DBG_VM, "VirtualMachine::op_playSound(0x%X, %d, %d, %d)", resourceId, freq, vol, channel);
	snd_playSound(resourceId, freq, vol, channel);
}

void VirtualMachine::op_updateMemList() {

	uint16 resourceId = _scriptPtr.fetchWord();
	debug(DBG_VM, "VirtualMachine::op_updateMemList(%d)", resourceId);

	if (resourceId == 0) {
		player->stop();
		mixer->stopAll();
		res->invalidateRes();
	} else {
		res->loadPartsOrMemoryEntry(resourceId);
	}
}

void VirtualMachine::op_playMusic() {
	uint16 resNum = _scriptPtr.fetchWord();
	uint16 delay = _scriptPtr.fetchWord();
	uint8 pos = _scriptPtr.fetchByte();
	debug(DBG_VM, "VirtualMachine::op_playMusic(0x%X, %d, %d)", resNum, delay, pos);
	snd_playMusic(resNum, delay, pos);
}

void VirtualMachine::initForPart(uint16 partId) {

	player->stop();
	mixer->stopAll();

	//WTF is that ?
	vmVariables[0xE4] = 0x14;

	res->setupPart(partId);

	//Set all thread to inactive (pc at 0xFFFF or 0xFFFE )
	memset((uint8 *)threadsData, 0xFF, sizeof(threadsData));


	memset((uint8 *)vmIsChannelActive, 0, sizeof(vmIsChannelActive));
	
	int firstThreadId = 0;
	threadsData[PC_OFFSET][firstThreadId] = 0;	
}

/* 
     This is called every frames in the infinite loop.
*/
void VirtualMachine::checkThreadRequests() {

	//Check if a part switch has been requested.
	if (res->requestedNextPart != 0) {
		initForPart(res->requestedNextPart);
		res->requestedNextPart = 0;
	}

	
	// Check if a state update has been requested for any thread during the previous VM execution:
	//      - Pause
	//      - Jump

	// JUMP:
	// Note: If a jump has been requested, the jump destination is stored
	// in threadsData[REQUESTED_PC_OFFSET]. Otherwise threadsData[REQUESTED_PC_OFFSET] == 0xFFFF

	// PAUSE:
	// Note: If a pause has been requested it is stored in  vmIsChannelActive[REQUESTED_STATE][i]

	for (int threadId = 0; threadId < VM_NUM_THREADS; threadId++) {

		vmIsChannelActive[CURR_STATE][threadId] = vmIsChannelActive[REQUESTED_STATE][threadId];

		uint16 n = threadsData[REQUESTED_PC_OFFSET][threadId];

		if (n != VM_NO_SETVEC_REQUESTED) {

			threadsData[PC_OFFSET][threadId] = (n == 0xFFFE) ? VM_INACTIVE_THREAD : n;
			threadsData[REQUESTED_PC_OFFSET][threadId] = VM_NO_SETVEC_REQUESTED;
		}
	}
}

void VirtualMachine::hostFrame() {

	// Run the Virtual Machine for every active threads (one vm frame).
	// Inactive threads are marked with a thread instruction pointer set to 0xFFFF (VM_INACTIVE_THREAD).
	// A thread must feature a break opcode so the interpreter can move to the next thread.

	for (int threadId = 0; threadId < VM_NUM_THREADS; threadId++) {

		if (vmIsChannelActive[CURR_STATE][threadId])
			continue;
		
		uint16 n = threadsData[0][threadId];

		if (n != VM_INACTIVE_THREAD) {

			// Set the script pointer to the right location.
			// script pc is used in executeThread in order
			// to get the next opcode.
			_scriptPtr.pc = res->segBytecode + n;
			_stackPtr = 0;

			gotoNextThread = false;
			debug(DBG_VM, "VirtualMachine::hostFrame() i=0x%02X n=0x%02X *p=0x%02X", threadId, n, *_scriptPtr.pc);
			executeThread();

			//Since .pc is going to be modified by this next loop iteration, we need to save it.
			threadsData[PC_OFFSET][threadId] = _scriptPtr.pc - res->segBytecode;


			debug(DBG_VM, "VirtualMachine::hostFrame() i=0x%02X pos=0x%X", threadId, threadsData[0][threadId]);
			if (sys->input.quit) {
				break;
			}
		}
		
	}
}

#define COLOR_BLACK 0xFF
#define DEFAULT_ZOOM 0x40


void VirtualMachine::executeThread() {

	while (!gotoNextThread) {
		uint8 opcode = _scriptPtr.fetchByte();

		// 1000 0000 is set
		if (opcode & 0x80) 
		{
			uint16 off = ((opcode << 8) | _scriptPtr.fetchByte()) * 2;
			res->_useSegVideo2 = false;
			int16 x = _scriptPtr.fetchByte();
			int16 y = _scriptPtr.fetchByte();
			int16 h = y - 199;
			if (h > 0) {
				y = 199;
				x += h;
			}
			debug(DBG_VIDEO, "vid_opcd_0x80 : opcode=0x%X off=0x%X x=%d y=%d", opcode, off, x, y);

			// This switch the polygon database to "cinematic" and probably draws a black polygon
			// over all the screen.
			video->setDataBuffer(res->segCinematic, off);
			video->readAndDrawPolygon(COLOR_BLACK, DEFAULT_ZOOM, Point(x,y));

			continue;
		} 

		// 0100 0000 is set
		if (opcode & 0x40) 
		{
			int16 x, y;
			uint16 off = _scriptPtr.fetchWord() * 2;
			x = _scriptPtr.fetchByte();

			res->_useSegVideo2 = false;

			if (!(opcode & 0x20)) 
			{
				if (!(opcode & 0x10))  // 0001 0000 is set
				{
					x = (x << 8) | _scriptPtr.fetchByte();
				} else {
					x = vmVariables[x];
				}
			} 
			else 
			{
				if (opcode & 0x10) { // 0001 0000 is set
					x += 0x100;
				}
			}

			y = _scriptPtr.fetchByte();

			if (!(opcode & 8))  // 0000 1000 is set
			{
				if (!(opcode & 4)) { // 0000 0100 is set
					y = (y << 8) | _scriptPtr.fetchByte();
				} else {
					y = vmVariables[y];
				}
			}

			uint16 zoom = _scriptPtr.fetchByte();

			if (!(opcode & 2))  // 0000 0010 is set
			{
				if (!(opcode & 1)) // 0000 0001 is set
				{
					--_scriptPtr.pc;
					zoom = 0x40;
				} 
				else 
				{
					zoom = vmVariables[zoom];
				}
			} 
			else 
			{
				
				if (opcode & 1) { // 0000 0001 is set
					res->_useSegVideo2 = true;
					--_scriptPtr.pc;
					zoom = 0x40;
				}
			}
			debug(DBG_VIDEO, "vid_opcd_0x40 : off=0x%X x=%d y=%d", off, x, y);
			video->setDataBuffer(res->_useSegVideo2 ? res->_segVideo2 : res->segCinematic, off);
			video->readAndDrawPolygon(0xFF, zoom, Point(x, y));

			continue;
		} 
		 
		
		if (opcode > 0x1A) 
		{
			error("VirtualMachine::executeThread() ec=0x%X invalid opcode=0x%X", 0xFFF, opcode);
		} 
		else 
		{
			(this->*opcodeTable[opcode])();
		}
		
	}
}

void VirtualMachine::inp_updatePlayer() {

	sys->processEvents();

	if (res->currentPartId == 0x3E89) {
		char c = sys->input.lastChar;
		if (c == 8 || /*c == 0xD |*/ c == 0 || (c >= 'a' && c <= 'z')) {
			vmVariables[VM_VARIABLE_LAST_KEYCHAR] = c & ~0x20;
			sys->input.lastChar = 0;
		}
	}

	int16 lr = 0;
	int16 m = 0;
	int16 ud = 0;

	if (sys->input.dirMask & PlayerInput::DIR_RIGHT) {
		lr = 1;
		m |= 1;
	}
	if (sys->input.dirMask & PlayerInput::DIR_LEFT) {
		lr = -1;
		m |= 2;
	}
	if (sys->input.dirMask & PlayerInput::DIR_DOWN) {
		ud = 1;
		m |= 4;
	}

	vmVariables[VM_VARIABLE_HERO_POS_UP_DOWN] = ud;

	if (sys->input.dirMask & PlayerInput::DIR_UP) {
		vmVariables[VM_VARIABLE_HERO_POS_UP_DOWN] = -1;
	}

	if (sys->input.dirMask & PlayerInput::DIR_UP) { // inpJump
		ud = -1;
		m |= 8;
	}

	vmVariables[VM_VARIABLE_HERO_POS_JUMP_DOWN] = ud;
	vmVariables[VM_VARIABLE_HERO_POS_LEFT_RIGHT] = lr;
	vmVariables[VM_VARIABLE_HERO_POS_MASK] = m;
	int16 button = 0;

	if (sys->input.button) { // inpButton
		button = 1;
		m |= 0x80;
	}

	vmVariables[VM_VARIABLE_HERO_ACTION] = button;
	vmVariables[VM_VARIABLE_HERO_ACTION_POS_MASK] = m;
}

void VirtualMachine::inp_handleSpecialKeys() {

	if (sys->input.pause) {

		if (res->currentPartId != GAME_PART1 && res->currentPartId != GAME_PART2) {
			sys->input.pause = false;
			while (!sys->input.pause) {
				sys->processEvents();
				sys->sleep(200);
			}
		}
		sys->input.pause = false;
	}

	if (sys->input.code) {
		sys->input.code = false;
		if (res->currentPartId != GAME_PART_LAST && res->currentPartId != GAME_PART_FIRST) {
			res->requestedNextPart = GAME_PART_LAST;
		}
	}

	// XXX
	if (vmVariables[0xC9] == 1) {
		warning("VirtualMachine::inp_handleSpecialKeys() unhandled case (vmVariables[0xC9] == 1)");
	}

}

void VirtualMachine::snd_playSound(uint16 resNum, uint8 freq, uint8 vol, uint8 channel) {

	debug(DBG_SND, "snd_playSound(0x%X, %d, %d, %d)", resNum, freq, vol, channel);
	
	MemEntry *me = &res->_memList[resNum];

	if (me->state != 1)
		return;

	
	if (vol == 0) {
		mixer->stopChannel(channel);
	} else {
		MixerChunk mc;
		memset(&mc, 0, sizeof(mc));
		mc.data = me->bufPtr + 8; // skip header
		mc.len = READ_BE_UINT16(me->bufPtr) * 2;
		mc.loopLen = READ_BE_UINT16(me->bufPtr + 2) * 2;
		if (mc.loopLen != 0) {
			mc.loopPos = mc.len;
		}
		assert(freq < 40);
		mixer->playChannel(channel & 3, &mc, frequenceTable[freq], MIN(vol, 0x3F));
	}
	
}

void VirtualMachine::snd_playMusic(uint16 resNum, uint16 delay, uint8 pos) {

	debug(DBG_SND, "snd_playMusic(0x%X, %d, %d)", resNum, delay, pos);

	if (resNum != 0) {
		player->loadSfxModule(resNum, delay, pos);
		player->start();
	} else if (delay != 0) {
		player->setEventsDelay(delay);
	} else {
		player->stop();
	}
}

void VirtualMachine::saveOrLoad(Serializer &ser) {
	Serializer::Entry entries[] = {
		SE_ARRAY(vmVariables, 0x100, Serializer::SES_INT16, VER(1)),
		SE_ARRAY(_scriptStackCalls, 0x100, Serializer::SES_INT16, VER(1)),
		SE_ARRAY(threadsData, 0x40 * 2, Serializer::SES_INT16, VER(1)),
		SE_ARRAY(vmIsChannelActive, 0x40 * 2, Serializer::SES_INT8, VER(1)),
		SE_END()
	};
	ser.saveOrLoadEntries(entries);
}

void VirtualMachine::bypassProtection() 
{
	File f(true);

	if (!f.open("bank0e", res->getDataDir(), "rb")) {
		warning("Unable to bypass protection: add bank0e file to datadir");
	} else {
		Serializer s(&f, Serializer::SM_LOAD, res->_memPtrStart, 2);
		this->saveOrLoad(s);
		res->saveOrLoad(s);
		video->saveOrLoad(s);
		player->saveOrLoad(s);
		mixer->saveOrLoad(s);
	}
	f.close();
}

