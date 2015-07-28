/*
**
** Memory
**
** Richard Buckley July 28, 2015
**
*/

#include <Device.h>

namespace ca {
	namespace pdp8 {
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
		
		MemoryCell::operator = (int v) {
			return (int)(m = v & 07777);
		}
		
		Memory::Memory() :
			Device("MEM", "Core Memory"),
			memorySize(MAXMEMSIZE)
		{
		}
		
		Memory::~Memory() {
			
		}
		
		MemoryCell & Memory::operator [] (int ma) {
			if (ma > MAXMEMSIZE) {
				flagStack.push( new pair < MemoryFlag, int >(MemFlagMaxSize, ma));
				return errCell;
			}
			
			MemoryFlag	flag = MemFlagClear;
			
			if (flags[ma] & MemFlagBreak) {
				flag |= flags[ma] & MemFlagBreak;
			}
			
			if (flags[ma] & MemFlagInitialized == MemFlagClear) {
				flag |= MemFlagInitialized;
			}
			
			if (ma > memorySize) {
				flag |= MemFlagRange;
			}
			
			if (flag !=  MemFlagClear) {
				flagStack.push( pair < MemoryFlag, int >(flag, ma));
			}
			
			return m[ma];
		}
			
	} // pdp8
} // ca