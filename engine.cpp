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

#include "engine.h"
#include "file.h"
#include "serializer.h"
#include "sys.h"


Engine::Engine(System *paramSys, const char *dataDir, const char *saveDir)
	: _stub(paramSys), vm(&mixer, &_res, &player, &video, _stub), mixer(_stub), _res(&video, dataDir), 
	player(&mixer, &_res, _stub), video(&_res, _stub), _dataDir(dataDir), _saveDir(saveDir), _stateSlot(0) {
}

void Engine::run() {

	while (!_stub->input.quit) {

		vm.setupScripts();

		vm.inp_updatePlayer();

		processInput();

		vm.hostFrame();
	}


}

Engine::~Engine(){

	finish();
	_stub->destroy();
}


void Engine::init() {

	
	//Init system
	_stub->init("Out Of This World");

	video.init();

	_res.allocMemBlock();

	_res.readEntries();

	vm.init();

	mixer.init();

	player.init();

	//Init virtual machine
	vm.initWithByteCodeAddress(VM_BYTECODE_STARTUP_ADDRESS); // demo starts at 0x3E81

}

void Engine::finish() {
	player.free();
	mixer.free();
	_res.freeMemBlock();
}

void Engine::processInput() {
	if (_stub->input.load) {
		loadGameState(_stateSlot);
		_stub->input.load = false;
	}
	if (_stub->input.save) {
		saveGameState(_stateSlot, "quicksave");
		_stub->input.save = false;
	}
	if (_stub->input.fastMode) {
		vm._fastMode = !vm._fastMode;
		_stub->input.fastMode = false;
	}
	if (_stub->input.stateSlot != 0) {
		int8 slot = _stateSlot + _stub->input.stateSlot;
		if (slot >= 0 && slot < MAX_SAVE_SLOTS) {
			_stateSlot = slot;
			debug(DBG_INFO, "Current game state slot is %d", _stateSlot);
		}
		_stub->input.stateSlot = 0;
	}
}

void Engine::makeGameStateName(uint8 slot, char *buf) {
	sprintf(buf, "raw.s%02d", slot);
}

void Engine::saveGameState(uint8 slot, const char *desc) {
	char stateFile[20];
	makeGameStateName(slot, stateFile);
	File f(true);
	if (!f.open(stateFile, _saveDir, "wb")) {
		warning("Unable to save state file '%s'", stateFile);
	} else {
		// header
		f.writeUint32BE('AWSV');
		f.writeUint16BE(Serializer::CUR_VER);
		f.writeUint16BE(0);
		char hdrdesc[32];
		strncpy(hdrdesc, desc, sizeof(hdrdesc) - 1);
		f.write(hdrdesc, sizeof(hdrdesc));
		// contents
		Serializer s(&f, Serializer::SM_SAVE, _res._memPtrStart);
		vm.saveOrLoad(s);
		_res.saveOrLoad(s);
		video.saveOrLoad(s);
		player.saveOrLoad(s);
		mixer.saveOrLoad(s);
		if (f.ioErr()) {
			warning("I/O error when saving game state");
		} else {
			debug(DBG_INFO, "Saved state to slot %d", _stateSlot);
		}
	}
}

void Engine::loadGameState(uint8 slot) {
	char stateFile[20];
	makeGameStateName(slot, stateFile);
	File f(true);
	if (!f.open(stateFile, _saveDir, "rb")) {
		warning("Unable to open state file '%s'", stateFile);
	} else {
		uint32 id = f.readUint32BE();
		if (id != 'AWSV') {
			warning("Bad savegame format");
		} else {
			// mute
			player.stop();
			mixer.stopAll();
			// header
			uint16 ver = f.readUint16BE();
			f.readUint16BE();
			char hdrdesc[32];
			f.read(hdrdesc, sizeof(hdrdesc));
			// contents
			Serializer s(&f, Serializer::SM_LOAD, _res._memPtrStart, ver);
			vm.saveOrLoad(s);
			_res.saveOrLoad(s);
			video.saveOrLoad(s);
			player.saveOrLoad(s);
			mixer.saveOrLoad(s);
		}
		if (f.ioErr()) {
			warning("I/O error when loading game state");
		} else {
			debug(DBG_INFO, "Loaded state from slot %d", _stateSlot);
		}
	}
}
