/*
 * Memory.h
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include <stdio.h>
#include <stdint.h>
#include <stack>
#include <utility>
#include <exception>
#include "Device.h"
#include "CPU.h"

namespace ca
{
    namespace pdp8
    {

        enum MemoryConstants {
            MAXMEMSIZE = 8 * 4096,
        };

		enum MemoryExceptionCode {
			MemoryExcpetionMaxSize = 0
		};
		class MemoryException : public std::exception {
		public:
			MemoryException(MemoryExceptionCode c) : code(c) {}
			virtual ~MemoryException() throw() {}
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

            //operator int();
            //operator uint16_t();
            operator int32_t() { return (int32_t)(mb = m); }
            //operator unsigned int() { return (unsigned int) operator int(); }

            int operator = (int v);

            bool operator == (MemoryCell & other) { return m == other.m; }

            operator bool() { return m != -1; }

        protected:
            uint16_t    m;
            static uint16_t     mb;

            void    clear() { m = 0; }
        };

        class Memory: public Device
        {
            Memory();

		public:
            virtual ~Memory();

			static Memory * instance();

			virtual void initialize() {}
			virtual void reset() {}
			virtual void stop() {}

            //MemoryCell & operator [] (int ma) throw(MemoryException);
            MemoryCell & operator [] (int32_t ma) throw(MemoryException);

            bool flagSet() { return ! flagStack.empty(); }
            int flagStackSize() { return flagStack.size(); }
            void clearFlackStack() { while (!flagStack.empty()) flagStack.pop(); }

            static uint16_t     MB() { return MemoryCell::mb; }
            static uint16_t     MA() { return ma; }

            int     loadFile(const char *fileName);
            int     saveFile(const char *fileName, int32_t start, int32_t end);
			void	initMemory(const int32_t *mem, int32_t start, int32_t end, int32_t startAddr, bool run = false);

        protected:

			static Memory *		_instance;
            static MemoryFlag   flags[];
            static MemoryCell   errCell, m[];
            static uint16_t     ma;

            int     memorySize;
            std::stack < std::pair < uint32_t, int > >  flagStack;
			bool	exceptionOn;

			int     sim_load_bin( FILE * fp );
			int32_t    sim_bin_getc (FILE *fi, uint32_t *newf);

			void    clearMemory();

			uint32_t	lastOrigin;
			uint32_t	lastField;
        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* MEMORY_H_ */
/* vim: set ts=4 sw=4  noet autoindent : */
