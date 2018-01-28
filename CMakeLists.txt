cmake_minimum_required(VERSION 3.9)
project(PiDP)

set(CMAKE_CXX_STANDARD 17)

add_executable(PiDP PDP8.cpp PDP8.h Registers.h Device.cpp Device.h Memory.h Chassis.cpp Chassis.h
        FileUtilities.h CPU.h CPU.cpp)
add_executable(Test main.cpp Registers.h Memory.h FileUtilities.h)
