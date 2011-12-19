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

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include "intern.h"


#define NUM_COLORS 16
#define BYTE_PER_PIXEL 3

struct PlayerInput {
	enum {
		DIR_LEFT  = 1 << 0,
		DIR_RIGHT = 1 << 1,
		DIR_UP    = 1 << 2,
		DIR_DOWN  = 1 << 3
	};

	uint8 dirMask;
	bool button;
	bool code;
	bool pause;
	bool quit;
	char lastChar;
	bool save, load;
	bool fastMode;
	int8 stateSlot;
};

/*
	System is an abstract class so any find of system can be plugged underneath.
*/
struct System {
	typedef void (*AudioCallback)(void *param, uint8 *stream, int len);
	typedef uint32 (*TimerCallback)(uint32 delay, void *param);
	
	PlayerInput input;

	virtual ~System() {}

	virtual void init(const char *title) = 0;
	virtual void destroy() = 0;

	virtual void setPalette(uint8 s, uint8 n, const uint8 *buf) = 0;
	virtual void copyRect(uint16 x, uint16 y, uint16 w, uint16 h, const uint8 *buf, uint32 pitch) = 0;

	virtual void processEvents() = 0;
	virtual void sleep(uint32 duration) = 0;
	virtual uint32 getTimeStamp() = 0;

	virtual void startAudio(AudioCallback callback, void *param) = 0;
	virtual void stopAudio() = 0;
	virtual uint32 getOutputSampleRate() = 0;
	
	virtual void *addTimer(uint32 delay, TimerCallback callback, void *param) = 0;
	virtual void removeTimer(void *timerId) = 0;

	virtual void *createMutex() = 0;
	virtual void destroyMutex(void *mutex) = 0;
	virtual void lockMutex(void *mutex) = 0;
	virtual void unlockMutex(void *mutex) = 0;
};

struct MutexStack {
	System *sys;
	void *_mutex;

	MutexStack(System *stub, void *mutex) 
		: sys(stub), _mutex(mutex) {
		sys->lockMutex(_mutex);
	}
	~MutexStack() {
		sys->unlockMutex(_mutex);
	}
};



#endif
