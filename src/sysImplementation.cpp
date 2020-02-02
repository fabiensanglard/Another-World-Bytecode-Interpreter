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

#include <SDL.h>
#include "sys.h"
#include "util.h"


struct SDLStub : System {
	typedef void (SDLStub::*ScaleProc)(uint16_t *dst, uint16_t dstPitch, const uint16_t *src, uint16_t srcPitch, uint16_t w, uint16_t h);

	enum {
		SCREEN_W = 320,
		SCREEN_H = 200,
		SOUND_SAMPLE_RATE = 22050
	};

	int DEFAULT_SCALE = 3;

	SDL_Surface *_screen = nullptr;
	SDL_Window * _window = nullptr;
	SDL_Renderer * _renderer = nullptr;
	uint8_t _scale = DEFAULT_SCALE;

	virtual ~SDLStub() {}
	virtual void init(const char *title);
	virtual void destroy();
	virtual void setPalette(const uint8_t *buf);
	virtual void updateDisplay(const uint8_t *src);
	virtual void processEvents();
	virtual void sleep(uint32_t duration);
	virtual uint32_t getTimeStamp();
	virtual void startAudio(AudioCallback callback, void *param);
	virtual void stopAudio();
	virtual uint32_t getOutputSampleRate();
	virtual int addTimer(uint32_t delay, TimerCallback callback, void *param);
	virtual void removeTimer(int timerId);
	virtual void *createMutex();
	virtual void destroyMutex(void *mutex);
	virtual void lockMutex(void *mutex);
	virtual void unlockMutex(void *mutex);

	void prepareGfxMode();
	void cleanupGfxMode();
	void switchGfxMode();
};

void SDLStub::init(const char *title) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
//	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_ShowCursor(SDL_DISABLE);

	SDL_ShowCursor( SDL_ENABLE );
	SDL_CaptureMouse(SDL_TRUE);

	memset(&input, 0, sizeof(input));
  _scale = DEFAULT_SCALE;
	prepareGfxMode();
}

void SDLStub::destroy() {
	cleanupGfxMode();
	SDL_Quit();
}

static SDL_Color palette[NUM_COLORS];
void SDLStub::setPalette(const uint8_t *p) {
  // The incoming palette is in 565 format.
  for (int i = 0; i < NUM_COLORS; ++i)
  {
    uint8_t c1 = *(p + 0);
    uint8_t c2 = *(p + 1);
    palette[i].r = (((c1 & 0x0F) << 2) | ((c1 & 0x0F) >> 2)) << 2; // r
    palette[i].g = (((c2 & 0xF0) >> 2) | ((c2 & 0xF0) >> 6)) << 2; // g
    palette[i].b = (((c2 & 0x0F) >> 2) | ((c2 & 0x0F) << 2)) << 2; // b
    palette[i].a = 0xFF;
    p += 2;
  }
  SDL_SetPaletteColors(_screen->format->palette, palette, 0, NUM_COLORS);
}

void SDLStub::prepareGfxMode() {
  int w = SCREEN_W;
  int h = SCREEN_H;

  _window = SDL_CreateWindow("Another World", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w * _scale, h * _scale, SDL_WINDOW_SHOWN);
  _renderer = SDL_CreateRenderer(_window, -1, 0);
  _screen = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 8, 0, 0, 0, 0);
  if (!_screen) {
    error("SDLStub::prepareGfxMode() unable to allocate _screen buffer");
  }
  // Upon resize during gameplay, the screen surface is re-created and a new palette is allocated.
  // This will result in an all-white surface palette displaying a window full of white until a
  // a palette is set by the VM.
  // To avoid this issue, we save the last palette locally and re-upload it each time. On game start-up this
  // is not requested.
  SDL_SetPaletteColors(_screen->format->palette, palette, 0, NUM_COLORS);
}

