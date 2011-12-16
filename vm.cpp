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
#include "systemstub.h"


VirtualMachine::VirtualMachine(Mixer *mix, Resource *res, SfxPlayer *ply, Video *vid, SystemStub *stub)
	: mixer(mix), _res(res), _ply(ply), _vid(vid), _stub(stub) {
}

void VirtualMachine::init() {
	memset(vmVariables, 0, sizeof(vmVariables));
	vmVariables[0x54] = 0x81;
	vmVariables[VM_VARIABLE_RANDOM_SEED] = time(0);
	_fastMode = false;
	_ply->_markVar = &vmVariables[VM_VARIABLE_MUS_MARK];
}

void VirtualMachine::op_movConst() {
	uint8 i = _scriptPtr.fetchByte();
	int16 n = _scriptPtr.fetchWord();
	debug(DBG_LOGIC, "VirtualMachine::op_movConst(0x%02X, %d)", i, n);
	vmVariables[i] = n;
}

void VirtualMachine::op_mov() {
	uint8 i = _scriptPtr.fetchByte();
	uint8 j = _scriptPtr.fetchByte();	
	debug(DBG_LOGIC, "VirtualMachine::op_mov(0x%02X, 0x%02X)", i, j);
	vmVariables[i] = vmVariables[j];
}

void VirtualMachine::op_add() {
	uint8 i = _scriptPtr.fetchByte();
	uint8 j = _scriptPtr.fetchByte();
	debug(DBG_LOGIC, "VirtualMachine::op_add(0x%02X, 0x%02X)", i, j);
	vmVariables[i] += vmVariables[j];
}

void VirtualMachine::op_addConst() {
	if (_res->_curPtrsId == 0x3E86 && _scriptPtr.pc == _res->_segCode + 0x6D48) {
		warning("VirtualMachine::op_addConst() hack for non-stop looping gun sound bug");
		// the script 0x27 slot 0x17 doesn't stop the gun sound from looping, I 
		// don't really know why ; for now, let's play the 'stopping sound' like 
		// the other scripts do
		//  (0x6D43) jmp(0x6CE5)
		//  (0x6D46) break
		//  (0x6D47) VAR(6) += -50
		snd_playSound(0x5B, 1, 64, 1);
	}
	uint8 i = _scriptPtr.fetchByte();
	int16 n = _scriptPtr.fetchWord();
	debug(DBG_LOGIC, "VirtualMachine::op_addConst(0x%02X, %d)", i, n);
	vmVariables[i] += n;
}

void VirtualMachine::op_call() {

	uint16 offset = _scriptPtr.fetchWord();
	uint8 sp = _stackPtr;

	debug(DBG_LOGIC, "VirtualMachine::op_call(0x%X)", offset);
	_scriptStackCalls[sp] = _scriptPtr.pc - _res->_segCode;
	if (_stackPtr == 0xFF) {
		error("VirtualMachine::op_call() ec=0x%X stack overflow", 0x8F);
	}
	++_stackPtr;
	_scriptPtr.pc = _res->_segCode + offset ;
}

void VirtualMachine::op_ret() {
	debug(DBG_LOGIC, "VirtualMachine::op_ret()");
	if (_stackPtr == 0) {
		error("VirtualMachine::op_ret() ec=0x%X stack underflow", 0x8F);
	}	
	--_stackPtr;
	uint8 sp = _stackPtr;
	_scriptPtr.pc = _res->_segCode + _scriptStackCalls[sp];
}

void VirtualMachine::op_break() {
	debug(DBG_LOGIC, "VirtualMachine::op_break()");
	_scriptHalted = true;
}

void VirtualMachine::op_jmp() {
	uint16 off = _scriptPtr.fetchWord();
	debug(DBG_LOGIC, "VirtualMachine::op_jmp(0x%02X)", off);
	_scriptPtr.pc = _res->_segCode + off;	
}

void VirtualMachine::op_setSetVect() {
	uint8 threadId = _scriptPtr.fetchByte();
	uint16 n = _scriptPtr.fetchWord();
	debug(DBG_LOGIC, "VirtualMachine::op_setSetVect(0x%X, 0x%X)", threadId, n);
	_scriptSlotsPos[1][threadId] = n;
}

void VirtualMachine::op_jnz() {
	uint8 i = _scriptPtr.fetchByte();
	debug(DBG_LOGIC, "VirtualMachine::op_jnz(0x%02X)", i);
	--vmVariables[i];
	if (vmVariables[i] != 0) {
		op_jmp();
	} else {
		_scriptPtr.fetchWord();
	}
}


