cmake_minimum_required(VERSION 3.21)
project(PiDP8 VERSION 0.0.1.0)

set(CMAKE_CXX_STANDARD 20)

list(APPEND PiDP8_SRC src/CoreMemory.cpp src/hardware.cpp src/cpu.cpp src/Terminal.cpp assembler/Assembler.cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(PiDP8 main.cpp assembler/Assembler.cpp ${PiDP8_SRC} src/Pdp8Terminal.cpp)
target_link_libraries(PiDP8 fmt)

add_executable(CPU_TEST_0 tests/CPU_Tests_0.cpp ${PiDP8_SRC} src/Terminal.cpp src/Terminal.h src/NullStream.cpp src/NullStream.h)
target_link_libraries(CPU_TEST_0 fmt)