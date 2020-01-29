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

#ifndef __MIXER_H__
#define __MIXER_H__

#include "intern.h"

struct MixerChunk {
	const uint8_t *data;
	uint16_t len;
	uint16_t loopPos;
	uint16_t loopLen;
};

struct MixerChannel {
	uint8_t active;
	uint8_t volume;
	MixerChunk chunk;
	uint32_t chunkPos;
	uint32_t chunkInc;
};

struct Serializer;
struct System;

#define AUDIO_NUM_CHANNELS 4

struct Mixer {


	void *_mutex;
	System *sys;

	// Since the virtal machine and SDL are running simultaneously in two different threads
	// any read or write to an elements of the sound channels MUST be synchronized with a 
	// mutex.
	MixerChannel _channels[AUDIO_NUM_CHANNELS];

	Mixer(System *stub);
	void init();
	void free();

	void playChannel(uint8_t channel, const MixerChunk *mc, uint16_t freq, uint8_t volume);
	void stopChannel(uint8_t channel);
	void setChannelVolume(uint8_t channel, uint8_t volume);
	void stopAll();
	void mix(int8_t *buf, int len);

	static void mixCallback(void *param, uint8_t *buf, int len);

	void saveOrLoad(Serializer &ser);
};

#endif
