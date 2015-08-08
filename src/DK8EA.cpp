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
#include "CPU.h"

namespace ca
{
    namespace pdp8
    {
		DK8EA * DK8EA::_instance = NULL;

        int32_t clk_mode = DK8EA_Mode_P;
        int32_t clk_rand = DK8EA_HWRNG;

        int hwrng = -1;                                           /* file descriptor of the hardware random number generator */
        const char *hwrngPath = "/dev/hwrng";                     /* the path name of the hardware random number generator */
        uint32_t seed = 0;                                        /* random seed */

#define X(nm,loc,r,w,o,d)	int32_t	loc = 0;
		DK8EA_REGISTERS
#undef X
		Register DK8EA::clkRegisters[] = {
#define X(nm,loc,r,w,o,d) { #nm, &(loc), (r), (w), (o), (d) },
			DK8EA_REGISTERS
#undef X
		};

		DK8EA::DK8EA() :
			Device("clk", "DK8EA", clkRegisters, sizeof(clkRegisters), NULL, 0)
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
#define X(nm,loc,r,w,o,d)	loc = 0;
		DK8EA_REGISTERS
#undef X
		}

		void DK8EA::reset() {
			clk_flags = clk_cnt0 = clk_cnt1 = 0;
			int32_t t;

			dev_done = dev_done & ~INT_CLK;                         /* clear done, int */
			int_req = int_req & ~INT_CLK;
			int_enable = int_enable & ~INT_CLK;                     /* clear enable */

			if (hwrng < 0) {
			    if (access(hwrngPath, R_OK) == 0) {
			        hwrng = open(hwrngPath, O_RDONLY);
			        clk_rand = 0;
			    } else {
			        hwrng = -1;
			        clk_rand = 1;
			        fprintf(stderr, "Can not access %s\n", hwrngPath);
			    }
			}

			if (seed == 0)
			    seed = (uint32_t)time(NULL);                         /* random seed */

			switch(clk_mode) {
			    case 0:
			        clk_flags = CLK_INT_FUNDAMENTAL;
			        break;
			    case 1:
			        clk_flags = CLK_INT_MULT;
			        break;
			}
		}

		int32_t DK8EA::dispatch(int32_t IR, int32_t AC) {
		switch (clk_mode) {
			case DK8EA_Mode_A:                                      /* DK8EA */
			/* IOT routine

			   IOT's 6131-6133 are the PDP-8/E clock
			   IOT's 6135-6137 are the PDP-8/A clock
			*/
			switch (IR & 07) {                                      /* decode IR<9:11> */

				case 1:                                             /* CLEI */
					int_enable = int_enable | INT_CLK;              /* enable clk ints */
					int_req = INT_UPDATE;                           /* update interrupts */
					return AC;

				case 2:                                             /* CLDI */
					int_enable = int_enable & ~INT_CLK;             /* disable clk ints */
					int_req = int_req & ~INT_CLK;                   /* update interrupts */
					return AC;

				case 3:                                             /* CLSC */
					if (dev_done & INT_CLK) {                       /* flag set? */
						dev_done = dev_done & ~INT_CLK;             /* clear flag */
						int_req = int_req & ~INT_CLK;               /* clear int req */
						return IOT_SKP + AC;
						}
					return AC;

				case 5:                                             /* CLLE */
					if (AC & 1)                                     /* test AC<11> */
						int_enable = int_enable | INT_CLK;
					else int_enable = int_enable & ~INT_CLK;
						int_req = INT_UPDATE;                           /* update interrupts */
					return AC;

				case 6:                                             /* CLCL */
					dev_done = dev_done & ~INT_CLK;                 /* clear flag */
					int_req = int_req & ~INT_CLK;                   /* clear int req */
					return AC;

				case 7:                                             /* CLSK */
					return (dev_done & INT_CLK)? IOT_SKP + AC: AC;

				default:
					return (stop_inst << IOT_V_REASON) + AC;
					}                                               /* end switch */
				break;
			case DK8EA_Mode_P:                                      /* DK8EAPi */
			/* IOT routine

			   IOT's 6131-6133 are the PDP-8/E clock
			   6134 CLSI    Set buffer and counter 0 from AC, return AC has old value
			   6135 CLSM    Set buffer and counter 1 from AC, return AC has old value
			   6136 RAND    Return a psuedo random number
			*/

			switch (IR & 07) {                                      /* decode IR<9:11> */
				int32_t   tmp;

				case 0:                                             /* CLSF */
					clk_flags = AC & 077;                           /* Set programmable flags */
					return AC;

				case 1:                                             /* CLEI */
					int_enable = int_enable | INT_CLK;              /* enable clk ints */
					int_req = INT_UPDATE;                           /* update interrupts */
					return AC;

				case 2:                                             /* CLDI */
					int_enable = int_enable & ~INT_CLK;             /* disable clk ints */
					int_req = int_req & ~INT_CLK;                   /* update interrupts */
					return AC;

				case 3:                                             /* CLSK */
					if (dev_done & INT_CLK) {                       /* flag set? */
						dev_done = dev_done & ~INT_CLK;             /* clear flag */
						int_req = int_req & ~INT_CLK;               /* clear int req */
						return IOT_SKP + AC;
						}
					return AC;

				case 4:                                             /* CLSI */
					tmp = clk_buf0;
					clk_buf0 = AC;
					clk_cnt0 = AC;
					return tmp;

				case 5:                                             /* CLSM */
					tmp = clk_buf1;
					clk_buf1 = AC;
					clk_cnt1 = AC;
					return tmp;

				case 6:                                             /* RAND */
					if (clk_rand == 0) {
						if (read(hwrng, &AC, sizeof(AC))) {
							AC &= 07777;
						} else {
							perror("error readind /dev/hwrng");
							return (stop_inst << IOT_V_REASON) + AC;
						}
					} else {
						AC = rand_r(&seed) & 07777;
					}
					return AC;

				case 7:                                             /* CLRF */
					AC = clk_flags & 07777;
					clk_flags = AC &00077;
					return AC;

				default:
					return (stop_inst << IOT_V_REASON) + AC;
					}                                               /* end switch */
				break;
			}

		}

