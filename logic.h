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

#ifndef __LOGIC_H__
#define __LOGIC_H__

#define BYPASS_PROTECTION

#include "intern.h"

struct Mixer;
struct Resource;
struct Serializer;
struct SfxPlayer;
struct SystemStub;
struct Video;

struct Logic {
	typedef void (Logic::*OpcodeStub)();

	enum ScriptVars {
		VAR_RANDOM_SEED          = 0x3C,
		
		VAR_LAST_KEYCHAR         = 0xDA,

		VAR_HERO_POS_UP_DOWN     = 0xE5,

		VAR_MUS_MARK             = 0xF4,

		VAR_SCROLL_Y             = 0xF9,
		VAR_HERO_ACTION          = 0xFA,
		VAR_HERO_POS_JUMP_DOWN   = 0xFB,
		VAR_HERO_POS_LEFT_RIGHT  = 0xFC,
		VAR_HERO_POS_MASK        = 0xFD,
		VAR_HERO_ACTION_POS_MASK = 0xFE,
		VAR_PAUSE_SLICES         = 0xFF
	};
	
	static const OpcodeStub _opTable[];
	static const uint16 _freqTable[];

	Mixer *_mix;
	Resource *_res;
	SfxPlayer *_ply;
	Video *_vid;
	SystemStub *_stub;

	int16 _scriptVar_0xBF;
	int16 _scriptVars[0x100];
	uint16 _scriptStackCalls[0x40];
	uint16 _scriptSlotsPos[2][0x40];
	uint8 _scriptPaused[2][0x40];
	Ptr _scriptPtr;
	uint8 _stackPtr;
	bool _scriptHalted;
	bool _fastMode;

	Logic(Mixer *mix, Resource *res, SfxPlayer *ply, Video *vid, SystemStub *stub);
	void init();
	
	void op_movConst();
	void op_mov();
	void op_add();
	void op_addConst();
	void op_call();
	void op_ret();
	void op_break();
	void op_jmp();
	void op_setScriptSlot();
	void op_jnz();
	void op_condJmp();
	void op_setPalette();
	void op_resetScript();
	void op_selectPage();
	void op_fillPage();
	void op_copyPage();
	void op_updateDisplay();
	void op_halt();	
	void op_drawString();
	void op_sub();
	void op_and();
	void op_or();
	void op_shl();
	void op_shr();
	void op_playSound();
	void op_updateMemList();
	void op_playMusic();

	void restartAt(uint16 ptrId);
	void setupPtrs(uint16 ptrId);
	void setupScripts();
	void runScripts();
	void executeScript();

	void inp_updatePlayer();
	void inp_handleSpecialKeys();
	
	void snd_playSound(uint16 resNum, uint8 freq, uint8 vol, uint8 channel);
	void snd_playMusic(uint16 resNum, uint16 delay, uint8 pos);
	
	void saveOrLoad(Serializer &ser);
};

#endif
