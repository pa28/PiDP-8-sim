/*
 * Device.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
 */

#include <iomanip>
#include "Device.h"

namespace ca
{
    namespace pdp8
    {

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
		
		int32_t Register::get(bool normal) {
			if (normal) {
				return ((*loc) & mask) >> offset;
			} else {
				return (*loc) & mask);
			}
		}
		
		void Register::set(int32_t v, bool normal) {
			*loc = (*loc) & ~mask;
			if (normal) {
				*loc |= (()v << offset) & mask;
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
		
        Device::Device(std::string nm, std::string longNm, Register *reg, int numReg, Modifier *mod, int numMod) :
			name(nm), longName(longNm), nReg(numReg), nMod(numMod), registers(reg), modifiers(mod)
        {
            // TODO Auto-generated constructor stub

        }

        Device::~Device()
        {
            // TODO Auto-generated destructor stub
        }

    } /* namespace pdp8 */
} /* namespace ca */
