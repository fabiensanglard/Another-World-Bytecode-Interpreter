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
   Great fan site here: https://sites.google.com/site/interlinkknight/anotherworld
   Contains the wheelcode :P ! 

   A lot of details can be found regarding the game and engine architecture at:
   http://www.anotherworld.fr/anotherworld_uk/another_world.htm

   The chronology of the game implementation can retraced via the ordering of the opcodes:
   The sound and music opcode are at the end: Music and sound was done at the end
*/