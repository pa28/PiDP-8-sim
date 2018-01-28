/*
 * Device.h
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
 */

#ifndef PIDP_DEVICE_H
#define PIDP_DEVICE_H


#include <string>
#include <iostream>
#include <stdint.h>

using namespace std;

namespace pdp8
{

#if 0
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

    enum ModifierType {
        ModifierValue,
        ModifierFunction,
    };

    class Modifier
    {
    public:
        Modifier(const char *nm, ModifierType t, void *l, int32_t v, int32_t m) :
                name(nm), type(t), loc(l), value(v), mask(m)
        {
        }
        virtual ~Modifier() {}

    protected:
        string				name;
        ModifierType		type;
        void				*loc;
        int32_t				value;
        int32_t				mask;
    };
#endif

} /* namespace pdp8 */




#endif //PIDP_DEVICE_H
