
SDL_CFLAGS = `sdl-config --cflags`
SDL_LIBS = `sdl-config --libs`

#comment this line and uncomment the one below it to force detection
DEFINES:= -DAUTO_DETECT_PLATFORM
#DEFINES = -DSYS_LITTLE_ENDIAN

CXX = g++
CXXFLAGS:= -Os -g -std=gnu++98 -fno-rtti -fno-exceptions -Wall -Wno-unknown-pragmas -Wshadow
CXXFLAGS+= -Wundef -Wwrite-strings -Wnon-virtual-dtor -Wno-multichar
CXXFLAGS+= $(SDL_CFLAGS) $(DEFINES)

SRCS = bank.cpp file.cpp engine.cpp mixer.cpp resource.cpp parts.cpp vm.cpp \
	serializer.cpp sfxplayer.cpp staticres.cpp util.cpp video.cpp main.cpp sysImplementation.cpp

# logic.cpp sdlstub.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

game: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(SDL_LIBS) -lz

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $*.o

clean:
	rm -f *.o *.d

-include $(DEPS)
