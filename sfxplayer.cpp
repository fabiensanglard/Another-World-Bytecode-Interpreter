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

#include "sfxplayer.h"
#include "mixer.h"
#include "resource.h"
#include "serializer.h"
#include "systemstub.h"


SfxPlayer::SfxPlayer(Mixer *mix, Resource *res, SystemStub *stub)
	: _mix(mix), _res(res), _stub(stub), _delay(0), _resNum(0) {
}

void SfxPlayer::init() {
	_mutex = _stub->createMutex();
}

void SfxPlayer::free() {
	stop();
	_stub->destroyMutex(_mutex);
}

void SfxPlayer::setEventsDelay(uint16 delay) {
	debug(DBG_SND, "SfxPlayer::setEventsDelay(%d)", delay);
	MutexStack(_stub, _mutex);
	_delay = delay * 60 / 7050;
}

void SfxPlayer::loadSfxModule(uint16 resNum, uint16 delay, uint8 pos) {
	debug(DBG_SND, "SfxPlayer::loadSfxModule(0x%X, %d, %d)", resNum, delay, pos);
	MutexStack(_stub, _mutex);
	MemEntry *me = &_res->_memList[resNum];
	if (me->valid == 1 && me->type == 1) {
		_resNum = resNum;
		memset(&_sfxMod, 0, sizeof(SfxModule));
		_sfxMod.curOrder = pos;
		_sfxMod.numOrder = READ_BE_UINT16(me->bufPtr + 0x3E);
		debug(DBG_SND, "SfxPlayer::loadSfxModule() curOrder = 0x%X numOrder = 0x%X", _sfxMod.curOrder, _sfxMod.numOrder);
		for (int i = 0; i < 0x80; ++i) {
			_sfxMod.orderTable[i] = *(me->bufPtr + 0x40 + i);
		}
		if (delay == 0) {
			_delay = READ_BE_UINT16(me->bufPtr);
		} else {
			_delay = delay;
		}
		_delay = _delay * 60 / 7050;
		_sfxMod.data = me->bufPtr + 0xC0;
		debug(DBG_SND, "SfxPlayer::loadSfxModule() eventDelay = %d ms", _delay);
		prepareInstruments(me->bufPtr + 2);
	} else {
		warning("SfxPlayer::loadSfxModule() ec=0x%X", 0xF8);
	}
}

void SfxPlayer::prepareInstruments(const uint8 *p) {
	memset(_sfxMod.samples, 0, sizeof(_sfxMod.samples));
	for (int i = 0; i < 15; ++i) {
		SfxInstrument *ins = &_sfxMod.samples[i];
		uint16 resNum = READ_BE_UINT16(p); p += 2;
		if (resNum != 0) {
			ins->volume = READ_BE_UINT16(p);
			MemEntry *me = &_res->_memList[resNum];
			if (me->valid == 1 && me->type == 0) {
				ins->data = me->bufPtr;
				memset(ins->data + 8, 0, 4);
				debug(DBG_SND, "Loaded instrument 0x%X n=%d volume=%d", resNum, i, ins->volume);
			} else {
				error("Error loading instrument 0x%X", resNum);
			}
		}
		p += 2; // skip volume
	}
}

void SfxPlayer::start() {
	debug(DBG_SND, "SfxPlayer::start()");
	MutexStack(_stub, _mutex);
	_sfxMod.curPos = 0;
	_timerId = _stub->addTimer(_delay, eventsCallback, this);			
}

void SfxPlayer::stop() {
	debug(DBG_SND, "SfxPlayer::stop()");
	MutexStack(_stub, _mutex);
	if (_resNum != 0) {
		_resNum = 0;
		_stub->removeTimer(_timerId);
	}
}

void SfxPlayer::handleEvents() {
	MutexStack(_stub, _mutex);
	uint8 order = _sfxMod.orderTable[_sfxMod.curOrder];
	const uint8 *patternData = _sfxMod.data + _sfxMod.curPos + order * 1024;
	for (uint8 ch = 0; ch < 4; ++ch) {
		handlePattern(ch, patternData);
		patternData += 4;
	}
	_sfxMod.curPos += 4 * 4;
	debug(DBG_SND, "SfxPlayer::handleEvents() order = 0x%X curPos = 0x%X", order, _sfxMod.curPos);
	if (_sfxMod.curPos >= 1024) {
		_sfxMod.curPos = 0;
		order = _sfxMod.curOrder + 1;
		if (order == _sfxMod.numOrder) {
			_resNum = 0;
			_stub->removeTimer(_timerId);
			_mix->stopAll();
		}
		_sfxMod.curOrder = order;
	}
}

