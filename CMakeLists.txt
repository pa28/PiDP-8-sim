cmake_minimum_required(VERSION 3.9)
project(PiDP)

find_package(Threads REQUIRED)

find_package(Curses REQUIRED)

set(CMAKE_CXX_STANDARD 17)

add_executable(PiDP PDP8.cpp PDP8.h Registers.h Device.cpp Device.h Memory.h Chassis.cpp Chassis.h
        FileUtilities.h CPU.h CPU.cpp Thread.h Thread.cpp Console.h Console.cpp Terminal.cpp Terminal.h
        VirtualPanel.h VirtualPanel.cpp FileUtilities.h SignalHandler.h Server.h)

target_link_libraries(PiDP ${CMAKE_THREAD_LIBS_INIT} ${CURSES_LIBRARIES})

add_executable(Test main.cpp Server.h Thread.cpp Thread.h ConsoleAPI.h)

target_link_libraries(Test ${CMAKE_THREAD_LIBS_INIT} ${CURSES_LIBRARIES})
