/*
 * DK8EA.h
 *
 *  Created on: Aug 7, 2015
 *      Author: H Richard Buckley
 */
 
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "DK8EA.h"

namespace ca
{
    namespace pdp8
    {
		DK8EA * DK8EA::_instance = NULL;
		
#define X(nm,loc,r,w,o,d)	int32_t	#nm = 0;
		REGISTERS
#undef X

		Register DK8EA::clkRegisters[] = {
#define X(nm,loc,r,w,o,d) { #nm, &(loc), (r), (w), (o), (d) },
			REGISTERS
#undef X
		};
		
		DK8EA::DK8EA() :
			Device("clk", "DK8EA", NULL, 0, NULL, 0)
#define X(nm,loc,r,w,o,d)	,#nm (0)
			REGISTERS
#undef X
		{
			
		}

		DK8EA::~DK8EA()
		{
			
		}
		
		DK8EA * DK8EA::instance() {
			if (_instance == NULL) {
				_instance = new DK8EA();
			}
			
			return _instance;
		}

        void DK8EA::initialize() {
#define X(nm,loc,r,w,o,d)	#nm = 0;
		REGISTERS
#undef X
		}
		
		void DK8EA::reset() {
			clk_flags = clk_cnt0 = clk_cnt1 = 0;
		}

		int32_t DK8EA::dispatch(int32_t IR, int32_t dat) {
			
		}
			
		tick();


    } /* namespace pdp8 */
} /* namespace ca */

/* vim: set ts=4 sw=4  noet autoindent : */
