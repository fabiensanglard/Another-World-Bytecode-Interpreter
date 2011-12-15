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

#include "mixer.h"
#include "serializer.h"
#include "systemstub.h"


static int8 addclamp(int a, int b) {
	int add = a + b;
	if (add < -128) {
		add = -128;
	}
	else if (add > 127) {
		add = 127;
	}
	return (int8)add;
}

Mixer::Mixer(SystemStub *stub) 
	: _stub(stub) {
}

void Mixer::init() {
	memset(_channels, 0, sizeof(_channels));
	_mutex = _stub->createMutex();
	_stub->startAudio(Mixer::mixCallback, this);
}

void Mixer::free() {
	stopAll();
	_stub->stopAudio();
	_stub->destroyMutex(_mutex);
}

void Mixer::playChannel(uint8 channel, const MixerChunk *mc, uint16 freq, uint8 volume) {
	debug(DBG_SND, "Mixer::playChannel(%d, %d, %d)", channel, freq, volume);
	assert(channel < NUM_CHANNELS);
	MutexStack(_stub, _mutex);
	MixerChannel *ch = &_channels[channel];
	ch->active = true;
	ch->volume = volume;
	ch->chunk = *mc;
	ch->chunkPos = 0;
	ch->chunkInc = (freq << 8) / _stub->getOutputSampleRate();
}

void Mixer::stopChannel(uint8 channel) {
	debug(DBG_SND, "Mixer::stopChannel(%d)", channel);
	assert(channel < NUM_CHANNELS);
	MutexStack(_stub, _mutex);	
	_channels[channel].active = false;
}

void Mixer::setChannelVolume(uint8 channel, uint8 volume) {
	debug(DBG_SND, "Mixer::setChannelVolume(%d, %d)", channel, volume);
	assert(channel < NUM_CHANNELS);
	MutexStack(_stub, _mutex);
	_channels[channel].volume = volume;
}

void Mixer::stopAll() {
	debug(DBG_SND, "Mixer::stopAll()");
	MutexStack(_stub, _mutex);
	for (uint8 i = 0; i < NUM_CHANNELS; ++i) {
		_channels[i].active = false;		
	}
}

void Mixer::mix(int8 *buf, int len) {
	MutexStack(_stub, _mutex);
	memset(buf, 0, len);
	for (uint8 i = 0; i < NUM_CHANNELS; ++i) {
		MixerChannel *ch = &_channels[i];
		if (ch->active) {
			int8 *pBuf = buf;
			for (int j = 0; j < len; ++j, ++pBuf) {
				uint16 p1, p2;
				uint16 ilc = (ch->chunkPos & 0xFF);
				p1 = ch->chunkPos >> 8;
				ch->chunkPos += ch->chunkInc;
				if (ch->chunk.loopLen != 0) {
					if (p1 == ch->chunk.loopPos + ch->chunk.loopLen - 1) {
						debug(DBG_SND, "Looping sample on channel %d", i);
						ch->chunkPos = p2 = ch->chunk.loopPos;
					} else {
						p2 = p1 + 1;
					}
				} else {
					if (p1 == ch->chunk.len - 1) {
						debug(DBG_SND, "Stopping sample on channel %d", i);
						ch->active = false;
						break;
					} else {
						p2 = p1 + 1;
					}
				}
				// interpolate
				int8 b1 = *(int8 *)(ch->chunk.data + p1);
				int8 b2 = *(int8 *)(ch->chunk.data + p2);
				int8 b = (int8)((b1 * (0xFF - ilc) + b2 * ilc) >> 8);
				// set volume and clamp
				*pBuf = addclamp(*pBuf, (int)b * ch->volume / 0x40);
			}
		}
	}
}

void Mixer::mixCallback(void *param, uint8 *buf, int len) {
	((Mixer *)param)->mix((int8 *)buf, len);
}

void Mixer::saveOrLoad(Serializer &ser) {
	_stub->lockMutex(_mutex);
	for (int i = 0; i < NUM_CHANNELS; ++i) {
		MixerChannel *ch = &_channels[i];
		Serializer::Entry entries[] = {
			SE_INT(&ch->active, Serializer::SES_BOOL, VER(2)),
			SE_INT(&ch->volume, Serializer::SES_INT8, VER(2)),
			SE_INT(&ch->chunkPos, Serializer::SES_INT32, VER(2)),
			SE_INT(&ch->chunkInc, Serializer::SES_INT32, VER(2)),
			SE_PTR(&ch->chunk.data, VER(2)),
			SE_INT(&ch->chunk.len, Serializer::SES_INT16, VER(2)),
			SE_INT(&ch->chunk.loopPos, Serializer::SES_INT16, VER(2)),
			SE_INT(&ch->chunk.loopLen, Serializer::SES_INT16, VER(2)),
			SE_END()
		};
		ser.saveOrLoadEntries(entries);
	}
	_stub->unlockMutex(_mutex);
};
