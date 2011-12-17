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

#define BYPASS_PROTECTION 1

#include "intern.h"

#define VM_NUM_THREADS 64
#define VM_NUM_VARIABLES 256
#define VM_NO_SETVEC_REQUESTED 0xFFFF
#define VM_INACTIVE_THREAD    0xFFFF

enum ScriptVars {
		VM_VARIABLE_RANDOM_SEED          = 0x3C,
		
		VM_VARIABLE_LAST_KEYCHAR         = 0xDA,

		VM_VARIABLE_HERO_POS_UP_DOWN     = 0xE5,

		VM_VARIABLE_MUS_MARK             = 0xF4,

		VM_VARIABLE_SCROLL_Y             = 0xF9, // = 239
		VM_VARIABLE_HERO_ACTION          = 0xFA,
		VM_VARIABLE_HERO_POS_JUMP_DOWN   = 0xFB,
		VM_VARIABLE_HERO_POS_LEFT_RIGHT  = 0xFC,
		VM_VARIABLE_HERO_POS_MASK        = 0xFD,
		VM_VARIABLE_HERO_ACTION_POS_MASK = 0xFE,
		VM_VARIABLE_PAUSE_SLICES         = 0xFF
	};

struct Mixer;
struct Resource;
struct Serializer;
struct SfxPlayer;
struct SystemStub;
struct Video;

struct VirtualMachine {

	// The type of entries in opcodeTable. This allows "fast" branching
	typedef void (VirtualMachine::*OpcodeStub)();
	static const OpcodeStub opcodeTable[];

	//This table is used to play a sound
	static const uint16 frequenceTable[];

	Mixer *mixer;
	Resource *_res;
	SfxPlayer *player;
	Video *video;
	SystemStub *_stub;




	
	int16 vmVariables[VM_NUM_VARIABLES];
	uint16 _scriptStackCalls[VM_NUM_THREADS];

    
	uint16 _scriptSlotsPos[2][VM_NUM_THREADS];
	// This array is used: 
	//     0 to save the channel's instruction pointer 
	//     when the channel release control (this happens on a break).
	//     1 When a setVec is requested for the next vm frame.


	uint8 vmChannelPaused[2][VM_NUM_THREADS];







	Ptr _scriptPtr;
	uint8 _stackPtr;
	bool _scriptHalted;
	bool _fastMode;

	VirtualMachine(Mixer *mix, Resource *res, SfxPlayer *ply, Video *vid, SystemStub *stub);
	void init();
	
	void op_movConst();
	void op_mov();
	void op_add();
	void op_addConst();
	void op_call();
	void op_ret();
	void op_break();
	void op_jmp();
	void op_setSetVect();
	void op_jnz();
	void op_condJmp();
	void op_setPalette();
	void op_resetThread();
	void op_selectPage();
	void op_fillPage();
	void op_copyPage();
	void op_blitFramebuffer();
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
	void hostFrame();
	void executeScript();

	void inp_updatePlayer();
	void inp_handleSpecialKeys();
	
	void snd_playSound(uint16 resNum, uint8 freq, uint8 vol, uint8 channel);
	void snd_playMusic(uint16 resNum, uint16 delay, uint8 pos);
	
	void saveOrLoad(Serializer &ser);
};

#endif
