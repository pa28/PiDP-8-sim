/*
 * Memory.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
 *
   Portions of this program are based substantially on work by Robert M Supnik
   The license for Mr Supnik's work follows:

   Copyright (c) 1993-2013, Robert M Supnik

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Robert M Supnik shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Robert M Supnik.
 */

#include "Memory.h"
#include "Console.h"

namespace ca
{
    namespace pdp8
    {

		Memory *	Memory::_instance = NULL;
        MemoryCell Memory::errCell(-1);
        MemoryCell Memory::m[MAXMEMSIZE];
        MemoryFlag Memory::flags[MAXMEMSIZE] = { MemFlagClear };
        uint16_t    MemoryCell::mb = 0;
        uint16_t    Memory::ma = 0;

        MemoryCell::MemoryCell(int v) :
            m(v)
        {
        }

        MemoryCell::~MemoryCell() {
        }

        /*
        MemoryCell::operator int () {
            return (int)(mb = m);
        }

        MemoryCell::operator uint16_t () {
            return (uint16_t)(mb = m);
        }
        */

        int MemoryCell::operator = (int v) {
            return (int)(mb = m = v & 07777);
        }

        Memory::Memory() :
            Device("MEM", "Core Memory"),
            memorySize(MAXMEMSIZE),
			exceptionOn(true),
			lastOrigin(0),
			lastField(0)
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

		/*
		MemoryCell & Memory::operator [] (uint16_t _ma) throw(MemoryException) {
		    return operator [] ((int)_ma);
		}
		*/

        MemoryCell & Memory::operator [] (int32_t _ma) throw(MemoryException) {
            ma = _ma;

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

        int32_t Memory::sim_bin_getc (FILE *fi, uint32_t *newf)
        {
            int32_t c, rubout;

            rubout = 0;                                             /* clear toggle */
            while ((c = getc (fi)) != EOF) {                        /* read char */
                if (rubout)                                         /* toggle set? */
                    rubout = 0;                                     /* clr, skip */
                else if (c == 0377)                                 /* rubout? */
                    rubout = 1;                                     /* set, skip */
                else if (c > 0200)                                  /* channel 8 set? */
                    *newf = (c & 070) << 9;                         /* change field */
                else return c;                                      /* otherwise ok */
            }
            return EOF;
        }

        int Memory::sim_load_bin (FILE *fi)
        {
            int32_t hi, lo, wd, csum, t;
            uint32_t field, newf, origin;
            int32_t sections_read = 0;

            for (;;) {
                csum = origin = field = newf = 0;                   /* init */
                do {                                                /* skip leader */
                    if ((hi = sim_bin_getc (fi, &newf)) == EOF) {
                        if (sections_read != 0) {
                            Console::instance()->printf ("%d sections sucessfully read\n\r", sections_read);
                            return 0;
                        } else {
                            return 1;
                        }
                    }
                } while ((hi == 0) || (hi >= 0200));
                for (;;) {                                          /* data blocks */
                    if ((lo = sim_bin_getc (fi, &newf)) == EOF)     /* low char */
                        return 1;
                    wd = (hi << 6) | lo;                            /* form word */
                    t = hi;                                         /* save for csum */
                    if ((hi = sim_bin_getc (fi, &newf)) == EOF)     /* next char */
                        return 1;
                    if (hi == 0200) {                               /* end of tape? */
                        if ((csum - wd) & 07777) {                  /* valid csum? */
                            if (sections_read != 0)
                                Console::instance()->printf ("%d sections sucessfully read\n\r", sections_read);
                            return 2;
                        }
                        //if (!(sim_switches & SWMASK ('A')))        /* Load all sections? */
                        //    return SCPE_OK;
                        sections_read++;
                        break;
                    }
                    csum = csum + t + lo;                           /* add to csum */
                    if (wd > 07777) {                               /* chan 7 set? */
                        origin = wd & 07777;                        /* new origin */
						if (lastOrigin == 0)
							lastOrigin = origin;
					}
                    else {                                          /* no, data */
                        if ((field | origin) >= memorySize)
                            return 3;
                        m[field | origin] = wd;
                        origin = (origin + 1) & 07777;
                    }
                    field = newf;                                   /* update field */
                }
            }
            return 0;
        }

        int Memory::loadFile( const char * fileName ) {
            FILE    *fp = fopen( fileName, "r" );
            if (fp) {
				lastField = lastOrigin = 0;
                clearMemory();
                int r = sim_load_bin( fp );
                fclose( fp );
                Console::instance()->printf ("Last secion %1o%04o\n", lastField, lastOrigin);
				CPU::instance()->setPC(lastOrigin);
				CPU::instance()->setDF(0);
				CPU::instance()->setIF(0);
				int32_t t = m[CPU::instance()->getIF() | CPU::instance()->getPC()];
				ma = lastOrigin;
				CPU::instance()->setState(ExamineState);
                return r;
            }

            return 0;
        }

        void Memory::clearMemory() {
            for (int i = 0; i < MAXMEMSIZE; ++i) {
                m[i].clear();
            }
        }

    } /* namespace pdp8 */
} /* namespace ca */
/* vim: set ts=4 sw=4  noet autoindent : */
