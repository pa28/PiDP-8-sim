cmake_minimum_required(VERSION 3.9)
project(PiDP)

set(CMAKE_CXX_STANDARD 17)

add_executable(PiDP main.cpp Registers.h CPU.cpp CPU.h Thread.cpp Thread.h Device.cpp Device.h Memory.cpp Memory.h Chassis.cpp Chassis.h PDP8.h Console.cpp Console.h DK8EA.cpp DK8EA.h Panel.cpp Panel.h Terminal.cpp Terminal.h VirtualPanel.cpp VirtualPanel.h)