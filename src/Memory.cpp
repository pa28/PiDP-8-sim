/*
 * Memory.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

#include "Memory.h"

namespace ca
{
    namespace pdp8
    {

		Memory *	Memory::_instance = NULL;
        MemoryCell Memory::errCell(-1);
        MemoryCell Memory::m[MAXMEMSIZE];
        MemoryFlag Memory::flags[MAXMEMSIZE] = { MemFlagClear };

        MemoryCell::MemoryCell(int v) :
            m(v)
        {
        }

        MemoryCell::~MemoryCell() {
        }

        MemoryCell::operator int () {
            return (int)(m);
        }

        int MemoryCell::operator = (int v) {
            return (int)(m = v & 07777);
        }

        Memory::Memory() :
            Device("MEM", "Core Memory"),
            memorySize(MAXMEMSIZE),
			exceptionOn(true)
        {
        }

        Memory::~Memory() {

        }

		Memory * Memory::instance() {
			if (_instance == NULL) {
				_instance = new Memory();
			}
			
			return _instance;
		}
		
        MemoryCell & Memory::operator [] (int ma) throw(MemoryException) {
            if (ma > MAXMEMSIZE) {
                flagStack.push( std::pair < MemoryFlag, int >(MemFlagMaxSize, ma));
				if (exceptionOn) {
					throw MemoryException(MemoryExcpetionMaxSize);
				}
                return errCell;
            }

            uint32_t  flag = MemFlagClear;

            if (flags[ma] & MemFlagBreak) {
                flag |= flags[ma] & MemFlagBreak;
            }

            if (flags[ma] & (MemFlagInitialized == MemFlagClear)) {
                flag |= MemFlagInitialized;
            }

            if (ma > memorySize) {
                flag |= MemFlagRange;
            }

            if (flag !=  MemFlagClear) {
                flagStack.push( std::pair < uint32_t, int >(flag, ma));
            }

            return m[ma];
        }
    } /* namespace pdp8 */
} /* namespace ca */