void SfxPlayer::handlePattern(uint8 channel, const uint8 *data) {
	SfxPattern pat;
	memset(&pat, 0, sizeof(SfxPattern));
	pat.note_1 = READ_BE_UINT16(data + 0);
	pat.note_2 = READ_BE_UINT16(data + 2);
	if (pat.note_1 != 0xFFFD) {
		uint16 sample = (pat.note_2 & 0xF000) >> 12;
		if (sample != 0) {
			uint8 *ptr = _sfxMod.samples[sample - 1].data;
			if (ptr != 0) {
				debug(DBG_SND, "SfxPlayer::handlePattern() preparing sample %d", sample);
				pat.sampleVolume = _sfxMod.samples[sample - 1].volume;
				pat.sampleStart = 8;
				pat.sampleBuffer = ptr;
				pat.sampleLen = READ_BE_UINT16(ptr) * 2;
				uint16 loopLen = READ_BE_UINT16(ptr + 2) * 2;
				if (loopLen != 0) {
					pat.loopPos = pat.sampleLen;
					pat.loopData = ptr;
					pat.loopLen = loopLen;
				} else {
					pat.loopPos = 0;
					pat.loopData = 0;
					pat.loopLen = 0;
				}
				int16 m = pat.sampleVolume;
				uint8 effect = (pat.note_2 & 0x0F00) >> 8;
				if (effect == 5) { // volume up
					uint8 volume = (pat.note_2 & 0xFF);
					m += volume;
					if (m > 0x3F) {
						m = 0x3F;
					}
				} else if (effect == 6) { // volume down
					uint8 volume = (pat.note_2 & 0xFF);
					m -= volume;
					if (m < 0) {
						m = 0;
					}	
				}
				_mix->setChannelVolume(channel, m);
				pat.sampleVolume = m;
			}
		}
	}
	if (pat.note_1 == 0xFFFD) {
		debug(DBG_SND, "SfxPlayer::handlePattern() _scriptVars[0xF4] = 0x%X", pat.note_2);
		*_markVar = pat.note_2;
	} else if (pat.note_1 != 0) {
		if (pat.note_1 == 0xFFFE) {
			_mix->stopChannel(channel);
		} else if (pat.sampleBuffer != 0) {
			MixerChunk mc;
			memset(&mc, 0, sizeof(mc));
			mc.data = pat.sampleBuffer + pat.sampleStart;
			mc.len = pat.sampleLen;
			mc.loopPos = pat.loopPos;
			mc.loopLen = pat.loopLen;
			assert(pat.note_1 >= 0x37 && pat.note_1 < 0x1000);
			// convert amiga period value to hz
			uint16 freq = 7159092 / (pat.note_1 * 2);
			debug(DBG_SND, "SfxPlayer::handlePattern() adding sample freq = 0x%X", freq);
			_mix->playChannel(channel, &mc, freq, pat.sampleVolume);
		}
	}
}

uint32 SfxPlayer::eventsCallback(uint32 interval, void *param) {
	SfxPlayer *p = (SfxPlayer *)param;
	p->handleEvents();
	return p->_delay;
}

void SfxPlayer::saveOrLoad(Serializer &ser) {
	_stub->lockMutex(_mutex);
	Serializer::Entry entries[] = {
		SE_INT(&_delay, Serializer::SES_INT8, VER(2)),
		SE_INT(&_resNum, Serializer::SES_INT16, VER(2)),
		SE_INT(&_sfxMod.curPos, Serializer::SES_INT16, VER(2)),
		SE_INT(&_sfxMod.curOrder, Serializer::SES_INT8, VER(2)),
		SE_END()
	};
	ser.saveOrLoadEntries(entries);
	_stub->unlockMutex(_mutex);
	if (ser._mode == Serializer::SM_LOAD && _resNum != 0) {
		uint16 delay = _delay;
		loadSfxModule(_resNum, 0, _sfxMod.curOrder);
		_delay = delay;
		_timerId = _stub->addTimer(_delay, eventsCallback, this);
	}
}
