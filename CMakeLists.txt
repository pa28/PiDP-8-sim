cmake_minimum_required(VERSION 3.24)
project(PDP8)

include_directories(fmt/include src tests)

list(APPEND LIB_FMT fmt/src/format.cc fmt/src/os.cc)
file(GLOB SOURCE src/*.cpp src/assembler/*.cpp)
file(GLOB TEST_SRC tests/libs/*.cpp)

set(CMAKE_CXX_STANDARD 20)

add_executable(PDP8 main.cpp src/Terminal.cpp ${SOURCE} ${LIB_FMT})

add_executable(TestOprInst tests/TestOprInst.cpp ${TEST_SRC} ${SOURCE} ${LIB_FMT})

add_executable(asm8 asm8.cpp ${SOURCE} ${LIB_FMT})
