cmake_minimum_required(VERSION 3.13)
project(raw)

set(CMAKE_CXX_STANDARD 11)

add_definitions(-DAUTO_DETECT_PLATFORM -DBYPASS_PROTECTION)
set(CMAKE_CXX_FLAGS " -Os -g -fno-rtti -fno-exceptions -Wall -Wno-unknown-pragmas -Wshadow -Wundef -Wwrite-strings -Wnon-virtual-dtor -Wno-multichar")

add_executable(raw
        bank.cpp
        bank.h
        endian.h
        engine.cpp
        engine.h
        file.cpp
        file.h
        intern.h
        main.cpp
        mixer.cpp
        mixer.h
        parts.cpp
        parts.h
        resource.cpp
        resource.h
        serializer.cpp
        serializer.h
        sfxplayer.cpp
        sfxplayer.h
        staticres.cpp
        sys.h
        sysImplementation.cpp
        util.cpp
        util.h
        video.cpp
        video.h
        vm.cpp
        vm.h)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(raw ${SDL2_LIBRARIES})
target_link_libraries(raw z)

