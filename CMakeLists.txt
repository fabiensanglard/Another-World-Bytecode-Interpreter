cmake_minimum_required(VERSION 3.13)
project(raw)

set(CMAKE_CXX_STANDARD 11)

add_definitions(-DAUTO_DETECT_PLATFORM -DBYPASS_PROTECTION)
set(CMAKE_CXX_FLAGS " -Os -g -fno-rtti -fno-exceptions -Wall -Wno-unknown-pragmas -Wshadow -Wundef -Wwrite-strings -Wnon-virtual-dtor -Wno-multichar")

add_executable(raw
        src/bank.cpp
        src/engine.cpp
        src/file.cpp
        src/main.cpp
        src/mixer.cpp
        src/parts.cpp
        src/resource.cpp
        src/serializer.cpp
        src/sfxplayer.cpp
        src/staticres.cpp
        src/sysImplementation.cpp
        src/util.cpp
        src/video.cpp
        src/vm.cpp
)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(raw ${SDL2_LIBRARIES})
target_link_libraries(raw z)

