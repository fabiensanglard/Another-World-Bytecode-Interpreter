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
#include "systemstub.h"
#include "util.h"


static const char *USAGE = 
	"Raw - Another World Interpreter\n"
	"Usage: raw [OPTIONS]...\n"
	"  --datapath=PATH   Path to where the game is installed (default '.')\n"
	"  --savepath=PATH   Path to where the save files are stored (default '.')\n";

static bool parseOption(const char *arg, const char *longCmd, const char **opt) {
	bool ret = false;
	if (arg[0] == '-' && arg[1] == '-') {
		if (strncmp(arg + 2, longCmd, strlen(longCmd)) == 0) {
			*opt = arg + 2 + strlen(longCmd);
			ret = true;
		}
	}
	return ret;
}

#undef main
int main(int argc, char *argv[]) {
	const char *dataPath = ".";
	const char *savePath = ".";
	for (int i = 1; i < argc; ++i) {
		bool opt = false;
		if (strlen(argv[i]) >= 2) {
			opt |= parseOption(argv[i], "datapath=", &dataPath);
			opt |= parseOption(argv[i], "savepath=", &savePath);
		}
		if (!opt) {
			printf(USAGE);
			return 0;
		}
	}
	//FCS
	//g_debugMask = DBG_INFO; // DBG_LOGIC | DBG_BANK | DBG_VIDEO | DBG_SER | DBG_SND
	g_debugMask = 0 ;//DBG_INFO |  DBG_LOGIC | DBG_BANK | DBG_VIDEO | DBG_SER | DBG_SND ;
	SystemStub *stub = SystemStub_SDL_create();
	Engine *e = new Engine(stub, dataPath, savePath);
	e->run();
	delete e;
	delete stub;
	return 0;
}



/*
   Game was originally made with 16. SIXTEEN colors. Running on 320x200 (64,000 pixels.)

   Great fan site here: https://sites.google.com/site/interlinkknight/anotherworld
   Contains the wheelcode :P ! 

   A lot of details can be found regarding the game and engine architecture at:
   http://www.anotherworld.fr/anotherworld_uk/another_world.htm

   The chronology of the game implementation can retraced via the ordering of the opcodes:
   The sound and music opcode are at the end: Music and sound was done at the end.

   Call tree:
   =========

   main
   {
       SystemStub *stub = SystemStub_SDL_create();
       Engine *e = new Engine();
	   e->run()
	   {
	      _stub->init("Out Of This World");
		  setup();
	      _log.restartAt(0x3E80); // demo starts at 0x3E81

	     while (!_stub->_pi.quit) 
		 {
		   _log.setupScripts();
		   _log.inp_updatePlayer();
		    processInput();
		   _log.runScripts();
	     }

	     finish();
	     _stub->destroy();
	   }
   }


   Virtual Machine:
   ================

	   Seems the threading model is collaborative multi-tasking (as opposed to preemptive multitasking):
	   A thread (called a Channel on Eric Chahi website) will release the hand to the next one via the
	   break opcode.

	   It seems that even when a setvec is requested by a thread, we cannot set the instruction pointer
	   yet. The thread is allowed to keep on executing its code for the remaining of the vm frame.




   Video :
   =======
	   Double buffer architecture. AW optcodes even has a special instruction for blitting from one
	   frame buffer to an other.



   Sound :
   =======
	   Mixing is done on software.




   Endianess:
   ==========

   Atari and Amiga used bigEndian CPUs. Data are hence stored within BANK in big endian format.
   On an Intel or ARM CPU data will have to be transformed when read.



   The original codebase contained a looooot of cryptic hexa values.
   0x100 (for 256 variables)
   0x400 (for one kilobyte)
   0x40 (for num threads)
   0x3F (num thread mask)
   I cleaned that up.

   Virtual Machine was named "logic" ?!?!

   Questions & Answers :
   =====================

   Q: How does the interpreter deals with the CPU speed ?! A pentium is a tad faster than a Motorola 68000
      after all.
   A: ? No idea ?




*/