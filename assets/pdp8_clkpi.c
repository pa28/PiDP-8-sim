/* pdp8_clkpi.c: PDP-8Pi programmable real-time clock simulator
 * Based on pdp8_clk.c by Robert M Supnik

   Copyright (c) 2015, H Richard Buckley
   Copyright (c) 1993-2012, Robert M Supnik

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

   clk          real time clock

   11-Jul-15    HRB     Expand to create a programmable real-time clock
   18-Apr-12    RMS     Added clock coscheduling
   18-Jun-07    RMS     Added UNIT_IDLE flag
   01-Mar-03    RMS     Aded SET/SHOW CLK FREQ support
   04-Oct-02    RMS     Added DIB, device number support
   30-Dec-01    RMS     Removed for generalized timers
   05-Sep-01    RMS     Added terminal multiplexor support
   17-Jul-01    RMS     Moved function prototype
   05-Mar-01    RMS     Added clock calibration support

*/

#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "pdp8_defs.h"

extern int32 int_req, int_enable, dev_done, stop_inst;

int32 clkpi_tps = 60;                                     /* ticks/second */
int32 clkpi_buf0 = 0;                                     /* count buffer 0 */
int32 clkpi_buf1 = 0;                                     /* count buffer 1 */
int32 clkpi_cnt0 = 0;                                     /* counter 0 */
int32 clkpi_cnt1 = 0;                                     /* counter 1 */
int32 seed = 0;                                           /* random seed */
int32 clkpi_clk_mode = 1;                                 /* clock mode DK8EAPi default */
int32 clkpi_flags;                                        /* clock operating mode and state flags */
int32 clkpi_rand = 0;									  /* random mode 0 hwrng 1 prng */

int hwrng = -1;											  /* file descriptor of the hardware random number generator */
char *hwrngPath = "/dev/hwrng";							  /* the path name of the hardware random number generator */

#define CLK_INT_FUNDAMENTAL     (0001)
#define CLK_INT_BASE            (0002)
#define CLK_INT_MULT            (0004)
#define CLK_OPR_SEQ             (0010)
#define CLK_FLAG_FUNDAMENTAL    (0100)
#define CLK_FLAG_BASE           (0200)
#define CLK_FLAG_MULT           (0400)

/*
 * We should be able to continue to use the value of tmxr_poll defined in pdp8_clk.c
 * as long as it is built into the simulation
 */
int32 tmxr_poll = 16000;                                /* term mux poll */
//extern int32 tmxr_poll;

extern int32 sim_is_running;

int32 clkpi (int32 IR, int32 AC);
t_stat clkpi_svc (UNIT *uptr);
t_stat clkpi_reset (DEVICE *dptr);
t_stat clkpi_set_mode (UNIT *uptr, int32 val, char *cptr, void *desc);
t_stat clkpi_show_mode (FILE *st, UNIT *uptr, int32 val, void *desc);
t_stat clkpi_set_freq (UNIT *uptr, int32 val, char *cptr, void *desc);
t_stat clkpi_show_freq (FILE *st, UNIT *uptr, int32 val, void *desc);
t_stat clkpi_set_rand (UNIT *uptr, int32 val, char *cptr, void *desc);
t_stat clkpi_show_rand (FILE *st, UNIT *uptr, int32 val, void *desc);

/* CLK data structures

   clk_dev      CLK device descriptor
   clkpi_unit     CLK unit descriptor
   clkpi_reg      CLK register list
*/

DIB clkpi_dib = { DEV_CLK, 1, { &clkpi } };

UNIT clkpi_unit = { UDATA (&clkpi_svc, UNIT_IDLE, 0), 16000 };

REG clkpi_reg[] = {
    { FLDATA (DONE, dev_done, INT_V_CLK) },
    { FLDATA (ENABLE, int_enable, INT_V_CLK) },
    { FLDATA (INT, int_req, INT_V_CLK) },
    { DRDATA (TIME, clkpi_unit.wait, 24), REG_NZ + PV_LEFT },
    { DRDATA (TPS, clkpi_tps, 8), PV_LEFT + REG_HRO },
    { DRDATA (BUF0, clkpi_buf0, 12) },
    { DRDATA (BUF1, clkpi_buf1, 12) },
    { DRDATA (CNT0, clkpi_cnt0, 12) },
    { DRDATA (CNT1, clkpi_cnt1, 12) },
    { DRDATA (SEED, seed, 32) },
    { ORDATA (FLAGS, clkpi_flags, 12) },
    { NULL }
    };

