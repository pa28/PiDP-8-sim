/*
 * Device.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
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

#include <iomanip>
#include "Device.h"

namespace pdp8
{
#if 0
    int32_t	Register::nmask[] = {
            0x0,
            0x1, 0x3, 0x7,	0xF,
            0x1F, 0x3F, 0x7F, 0xFF,
            0x1FF, 0x3FF, 0x7FF, 0xFFF,
            0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF
    };

    Register::Register( const char *nm, int32_t *l, int32_t r, int32_t w, int32_t o, int32_t d) :
            name(nm), loc(l), radix(r), width(w), offset(o), depth(d), mask(nmask[w] << o)
    {
    }

    int32_t Register::get(bool normal) const {
        if (normal) {
            return ((*loc) & mask) >> offset;
        } else {
            return ((*loc) & mask);
        }
    }

    void Register::set(int32_t v, bool normal) {
        *loc = (*loc) & ~mask;
        if (normal) {
            *loc |= ((v << offset) & mask);
        } else {
            *loc |= (v & mask);
        }
    }

    ostream& Register::printOn(ostream &s) const {
        if (radix != 10) {
            s << std::setfill('0') << std::setbase(radix) << std::setw(width) << get(true);
        } else {
            s << std::setbase(radix) << std::setw(width) << get(true);
        }

        return s;
    }

    istream& Register::scanFrom(istream &s) {
        int32_t i;
        s >> std::setbase(radix) >> i;
        set(i,true);
        return s;
    }
#endif

} /* namespace pdp8 */
