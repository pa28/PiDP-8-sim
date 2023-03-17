cmake_minimum_required(VERSION 3.22)
project(PDP8)
enable_testing()

include_directories(fmt/include src tests ~/opt/include)
link_directories(/usr/local/lib)

list(APPEND LIB_FMT fmt/src/format.cc fmt/src/os.cc)
file(GLOB SOURCE src/*.cpp src/assembler/*.cpp)
file(GLOB TEST_SRC tests/libs/*.cpp)

set(CMAKE_CXX_STANDARD 20)

add_executable(PDP8 main.cpp src/Terminal.cpp ${SOURCE} ${LIB_FMT})

add_executable(ct_OprInst tests/ct_OprInst.cpp ${TEST_SRC} ${SOURCE} ${LIB_FMT})
target_link_libraries(ct_OprInst PRIVATE cleantest-main-shared cleantest-shared)
add_test(NAME OprInst COMMAND ct_OprInst)

add_executable(asm8 asm8.cpp ${SOURCE} ${LIB_FMT})
