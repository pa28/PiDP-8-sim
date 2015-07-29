/*
 * Memory.h
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include <stdint.h>
#include <stack>
#include <utility>
#include <exception>
#include "Device.h"

namespace ca
{
    namespace pdp8
    {

        enum MemoryConstants {
            MAXMEMSIZE = 8 * 4096,
        };
		
		enum MemoryExceptionCode {
			MemoryExcpetionMaxSize = 0
		}
		class MemoryException : public exception {
		public:
			MemoryException(MemoryExceptionCode c) : code(c) {}
			virtual ~MemoryException() {}
			virtual const char* what() const throw() {
				switch (code) {
					case MemoryExcpetionMaxSize:
						return "Memory request beyond maximum possable memory size.";
					default:
						return "Unknown memory error.";
				}
			}
		protected:
			MemoryExceptionCode	code;
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
            friend class Memory;

            MemoryCell(int v = 0);

        public:
            virtual ~MemoryCell();

            operator int();
            int operator = (int v);

            bool operator == (MemoryCell & other) { return m == other.m; }

            operator bool() { return m != -1; }

        protected:
            uint16_t    m;

        };

        class Memory: public Device
        {
            Memory();

		public:
            virtual ~Memory();
			
			static Memory * instance();

            MemoryCell & operator [] (int ma) throw(MemoryException);

            bool flagSet() { return ! flagStack.empty(); }
            int flagStackSize() { return flagStack.size(); }
            void clearFlackStack() { while (!flagStack.empty()) flagStack.pop(); }

        protected:

			static Memory *		_instance;
            static MemoryFlag   flags[];
            static MemoryCell   errCell, m[];

            int     memorySize;
            std::stack < std::pair < uint32_t, int > >  flagStack;
			bool	exceptionOn;

        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* MEMORY_H_ */
