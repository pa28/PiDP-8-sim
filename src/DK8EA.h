/*
 * DK8EA.h
 *
 *  Created on: Aug 7, 2015
 *      Author: H Richard Buckley
 */

#ifndef DK8EA_H_
#define DK8EA_H_

#include <stdint.h>
#include "Device.h"

namespace ca
{
    namespace pdp8
    {
		
		#define REGISTERS \
			X(FLAGS, clk_flags, 8, 12, 0, 12 ) \
			X(BUF0, clk_buf0, 8, 12, 0, 12 ) \
			X(BUF1, clk_buf1, 8, 12, 0, 12 ) \
			X(CNT0, clk_cnt0, 8, 12, 0, 12 ) \
			X(CNT1, clk_cnt1, 8, 12, 0, 12 )

		class DK8EA : public Device
		{
			DK8EA();
			
		public:
			virtual ~DK8EA();
			
			static DK8EA * instance();

            virtual void initialize();
			virtual void reset();
			virtual void stop() {}

			virtual int32_t dispatch(int32_t IR, int32_t dat) { return 0; }
			
			void	tick();

			#define X(nm,loc,r,w,o,d)	Index ## nm,
			enum RegisterIndex {
				REGISTERS
				RegisterCount,
			};
			#undef X
			//
			//
			#define X(nm,loc,r,w,o,d)	int32_t get ## nm (bool normal = false) const { return clkRegisters[Index ## nm].get(normal); }
			REGISTERS
			#undef X

			#define X(nm,loc,r,w,o,d)	void set ## nm (int32_t v, bool normal = false) { clkRegisters[Index ## nm].set(v, normal); }
			REGISTERS
			#undef X
			
		protected:
			static DK8EA *    _instance;
			static Register clkRegisters[];

		};
		
    } /* namespace pdp8 */
} /* namespace ca */

#endif /* DK8EA_H_ */
/* vim: set ts=4 sw=4  noet autoindent : */
