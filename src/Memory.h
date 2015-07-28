/*
**
** Memory
**
** Richard Buckley July 28, 2015
**
*/

#include <stdint>
#include <stack>
#include <utility>
#include <Device.h>

namespace ca {
	namespace pdp8 {
		
		enum MemoryConstants {
			MAXMEMSIZE = 8 * 4096,
		};
		
		enum MemoryFlag {
			MemFlagClear = 0,
			MemFlagInitialized = 1,
			MemFlagBreakExecute = 2,
			MemFlagBreakRead = 4,
			MemFlagBreakWrite = 8,
			MemFlagBreak = MemFlagBreakExecute | MemFlagBreakRead | MemFlagBreakWrite,
			MemFlagRange = 16,
			MemFlagMaxSize = 32,
		};
		
		class MemoryCell {
			MemoryCell(int v = 0);
			
		public:
			virtual ~MemoryCell();
			
			operator int();
			int operator = (int v);
			
			bool operator == (MemoryCell & other) { return m == other.m; }
			
			operator bool() { return m != -1; }
			
		protected:
			uint16_t	m;
			
		};
		
		class Memory : public Device {
		public:
			Memory();
			virtual ~Memory();
			
			MemoryCell & operator [] (int ma);
			
			bool flags() { return ! flagStack.empty(); }
			int	flagStackSize() { return flagStack.size(); }
			void clearFlackStack() { while (!flagStack.empty()) flagStack.pop(); }
		
		protected:
		
			static MemoryFlag	flags[];
			static MemoryCell	errCell, m[];
			
			int		memorySize;
			stack < pair < MemoryFlag, int > >	flagStack;
			
		};
	} // pdp8
} // ca