MTAB clkpi_mod[] = {
    // Clock drive frequency settings
    { MTAB_XTD|MTAB_VDV, 50, NULL, "50HZ",
      &clkpi_set_freq, NULL, NULL },
    { MTAB_XTD|MTAB_VDV, 60, NULL, "60HZ",
      &clkpi_set_freq, NULL, NULL },
    { MTAB_XTD|MTAB_VDV, 0, "FREQUENCY", NULL,
      NULL, &clkpi_show_freq, NULL },
    // Clock operation mode settings
    { MTAB_XTD|MTAB_VDV, 0, NULL, "EA",
      &clkpi_set_mode, NULL, NULL },
    { MTAB_XTD|MTAB_VDV, 1, NULL, "PI",
      &clkpi_set_mode, NULL, NULL },
    { MTAB_XTD|MTAB_VDV, 0, "MODE", NULL,
      NULL, &clkpi_show_mode, NULL },
	// Random number generator mode
    { MTAB_XTD|MTAB_VDV, 0, NULL, "HWRNG",
      &clkpi_set_rand, NULL, NULL },
    { MTAB_XTD|MTAB_VDV, 1, NULL, "PRNG",
      &clkpi_set_rand, NULL, NULL },
	{ MTAB_XTD|MTAB_VDV, 0, "RAND", NULL,
	  NULL, &clkpi_show_rand, NULL },
    // Show the devcie
    { MTAB_XTD|MTAB_VDV, 0, "DEVNO", NULL, NULL, &show_dev },
    { 0 }
    };

DEVICE clk_dev = {
    "CLK", &clkpi_unit, clkpi_reg, clkpi_mod,
    1, 0, 0, 0, 0, 0,
    NULL, NULL, &clkpi_reset,
    NULL, NULL, NULL,
    &clkpi_dib, 0
    };