void SDLStub::updateDisplay(const uint8_t *src) {
  uint16_t height = SCREEN_H;
	uint8_t* p = (uint8_t*)_screen->pixels;

	//For each line
	while (height--) {
		//One byte gives us two pixels, we only need to iterate w/2 times.
		for (int i = 0; i < SCREEN_W / 2; ++i) {
			//Extract two palette indices from upper byte and lower byte.
			p[i * 2 + 0] = *(src + i) >> 4;
			p[i * 2 + 1] = *(src + i) & 0xF;
		}
		p += _screen->pitch;
    src += SCREEN_W/2;
	}

  SDL_Texture* texture = SDL_CreateTextureFromSurface(_renderer, _screen);
  SDL_RenderCopy(_renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(_renderer);
  SDL_DestroyTexture(texture);

}

void SDLStub::processEvents() {
	SDL_Event ev;
	while(SDL_PollEvent(&ev)) {
		switch (ev.type) {
		case SDL_QUIT:
			input.quit = true;
			break;
		case SDL_KEYUP:
			switch(ev.key.keysym.sym) {
			case SDLK_LEFT:
				input.dirMask &= ~PlayerInput::DIR_LEFT;
				break;
			case SDLK_RIGHT:
				input.dirMask &= ~PlayerInput::DIR_RIGHT;
				break;
			case SDLK_UP:
				input.dirMask &= ~PlayerInput::DIR_UP;
				break;
			case SDLK_DOWN:
				input.dirMask &= ~PlayerInput::DIR_DOWN;
				break;
			case SDLK_SPACE:
			case SDLK_RETURN:
				input.button = false;
				break;
			case SDLK_ESCAPE:
        input.quit = true;
				break;
			}
			break;
		case SDL_KEYDOWN:
			if (ev.key.keysym.mod & KMOD_CTRL) {
        if (ev.key.keysym.sym == SDLK_x) {
          input.quit = true;
        } else if (ev.key.keysym.sym == SDLK_s) {
					input.save = true;
				} else if (ev.key.keysym.sym == SDLK_l) {
					input.load = true;
				} else if (ev.key.keysym.sym == SDLK_KP_PLUS) {
					input.stateSlot = 1;
				} else if (ev.key.keysym.sym == SDLK_KP_MINUS) {
					input.stateSlot = -1;
				}
        break;
			}
			input.lastChar = ev.key.keysym.sym;
			switch(ev.key.keysym.sym) {
			case SDLK_LEFT:
				input.dirMask |= PlayerInput::DIR_LEFT;
				break;
			case SDLK_RIGHT:
				input.dirMask |= PlayerInput::DIR_RIGHT;
				break;
			case SDLK_UP:
				input.dirMask |= PlayerInput::DIR_UP;
				break;
			case SDLK_DOWN:
				input.dirMask |= PlayerInput::DIR_DOWN;
				break;
			case SDLK_SPACE:
			case SDLK_RETURN:
				input.button = true;
				break;
			case SDLK_c:
				input.code = true;
				break;
			case SDLK_p:
				input.pause = true;
				break;
			  case SDLK_TAB :
          _scale = _scale + 1;
        if (_scale > 4) { _scale = 1; }
        switchGfxMode();
        break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
}

void SDLStub::sleep(uint32_t duration) {
	SDL_Delay(duration);
}

uint32_t SDLStub::getTimeStamp() {
	return SDL_GetTicks();	
}

void SDLStub::startAudio(AudioCallback callback, void *param) {
	SDL_AudioSpec desired;
	memset(&desired, 0, sizeof(desired));

	desired.freq = SOUND_SAMPLE_RATE;
	desired.format = AUDIO_U8;
	desired.channels = 1;
	desired.samples = 2048;
	desired.callback = callback;
	desired.userdata = param;
	if (SDL_OpenAudio(&desired, NULL) == 0) {
		SDL_PauseAudio(0);
	} else {
		error("SDLStub::startAudio() unable to open sound device");
	}
}

void SDLStub::stopAudio() {
	SDL_CloseAudio();
}

uint32_t SDLStub::getOutputSampleRate() {
	return SOUND_SAMPLE_RATE;
}

int SDLStub::addTimer(uint32_t delay, TimerCallback callback, void *param) {
	return SDL_AddTimer(delay, (SDL_TimerCallback)callback, param);
}

void SDLStub::removeTimer(int timerId) {
	SDL_RemoveTimer(timerId);
}

void *SDLStub::createMutex() {
	return SDL_CreateMutex();
}

void SDLStub::destroyMutex(void *mutex) {
	SDL_DestroyMutex((SDL_mutex *)mutex);
}

void SDLStub::lockMutex(void *mutex) {
	SDL_mutexP((SDL_mutex *)mutex);
}

void SDLStub::unlockMutex(void *mutex) {
	SDL_mutexV((SDL_mutex *)mutex);
}



void SDLStub::cleanupGfxMode() {
	if (_screen) {
		SDL_FreeSurface(_screen);
    _screen = 0;
	}

	if (_window) {
	  SDL_DestroyWindow(_window);
	  _window = nullptr;
	}

	if (_screen) {
	  SDL_FreeSurface(_screen);
	  _screen = nullptr;
	}
}

void SDLStub::switchGfxMode() {
  cleanupGfxMode();
	prepareGfxMode();
}

SDLStub sysImplementation;
System *stub = &sysImplementation;

