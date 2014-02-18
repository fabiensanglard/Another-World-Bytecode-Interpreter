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
#include "sys.h"
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

/*
	We use here a design pattern found in Doom3:
	An Abstract Class pointer pointing to the implementation on the Heap.
*/
//extern System *System_SDL_create();
extern System *stub ;//= System_SDL_create();

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
			printf("%s",USAGE);
			return 0;
		}
	}
	//FCS
	//g_debugMask = DBG_INFO; // DBG_VM | DBG_BANK | DBG_VIDEO | DBG_SER | DBG_SND
	g_debugMask = DBG_RES ;
	//g_debugMask = 0 ;//DBG_INFO |  DBG_VM | DBG_BANK | DBG_VIDEO | DBG_SER | DBG_SND ;
	
	Engine* e = new Engine(stub, dataPath, savePath);
	e->init();
	e->run();


	delete e;

	//delete stub;

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

   SDLSystem       systemImplementaion ;
   System *sys = & systemImplementaion ;

   main
   {
       
       Engine *e = new Engine();
	   e->run()
	   {
	      sys->init("Out Of This World");
		  setup();
	      vm.restartAt(0x3E80); // demo starts at 0x3E81

	     while (!_stub->_pi.quit) 
		 {
		   vm.setupScripts();
		   vm.inp_updatePlayer();
		    processInput();
		   vm.runScripts();
	     }

	     finish();
	     
	   }
   }


   Virtual Machine:
   ================

	   Seems the threading model is collaborative multi-tasking (as opposed to preemptive multitasking):
	   A thread (called a Channel on Eric Chahi website) will release the hand to the next one via the
	   break opcode.

	   It seems that even when a setvec is requested by a thread, we cannot set the instruction pointer
	   yet. The thread is allowed to keep on executing its code for the remaining of the vm frame.

       A virtual machine frame has a variable duration. The unit of time is 20ms and the frame can be set
	   to live for 1 (20ms ; 50Hz) up to 5 (100ms ; 10Hz).


	   30 something opcode. The graphic opcode are more complex, not only the declare the operation to perform
	   they also define where to find the vertices (segVideo1 or segVideo2).

	   No stack available but a thread can save its pc (Program Counter) once: One method call and return is possible.


   Video :
   =======
	   Double buffer architecture. AW optcodes even has a special instruction for blitting from one
	   frame buffer to an other.

	   Double buffering is implemented in software

	   According to Eric Chahi's webpage there are 4 framebuffer. Since on full screenbuffer is 320x200/2 = 32kB
	   that would mean the total size consumed is 128KB ?

   Sound :
   =======
	   Mixing is done on software.

	   Since the virtal machine and SDL are running simultaneously in two different threads:
	   Any read or write to an elements of the sound channels MUST be synchronized with a 
	   mutex.

   FastMode :
   ==========

   The game engine features a "fast-mode"...what it to be able to respond to the now defunct
   TURBO button commonly found on 386/486 era PC ?!


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
   A: See vm frame time: The vm frame duration is variable. The vm actually write for how long a video frame
      should be displayed in variable 0xFF. The value is the number of 20ms slice

   Q: Why is a palette 2048 bytes if there are only 16 colors ? I would have expected 48 bytes...
   A: ???

   Q: Why does Resource::load() search for ressource to load from higher to lower....since it will load stuff
      until no more ressources are marked as "Need to be loaded".
   A: ???

   Orignial DOS version :
   ======================

   Banks: 1,236,519 B
   exe  :    20,293 B


   Total bank      size: 1236519 (100%)
   ---------------------------------
   Total RT_SOUND    size: 585052  ( 47%)
   Total RT_MUSIC    size:   3540  (  0%)
   Total RT_POLY_ANIM   size: 106676  (  9%)
   Total RT_PALETTE      size:  11032  (  1%)
   Total RT_BYTECODE size: 135948  ( 11%)
   Total RT_POLY_CINEMATIC     size: 291008  ( 24%)

   As usual sounds are the most consuming assets (Quake1,Quake2 etc.....)


   memlist.bin features 146 entries :
   ==================================

   Most important part in an entry are:

   bankId          : - Give the file were the resource is.
   offset          : - How much to skip in the file before hiting the resource.
   size,packetSize : - How much to read, should we unpack what we read.



   Polygons drawing :
   =================

   Polygons can be given as:
    - a pure screenspace sequence of points: I call those screenspace polygons.
	- a list of delta to add or substract to the first vertex. I call those: objectspace polygons.

   Video : 
   =======

   Q: Why 4 framebuffer ?
   A: It seems the background is generated once (like in the introduction) and stored in a framebuffer.
      Everyframe the saved background is copied and new elements are drawn on top.


   Trivia :
   ========

   If you are used to RGBA 32bits per pixel framebuffer you are in for a shock:
   Another world is 16 colors palette based, making it 4bits per pixel !!

   Video generation :
   ==================

   Thank god the engine sets the palette before starting to drawing instead of after bliting.
   I would have been unable to generate screenshots otherwise.

   Memor managment :
   =================

   There is 0 malloc during the game. All resources are loaded in one big buffer (Resource::load).
   The entire buffer is "freed" at the end of a game part.


   The renderer is actually capable of Blending a new poly in the framebuffer (Video::drawLineT)


   	// I am almost sure that:
	// _curPagePtr1 is the backbuffer 
	// _curPagePtr2 is the frontbuffer
	// _curPagePtr3 is the background builder.


   RETARTED :
   ==========

   * Why does memlist.bin uses a special state field 0xFF in order to mark the end of resources ??!
     It would have been so much easier to write the number of resources at the beginning of the code.

   TODO :
   ======

       - Remove protection.
	   - Add OpenGL support.
	   - Add screenshot capability.
	   - Try to run with "Another World" asset instead of "Out Of This World" assets.
	   - Find where bitmap background are used...
*/