int32 clkpi (int32 IR, int32 AC)
{
switch (clkpi_clk_mode) {
    case 0:                                                 /* DK8EA */
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
    case 1:                                                 /* DK8EAPi */
    /* IOT routine

       IOT's 6131-6133 are the PDP-8/E clock
       6134 CLSI    Set buffer and counter 0 from AC, return AC has old value
       6135 CLSM    Set buffer and counter 1 from AC, return AC has old value
       6136 RAND    Return a psuedo random number
    */

    switch (IR & 07) {                                      /* decode IR<9:11> */
        int32   tmp;

        case 0:                                             /* CLSF */
            clkpi_flags = AC & 077;                         /* Set programmable flags */
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
            tmp = clkpi_buf0;
            clkpi_buf0 = AC;
            clkpi_cnt0 = AC;
            return tmp;

        case 5:                                             /* CLSM */
            tmp = clkpi_buf1;
            clkpi_buf1 = AC;
            clkpi_cnt1 = AC;
            return tmp;

        case 6:                                             /* RAND */
            if (clkpi_rand == 0) {
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
            AC = clkpi_flags & 07777;
            clkpi_flags = AC &00077;
            return AC;

        default:
            return (stop_inst << IOT_V_REASON) + AC;
            }                                               /* end switch */
        break;
    }
}

/* Unit service */

t_stat clkpi_svc (UNIT *uptr)
{
int32 t;

switch(clkpi_clk_mode) {
    case 0:                                                 /* DK8EA */
        dev_done = dev_done | INT_CLK;                          /* set done */
        int_req = INT_UPDATE;                                   /* update interrupts */
        clkpi_flags |= (CLK_FLAG_FUNDAMENTAL);
        t = sim_rtcn_calb (clkpi_tps, TMR_CLK);                 /* calibrate clock */
        sim_activate (&clkpi_unit, t);                          /* reactivate unit */
        tmxr_poll = t;                                          /* set mux poll */
        return SCPE_OK;
    case 1:                                                 /* DK8EAPi */
        if (clkpi_flags & CLK_INT_FUNDAMENTAL) {
            dev_done = dev_done | INT_CLK;                      /* set done */
            int_req = INT_UPDATE;                               /* update interrupts */
            clkpi_flags |= (CLK_FLAG_FUNDAMENTAL);
        }
        if (clkpi_flags & CLK_OPR_SEQ) {
            if (clkpi_cnt0 == 0) {
                if (clkpi_cnt1 == 0) {
                    clkpi_cnt0 = clkpi_buf0;                         /* reload base counter */
                    clkpi_cnt1 = clkpi_buf1;                         /* reload multiplier */
                } else {
                    clkpi_cnt1 --;
                    if (clkpi_cnt1 == 0) {
                        dev_done = dev_done | INT_CLK;                      /* set done */
                        int_req = INT_UPDATE;                               /* update interrupts */
                        clkpi_flags |= (CLK_FLAG_MULT);
                    }
                }
            } else {
                clkpi_cnt0 --;
                if (clkpi_cnt0 == 0) {
                    dev_done = dev_done | INT_CLK;                      /* set done */
                    int_req = INT_UPDATE;                               /* update interrupts */
                    clkpi_flags |= (CLK_FLAG_BASE);
                }
            }
        } else {
            if (clkpi_cnt0 == 0) {
                if (clkpi_flags & CLK_INT_BASE) {
                    dev_done = dev_done | INT_CLK;                      /* set done */
                    int_req = INT_UPDATE;                               /* update interrupts */
                    clkpi_flags |= (CLK_FLAG_BASE);
                }
                if (clkpi_cnt1 == 0) {
                    if (clkpi_flags & CLK_INT_MULT) {
                        dev_done = dev_done | INT_CLK;                      /* set done */
                        int_req = INT_UPDATE;                               /* update interrupts */
                        clkpi_flags |= (CLK_FLAG_MULT);
                    }
                    clkpi_cnt0 = clkpi_buf0;                         /* reload base counter */
                    clkpi_cnt1 = clkpi_buf1;                         /* reload multiplier */
                } else {
                    clkpi_cnt0 = clkpi_buf0;
                    clkpi_cnt1 --;
                }
            } else {
                clkpi_cnt0 --;
            }
        }
        t = sim_rtcn_calb (clkpi_tps, TMR_CLK);                 /* calibrate clock */
        sim_activate (&clkpi_unit, t);                          /* reactivate unit */
        tmxr_poll = t;                                          /* set mux poll */
        return SCPE_OK;
    }
}

/* Clock coscheduling routine */

int32 clk_cosched (int32 wait)
{
int32 t;

t = sim_is_active (&clkpi_unit);
return (t? t - 1: wait);
}

/* Reset routine */

t_stat clkpi_reset (DEVICE *dptr)
{
int32 t;

dev_done = dev_done & ~INT_CLK;                         /* clear done, int */
int_req = int_req & ~INT_CLK;
int_enable = int_enable & ~INT_CLK;                     /* clear enable */

if (hwrng < 0) {
    if (access(hwrngPath, R_OK) == 0) {
        hwrng = open(hwrngPath, O_RDONLY);
        clkpi_rand = 0;
    } else {
        hwrng = -1;
        clkpi_rand = 1;
        fprintf(stderr, "Can not access %s\n", hwrngPath);
    }
}

if (seed == 0)
	seed = (int32)time(NULL);                           /* random seed */

switch(clkpi_clk_mode) {
    case 0:
        clkpi_flags = CLK_INT_FUNDAMENTAL;
        break;
    case 1:
        clkpi_flags = CLK_INT_MULT;
        break;
}

if (!sim_is_running) {                                  /* RESET (not CAF)? */
    t = sim_rtcn_init (clkpi_unit.wait, TMR_CLK);
    sim_activate (&clkpi_unit, t);                        /* activate unit */
    tmxr_poll = t;
    }
return SCPE_OK;
}

/* Set frequency */

t_stat clkpi_set_freq (UNIT *uptr, int32 val, char *cptr, void *desc)
{
if (cptr)
    return SCPE_ARG;
if ((val != 50) && (val != 60))
    return SCPE_IERR;
clkpi_tps = val;
return SCPE_OK;
}

/* Set mode */

t_stat clkpi_set_mode (UNIT *uptr, int32 val, char *cptr, void *desc)
{
if (cptr)
    return SCPE_ARG;
if ((val != 0) && (val != 1))
    return SCPE_IERR;
clkpi_clk_mode = val;
return SCPE_OK;
}

/* Set RAND */

t_stat clkpi_set_rand (UNIT *uptr, int32 val, char *cptr, void *desc)
{
if (cptr)
    return SCPE_ARG;
if ((val != 0) && (val != 1))
    return SCPE_IERR;
if (val == 1) {
	clkpi_rand = val;
	return SCPE_OK;
}

if (hwrng >= 0) {
	clkpi_rand = val;
	return SCPE_OK;
}

clkpi_rand = 1;
return SCPE_IERR;
}

/* Show frequency */

t_stat clkpi_show_freq (FILE *st, UNIT *uptr, int32 val, void *desc)
{
fprintf (st, (clkpi_tps == 50)? "50Hz": "60Hz");
return SCPE_OK;
}

/* Show mode */

t_stat clkpi_show_mode (FILE *st, UNIT *uptr, int32 val, void *desc)
{
fprintf (st, (clkpi_clk_mode == 0)? "EA": "Pi");
return SCPE_OK;
}

/* Show rand */

t_stat clkpi_show_rand (FILE *st, UNIT *uptr, int32 val, void *desc)
{
fprintf (st, (clkpi_rand == 0)? "HWRNG": "PRNG");
return SCPE_OK;
}

/* vim: set ts=4 sw=4 et : */
