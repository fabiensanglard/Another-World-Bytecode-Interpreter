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
struct System;
struct Video;

//For threadsData navigation
#define PC_OFFSET 0
#define REQUESTED_PC_OFFSET 1
#define NUM_DATA_FIELDS 2

//For vmIsChannelActive navigation
#define CURR_STATE 0
#define REQUESTED_STATE 1
#define NUM_THREAD_FIELDS 2

struct VirtualMachine {

	// The type of entries in opcodeTable. This allows "fast" branching
	typedef void (VirtualMachine::*OpcodeStub)();
	static const OpcodeStub opcodeTable[];

	//This table is used to play a sound
	static const uint16_t frequenceTable[];

	Mixer *mixer;
	Resource *res;
	SfxPlayer *player;
	Video *video;
	System *sys;




	
	int16_t vmVariables[VM_NUM_VARIABLES];
	uint16_t _scriptStackCalls[VM_NUM_THREADS];

    
	uint16_t threadsData[NUM_DATA_FIELDS][VM_NUM_THREADS];
	// This array is used: 
	//     0 to save the channel's instruction pointer 
	//     when the channel release control (this happens on a break).

	//     1 When a setVec is requested for the next vm frame.


	uint8_t vmIsChannelActive[NUM_THREAD_FIELDS][VM_NUM_THREADS];







	Ptr _scriptPtr;
	uint8_t _stackPtr;
	bool gotoNextThread;
	bool _fastMode;

	VirtualMachine(Mixer *mix, Resource *res, SfxPlayer *ply, Video *vid, System *stub);
	void init();
	
	void op_movConst();
	void op_mov();
	void op_add();
	void op_addConst();
	void op_call();
	void op_ret();
	void op_pauseThread();
	void op_jmp();
	void op_setSetVect();
	void op_jnz();
	void op_condJmp();
	void op_setPalette();
	void op_resetThread();
	void op_selectVideoPage();
	void op_fillVideoPage();
	void op_copyVideoPage();
	void op_blitFramebuffer();
	void op_killThread();	
	void op_drawString();
	void op_sub();
	void op_and();
	void op_or();
	void op_shl();
	void op_shr();
	void op_playSound();
	void op_updateMemList();
	void op_playMusic();

	void initForPart(uint16_t partId);
	void setupPart(uint16_t partId);
	void checkThreadRequests();
	void hostFrame();
	void executeThread();

	void inp_updatePlayer();
	void inp_handleSpecialKeys();
	
	void snd_playSound(uint16_t resNum, uint8_t freq, uint8_t vol, uint8_t channel);
	void snd_playMusic(uint16_t resNum, uint16_t delay, uint8_t pos);
	
	void saveOrLoad(Serializer &ser);
	void bypassProtection();
};

#endif
