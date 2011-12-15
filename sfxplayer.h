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

#ifndef __SFXPLAYER_H__
#define __SFXPLAYER_H__

#include "intern.h"

struct SfxInstrument {
	uint8 *data;
	uint16 volume;
};

struct SfxModule {
	const uint8 *data;
	uint16 curPos;
	uint8 curOrder;
	uint8 numOrder;
	uint8 orderTable[0x80];
	SfxInstrument samples[15];
};

struct SfxPattern {
	uint16 note_1;
	uint16 note_2;
	uint16 sampleStart;
	uint8 *sampleBuffer;
	uint16 sampleLen;
	uint16 loopPos;
	uint8 *loopData;
	uint16 loopLen;
	uint16 sampleVolume;
};

struct Mixer;
struct Resource;
struct Serializer;
struct SystemStub;

struct SfxPlayer {
	Mixer *_mix;
	Resource *_res;
	SystemStub *_stub;

	void *_mutex;
	void *_timerId;
	uint16 _delay;
	uint16 _resNum;
	SfxModule _sfxMod;
	int16 *_markVar;

	SfxPlayer(Mixer *mix, Resource *res, SystemStub *stub);
	void init();
	void free();

	void setEventsDelay(uint16 delay);
	void loadSfxModule(uint16 resNum, uint16 delay, uint8 pos);
	void prepareInstruments(const uint8 *p);
	void start();
	void stop();
	void handleEvents();
	void handlePattern(uint8 channel, const uint8 *patternData);

	static uint32 eventsCallback(uint32 interval, void *param);

	void saveOrLoad(Serializer &ser);
};

#endif