		void DK8EA::tick() {
			// TODO: change dev_done and int_req updates.
			switch(clk_mode) {
				case 0:                                                 /* DK8EA */
					clk_flags |= (CLK_FLAG_FUNDAMENTAL);
					dev_done = dev_done | INT_CLK;                          /* set done */
					int_req = INT_UPDATE;                                   /* update interrupts */
					CPU::instance()->cpuContinueFromIdle();                 /* if idle, continue */
					break;
				case 1:                                                 /* DK8EAPi */
					if (clk_flags & CLK_INT_FUNDAMENTAL) {
						clk_flags |= (CLK_FLAG_FUNDAMENTAL);
						dev_done = dev_done | INT_CLK;                      /* set done */
						int_req = INT_UPDATE;                               /* update interrupts */
	                    CPU::instance()->cpuContinueFromIdle();                 /* if idle, continue */
					}
					if (clk_flags & CLK_OPR_SEQ) {
						if (clk_cnt0 == 0) {
							if (clk_cnt1 == 0) {
								clk_cnt0 = clk_buf0;                         /* reload base counter */
								clk_cnt1 = clk_buf1;                         /* reload multiplier */
							} else {
								clk_cnt1 --;
								if (clk_cnt1 == 0) {
									clk_flags |= (CLK_FLAG_MULT);
									dev_done = dev_done | INT_CLK;                      /* set done */
									int_req = INT_UPDATE;                               /* update interrupts */
				                    CPU::instance()->cpuContinueFromIdle();                 /* if idle, continue */
								}
							}
						} else {
							clk_cnt0 --;
							if (clk_cnt0 == 0) {
								clk_flags |= (CLK_FLAG_BASE);
								dev_done = dev_done | INT_CLK;                      /* set done */
								int_req = INT_UPDATE;                               /* update interrupts */
			                    CPU::instance()->cpuContinueFromIdle();                 /* if idle, continue */
							}
						}
					} else {
						if (clk_cnt0 == 0) {
							if (clk_flags & CLK_INT_BASE) {
								clk_flags |= (CLK_FLAG_BASE);
								dev_done = dev_done | INT_CLK;                      /* set done */
								int_req = INT_UPDATE;                               /* update interrupts */
			                    CPU::instance()->cpuContinueFromIdle();                 /* if idle, continue */
							}
							if (clk_cnt1 == 0) {
								if (clk_flags & CLK_INT_MULT) {
									clk_flags |= (CLK_FLAG_MULT);
									dev_done = dev_done | INT_CLK;                      /* set done */
									int_req = INT_UPDATE;                               /* update interrupts */
				                    CPU::instance()->cpuContinueFromIdle();                 /* if idle, continue */
								}
								clk_cnt0 = clk_buf0;                         /* reload base counter */
								clk_cnt1 = clk_buf1;                         /* reload multiplier */
							} else {
								clk_cnt0 = clk_buf0;
								clk_cnt1 --;
							}
						} else {
							clk_cnt0 --;
						}
					}
				}

		}


    } /* namespace pdp8 */
} /* namespace ca */

/* vim: set ts=4 sw=4  noet autoindent : */
