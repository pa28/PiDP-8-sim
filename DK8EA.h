/*
 * DK8EA.h
 *
 *  Created on: Aug 7, 2015
 *      Author: H Richard Buckley
 */

#ifndef PIDP_DK8EA_H
#define PIDP_DK8EA_H


#include <cstdint>
#include "Device.h"

namespace pdp8
{

#define DK8EA_REGISTERS \
        X(FLAGS, clk_flags, 8, 12, 0, 12 ) \
        X(BUF0, clk_buf0, 8, 12, 0, 12 ) \
        X(BUF1, clk_buf1, 8, 12, 0, 12 ) \
        X(CNT0, clk_cnt0, 8, 12, 0, 12 ) \
        X(CNT1, clk_cnt1, 8, 12, 0, 12 )

    enum DK8EA_Flags {
        CLK_INT_FUNDAMENTAL = 0001,
        CLK_INT_BASE = 0002,
        CLK_INT_MULT = 0004,
        CLK_OPR_SEQ = 0010,
        CLK_FLAG_FUNDAMENTAL = 0100,
        CLK_FLAG_BASE = 0200,
        CLK_FLAG_MULT = 0400,
    };

    enum DK8EA_Constants {
        DK8EA_Mode_A = 0,
        DK8EA_Mode_P = 1,
        DK8EA_PRNG = 0,
        DK8EA_HWRNG = 1,
    };

    class DK8EA : public Device
    {
        DK8EA();

    public:
        virtual ~DK8EA();

        static DK8EA * instance();

        virtual void initialize();
        virtual void reset();
        virtual void stop() {}

        virtual int32_t dispatch(int32_t IR, int32_t dat);

        void	tick();

        enum RegisterIndex {
#define X(nm,loc,r,w,o,d)	Index ## nm,
            DK8EA_REGISTERS
#undef X
        };

#define X(nm,loc,r,w,o,d)	int32_t get ## nm (bool normal = false) const { return clkRegisters[Index ## nm].get(normal); }
        DK8EA_REGISTERS
#undef X

#define X(nm,loc,r,w,o,d)	void set ## nm (int32_t v, bool normal = false) { clkRegisters[Index ## nm].set(v, normal); }
        DK8EA_REGISTERS
#undef X

    protected:
        static DK8EA *    _instance;
        static Register clkRegisters[];

    };

} /* namespace pdp8 */



#endif //PIDP_DK8EA_H
