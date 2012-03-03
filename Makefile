
SDL_CFLAGS = `sdl-config --cflags`
SDL_LIBS = `sdl-config --libs`

DEFINES = -DSYS_LITTLE_ENDIAN

CXX = g++
CXXFLAGS:= -Os -g -std=gnu++98 -fno-rtti -fno-exceptions -Wall -Wuninitialized -Wno-unknown-pragmas -Wshadow
CXXFLAGS+= -Wundef -Wreorder -Wwrite-strings -Wnon-virtual-dtor -Wno-multichar
CXXFLAGS+= $(SDL_CFLAGS) $(DEFINES)

SRCS = bank.cpp file.cpp engine.cpp mixer.cpp resource.cpp parts.cpp vm.cpp \
	serializer.cpp sfxplayer.cpp staticres.cpp util.cpp video.cpp main.cpp sysImplementation.cpp

# logic.cpp sdlstub.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

game: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(SDL_LIBS) -lz

.cpp.o:
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $*.o

clean:
	rm -f *.o *.d

-include $(DEPS)
