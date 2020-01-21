cmake_minimum_required(VERSION 3.15)
project(AW)

set(CMAKE_CXX_STANDARD 14)

add_definitions(-DAUTO_DETECT_PLATFORM)

add_executable(AW
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
target_link_libraries(AW ${SDL2_LIBRARIES})
target_link_libraries(AW z)

