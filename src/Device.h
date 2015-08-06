/*
 * Device.h
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
 */

#ifndef DEVICE_H_
#define DEVICE_H_

#include <string>
#include <iostream>
#include <stdint.h>

using namespace std;

namespace ca
{
    namespace pdp8
    {

		class Register
		{
		public:
			Register( const char *nm, int32_t *l, int32_t r, int32_t w, int32_t o, int32_t d);
			virtual ~Register() {}

			int32_t				get(bool normal = false) const;
			void				set(int32_t v, bool normal = false);

			ostream&			printOn(ostream &strm) const;
			istream&			scanFrom(istream &strm);

		protected:
			string              name;                           /* name */
			int32_t             *loc;                           /* location */
			uint32_t            radix;                          /* radix */
			uint32_t            width;                          /* width */
			uint32_t            offset;                         /* starting bit */
			uint32_t            depth;                          /* save depth */
			uint32_t            mask;

			static int32_t		nmask[];
		};

		inline ostream & operator << (ostream & s, const Register &r) { return r.printOn(s); }
		inline istream & operator >> (istream & s, Register &r) { return r.scanFrom(s); }

		class Modifier
		{
		public:
			Modifier(const char *nm, int32_t *l, int32_t v, int32_t m) :
				name(nm), loc(l), value(v), mask(m)
				{
				}
			virtual ~Modifier() {}

		protected:
			string				name;
			int32_t				*loc;
			int32_t				value;
			int32_t				mask;
		};

		class Device
        {
        public:
            Device(std::string name, std::string longName, Register *reg = NULL, int numReg = 0, Modifier *mod = NULL, int numMod = 0);
            virtual ~Device();

            virtual void initialize() = 0;
			virtual void reset() = 0;
			virtual void stop() = 0;

			virtual int32_t dispatch(int32_t IR, int32_t dat) { return 0; }

		protected:
			string		name, longName;
			int			nReg, nMod;
			Register	*registers;
			Modifier	*modifiers;

        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* DEVICE_H_ */