void VirtualMachine::op_condJmp() {

#ifdef BYPASS_PROTECTION
	if (_res->_curPtrsId == 0x3E80 && _scriptPtr.pc == _res->_segCode + 0xCB9) {
		// (0x0CB8) condJmp(0x80, VAR(41), VAR(30), 0xCD3)
		*(_scriptPtr.pc + 0x00) = 0x81;
		*(_scriptPtr.pc + 0x03) = 0x0D;
		*(_scriptPtr.pc + 0x04) = 0x24;
		// (0x0D4E) condJmp(0x4, VAR(50), 6, 0xDBC)		
		*(_scriptPtr.pc + 0x99) = 0x0D;
		*(_scriptPtr.pc + 0x9A) = 0x5A;
		printf("VirtualMachine::op_condJmp() bypassing protection");
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
	debug(DBG_LOGIC, "VirtualMachine::op_condJmp(%d, 0x%02X, 0x%02X)", opcode, b, a);

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
	uint16 i = _scriptPtr.fetchWord();
	debug(DBG_LOGIC, "VirtualMachine::op_changePalette(%d)", i);
	_vid->_newPal = i >> 8;
}

void VirtualMachine::op_resetScript() {

	uint8 threadId = _scriptPtr.fetchByte();
	uint8 i =        _scriptPtr.fetchByte();

	// FCS: WTF cryptic as hell !!
	//int8 n = (i & 0x3F) - threadId;  //0x3F = 0011 1111
	// The following is so much clearer

	//Make sure i within [0-VM_NUM_THREADS-1]
	i = i & (VM_NUM_THREADS-1) ;
	int8 n = i - threadId;

	if (n < 0) {
		warning("VirtualMachine::op_resetScript() ec=0x%X (n < 0)", 0x880);
		return;
	}
	++n;
	uint8 a = _scriptPtr.fetchByte();

	debug(DBG_LOGIC, "VirtualMachine::op_resetScript(%d, %d, %d)", threadId, i, a);

	if (a == 2) {
		uint16 *p = &_scriptSlotsPos[1][threadId];
		while (n--) {
			*p++ = 0xFFFE;
		}
	} else if (a < 2) {
		uint8 *p = &vmChannelPaused[1][threadId];
		while (n--) {
			*p++ = a;
		}
	}
}

void VirtualMachine::op_selectPage() {
	uint8 i = _scriptPtr.fetchByte();
	debug(DBG_LOGIC, "VirtualMachine::op_selectPage(%d)", i);
	_vid->changePagePtr1(i);
}

void VirtualMachine::op_fillPage() {
	uint8 screen = _scriptPtr.fetchByte();
	uint8 color = _scriptPtr.fetchByte();
	debug(DBG_LOGIC, "VirtualMachine::op_fillPage(%d, %d)", screen, color);
	_vid->fillPage(screen, color);
}

void VirtualMachine::op_copyPage() {
	uint8 i = _scriptPtr.fetchByte();
	uint8 j = _scriptPtr.fetchByte();
	debug(DBG_LOGIC, "VirtualMachine::op_copyPage(%d, %d)", i, j);
	_vid->copyPage(i, j, vmVariables[VM_VARIABLE_SCROLL_Y]);
}

void VirtualMachine::op_updateDisplay() {
	uint8 page = _scriptPtr.fetchByte();
	debug(DBG_LOGIC, "VirtualMachine::op_updateDisplay(%d)", page);
	inp_handleSpecialKeys();


	if (_res->_curPtrsId == 0x3E80 && vmVariables[0x67] == 1) {
		
		vmVariables[0xDC] = 0x21;
	}

	static uint32 tstamp = 0;
	if (!_fastMode) {
		int32 delay = _stub->getTimeStamp() - tstamp;
		int32 pause = vmVariables[VM_VARIABLE_PAUSE_SLICES] * 20 - delay;
		if (pause > 0) {
			_stub->sleep(pause);
		}
		tstamp = _stub->getTimeStamp();
	}
	vmVariables[0xF7] = 0;

	_vid->updateDisplay(page);
}

void VirtualMachine::op_halt() {
	debug(DBG_LOGIC, "VirtualMachine::op_halt()");
	_scriptPtr.pc = _res->_segCode + 0xFFFF;
	_scriptHalted = true;
}

void VirtualMachine::op_drawString() {
	uint16 stringId = _scriptPtr.fetchWord();
	uint16 x = _scriptPtr.fetchByte();
	uint16 y = _scriptPtr.fetchByte();
	uint16 color = _scriptPtr.fetchByte();

	debug(DBG_LOGIC, "VirtualMachine::op_drawString(0x%03X, %d, %d, %d)", stringId, x, y, color);

	_vid->drawString(color, x, y, stringId);
}

void VirtualMachine::op_sub() {
	uint8 i = _scriptPtr.fetchByte();
	uint8 j = _scriptPtr.fetchByte();
	debug(DBG_LOGIC, "VirtualMachine::op_sub(0x%02X, 0x%02X)", i, j);
	vmVariables[i] -= vmVariables[j];
}

void VirtualMachine::op_and() {
	uint8 variableId = _scriptPtr.fetchByte();
	uint16 n = _scriptPtr.fetchWord();
	debug(DBG_LOGIC, "VirtualMachine::op_and(0x%02X, %d)", variableId, n);
	vmVariables[variableId] = (uint16)vmVariables[variableId] & n;
}

void VirtualMachine::op_or() {
	uint8 variableId = _scriptPtr.fetchByte();
	uint16 value = _scriptPtr.fetchWord();
	debug(DBG_LOGIC, "VirtualMachine::op_or(0x%02X, %d)", variableId, value);
	vmVariables[variableId] = (uint16)vmVariables[variableId] | value;
}

void VirtualMachine::op_shl() {
	uint8 variableId = _scriptPtr.fetchByte();
	uint16 leftShiftValue = _scriptPtr.fetchWord();
	debug(DBG_LOGIC, "VirtualMachine::op_shl(0x%02X, %d)", variableId, leftShiftValue);
	vmVariables[variableId] = (uint16)vmVariables[variableId] << leftShiftValue;
}

void VirtualMachine::op_shr() {
	uint8 variableId = _scriptPtr.fetchByte();
	uint16 rightShiftValyeue = _scriptPtr.fetchWord();
	debug(DBG_LOGIC, "VirtualMachine::op_shr(0x%02X, %d)", variableId, rightShiftValyeue);
	vmVariables[variableId] = (uint16)vmVariables[variableId] >> rightShiftValyeue;
}

void VirtualMachine::op_playSound() {
	uint16 resNum = _scriptPtr.fetchWord();
	uint8 freq = _scriptPtr.fetchByte();
	uint8 vol = _scriptPtr.fetchByte();
	uint8 channel = _scriptPtr.fetchByte();
	debug(DBG_LOGIC, "VirtualMachine::op_playSound(0x%X, %d, %d, %d)", resNum, freq, vol, channel);
	snd_playSound(resNum, freq, vol, channel);
}

void VirtualMachine::op_updateMemList() {
	uint16 num = _scriptPtr.fetchWord();
	debug(DBG_LOGIC, "VirtualMachine::op_updateMemList(%d)", num);
	if (num == 0) {
		_ply->stop();
		mixer->stopAll();
		_res->invalidateRes();
	} else {
		_res->update(num);
	}
}

void VirtualMachine::op_playMusic() {
	uint16 resNum = _scriptPtr.fetchWord();
	uint16 delay = _scriptPtr.fetchWord();
	uint8 pos = _scriptPtr.fetchByte();
	debug(DBG_LOGIC, "VirtualMachine::op_playMusic(0x%X, %d, %d)", resNum, delay, pos);
	snd_playMusic(resNum, delay, pos);
}

void VirtualMachine::restartAt(uint16 ptrId) {
	_ply->stop();
	mixer->stopAll();
	vmVariables[0xE4] = 0x14;
	_res->setupPtrs(ptrId);
	memset((uint8 *)_scriptSlotsPos, 0xFF, sizeof(_scriptSlotsPos));
	memset((uint8 *)vmChannelPaused, 0, sizeof(vmChannelPaused));
	_scriptSlotsPos[0][0] = 0;	
}

/* 
     This is called every frames in the infinite loop.
*/
void VirtualMachine::setupScripts() {


	if (_res->_newPtrsId != 0) {
		restartAt(_res->_newPtrsId);
		_res->_newPtrsId = 0;
	}

	
	// Check if a state update has been requested for any thread during the previous VM execution:
	//      - Pause
	//      - Jump

	// JUMP:
	// Note: If a jump has been requested, the jump destination is stored
	// in _scriptSlotsPos[1]. Otherwise _scriptSlotsPos[1] == 0xFFFF

	// PAUSE:
	// Note: If a pause has been requested it is stored in  vmChannelPaused[1][i]

	for (int i = 0; i < VM_NUM_THREADS; ++i) {

		vmChannelPaused[0][i] = vmChannelPaused[1][i];

		uint16 n = _scriptSlotsPos[1][i];
		if (n != VM_NO_SETVEC_REQUESTED) {
			_scriptSlotsPos[0][i] = (n == 0xFFFE) ? VM_INACTIVE_THREAD : n;
			_scriptSlotsPos[1][i] = VM_NO_SETVEC_REQUESTED;
		}
	}
}

void VirtualMachine::runScripts() {

	// Run the Virtual Machine for every active threads (one vm frame).
	// Inactive threads are marked with a thread instruction pointer set to 0xFFFF (VM_INACTIVE_THREAD).
	// A thread must feature a break opcode so the interpreter can move to the next thread.

	for (int i = 0; i < VM_NUM_THREADS; ++i) {

		if (vmChannelPaused[0][i])
			continue;
		
		uint16 n = _scriptSlotsPos[0][i];

		if (n != VM_INACTIVE_THREAD) {

			// Set the script pointer to the right location.
			// script pc is used in executeScript in order
			// to get the next opcode.
			_scriptPtr.pc = _res->_segCode + n;
			_stackPtr = 0;

			_scriptHalted = false;
			debug(DBG_LOGIC, "VirtualMachine::runScripts() i=0x%02X n=0x%02X *p=0x%02X", i, n, *_scriptPtr.pc);
			executeScript();

			//Since .pc is going to be modified by this next loop iteration, we need to save it.
			_scriptSlotsPos[0][i] = _scriptPtr.pc - _res->_segCode;


			debug(DBG_LOGIC, "VirtualMachine::runScripts() i=0x%02X pos=0x%X", i, _scriptSlotsPos[0][i]);
			if (_stub->_pi.quit) {
				break;
			}
		}
		
	}
}

#define COLOR_BLACK 0xFF
#define DEFAULT_ZOOM 0x40

void VirtualMachine::executeScript() {
	while (!_scriptHalted) {
		uint8 opcode = _scriptPtr.fetchByte();

		// 1000 0000 is set
		if (opcode & 0x80) 
		{
			uint16 off = ((opcode << 8) | _scriptPtr.fetchByte()) * 2;
			_res->_useSegVideo2 = false;
			int16 x = _scriptPtr.fetchByte();
			int16 y = _scriptPtr.fetchByte();
			int16 h = y - 199;
			if (h > 0) {
				y = 199;
				x += h;
			}
			debug(DBG_VIDEO, "vid_opcd_0x80 : opcode=0x%X off=0x%X x=%d y=%d", opcode, off, x, y);
			_vid->setDataBuffer(_res->_segVideo1, off);
			_vid->drawShape(COLOR_BLACK, DEFAULT_ZOOM, Point(x,y));
		} 
		// 0100 0000 is set
		else if (opcode & 0x40) 
		{
			int16 x, y;
			uint16 off = _scriptPtr.fetchWord() * 2;
			x = _scriptPtr.fetchByte();
			_res->_useSegVideo2 = false;
			if (!(opcode & 0x20)) 
			{
				if (!(opcode & 0x10)) 
				{
					x = (x << 8) | _scriptPtr.fetchByte();
				} else {
					x = vmVariables[x];
				}
			} 
			else 
			{
				if (opcode & 0x10) {
					x += 0x100;
				}
			}

			y = _scriptPtr.fetchByte();

			if (!(opcode & 8)) 
			{
				if (!(opcode & 4)) {
					y = (y << 8) | _scriptPtr.fetchByte();
				} else {
					y = vmVariables[y];
				}
			}

			uint16 zoom = _scriptPtr.fetchByte();

			if (!(opcode & 2)) 
			{
				if (!(opcode & 1)) 
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
				if (opcode & 1) {
					_res->_useSegVideo2 = true;
					--_scriptPtr.pc;
					zoom = 0x40;
				}
			}
			debug(DBG_VIDEO, "vid_opcd_0x40 : off=0x%X x=%d y=%d", off, x, y);
			_vid->setDataBuffer(_res->_useSegVideo2 ? _res->_segVideo2 : _res->_segVideo1, off);
			_vid->drawShape(0xFF, zoom, Point(x, y));
		} 
		else 
		{
			if (opcode > 0x1A) 
			{
				error("VirtualMachine::executeScript() ec=0x%X invalid opcode=0x%X", 0xFFF, opcode);
			} 
			else 
			{
				(this->*opcodeTable[opcode])();
			}
		}
	}
}

void VirtualMachine::inp_updatePlayer() {

	_stub->processEvents();

	if (_res->_curPtrsId == 0x3E89) {
		char c = _stub->_pi.lastChar;
		if (c == 8 | /*c == 0xD |*/ c == 0 | (c >= 'a' && c <= 'z')) {
			vmVariables[VM_VARIABLE_LAST_KEYCHAR] = c & ~0x20;
			_stub->_pi.lastChar = 0;
		}
	}

	int16 lr = 0;
	int16 m = 0;
	int16 ud = 0;

	if (_stub->_pi.dirMask & PlayerInput::DIR_RIGHT) {
		lr = 1;
		m |= 1;
	}
	if (_stub->_pi.dirMask & PlayerInput::DIR_LEFT) {
		lr = -1;
		m |= 2;
	}
	if (_stub->_pi.dirMask & PlayerInput::DIR_DOWN) {
		ud = 1;
		m |= 4;
	}

	vmVariables[VM_VARIABLE_HERO_POS_UP_DOWN] = ud;

	if (_stub->_pi.dirMask & PlayerInput::DIR_UP) {
		vmVariables[VM_VARIABLE_HERO_POS_UP_DOWN] = -1;
	}

	if (_stub->_pi.dirMask & PlayerInput::DIR_UP) { // inpJump
		ud = -1;
		m |= 8;
	}

	vmVariables[VM_VARIABLE_HERO_POS_JUMP_DOWN] = ud;
	vmVariables[VM_VARIABLE_HERO_POS_LEFT_RIGHT] = lr;
	vmVariables[VM_VARIABLE_HERO_POS_MASK] = m;
	int16 button = 0;

	if (_stub->_pi.button) { // inpButton
		button = 1;
		m |= 0x80;
	}

	vmVariables[VM_VARIABLE_HERO_ACTION] = button;
	vmVariables[VM_VARIABLE_HERO_ACTION_POS_MASK] = m;
}

void VirtualMachine::inp_handleSpecialKeys() {

	if (_stub->_pi.pause) {

		if (_res->_curPtrsId != 0x3E80 && _res->_curPtrsId != 0x3E81) {
			_stub->_pi.pause = false;
			while (!_stub->_pi.pause) {
				_stub->processEvents();
				_stub->sleep(200);
			}
		}
		_stub->_pi.pause = false;
	}

	if (_stub->_pi.code) {
		_stub->_pi.code = false;
		if (_res->_curPtrsId != 0x3E89 && _res->_curPtrsId != 0x3E80) {
			_res->_newPtrsId = 0x3E89;
		}
	}

	// XXX
	if (vmVariables[0xC9] == 1) {
		warning("VirtualMachine::inp_handleSpecialKeys() unhandled case (vmVariables[0xC9] == 1)");
	}

}

void VirtualMachine::snd_playSound(uint16 resNum, uint8 freq, uint8 vol, uint8 channel) {
	debug(DBG_SND, "snd_playSound(0x%X, %d, %d, %d)", resNum, freq, vol, channel);
	
	MemEntry *me = &_res->_memList[resNum];
	if (me->valid == 1) {
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
}

void VirtualMachine::snd_playMusic(uint16 resNum, uint16 delay, uint8 pos) {
	debug(DBG_SND, "snd_playMusic(0x%X, %d, %d)", resNum, delay, pos);
	if (resNum != 0) {
		_ply->loadSfxModule(resNum, delay, pos);
		_ply->start();
	} else if (delay != 0) {
		_ply->setEventsDelay(delay);
	} else {
		_ply->stop();
	}
}

void VirtualMachine::saveOrLoad(Serializer &ser) {
	Serializer::Entry entries[] = {
		SE_ARRAY(vmVariables, 0x100, Serializer::SES_INT16, VER(1)),
		SE_ARRAY(_scriptStackCalls, 0x100, Serializer::SES_INT16, VER(1)),
		SE_ARRAY(_scriptSlotsPos, 0x40 * 2, Serializer::SES_INT16, VER(1)),
		SE_ARRAY(vmChannelPaused, 0x40 * 2, Serializer::SES_INT8, VER(1)),
		SE_END()
	};
	ser.saveOrLoadEntries(entries);
}
