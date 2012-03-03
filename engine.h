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

#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "intern.h"
#include "vm.h"
#include "mixer.h"
#include "sfxplayer.h"
#include "resource.h"
#include "video.h"

struct System;

struct Engine {
	enum {
		MAX_SAVE_SLOTS = 100
	};

	System *sys;
	VirtualMachine vm;
	Mixer mixer;
	Resource res;
	SfxPlayer player;
	Video video;
	const char *_dataDir, *_saveDir;
	uint8_t _stateSlot;

	Engine(System *stub, const char *dataDir, const char *saveDir);
	~Engine();

	void run();
	void init();
	void finish();
	void processInput();
	
	void makeGameStateName(uint8_t slot, char *buf);
	void saveGameState(uint8_t slot, const char *desc);
	void loadGameState(uint8_t slot);
	const char* getDataDir();
};

#endif
