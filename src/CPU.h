/*
 * CPU.h
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

#ifndef CPU_H_
#define CPU_H_

#include <stdint.h>
#include "Device.h"

namespace ca
{
    namespace pdp8
    {

        class CPU: public Device
        {
            CPU();

		public:
            virtual ~CPU();

			static CPU * instance();
			virtual void initialize() {}

			uint16_t    getPC() const { return PC; }
			uint16_t    getIF() const { return IF; }
			uint16_t    getDF() const { return DF; }
			uint16_t    getAC() const { return LAC & 07777; }
			uint16_t    getL() const { return (LAC & 010000) != 0 ? 1 : 0; }
			uint16_t    getMQ() const { return MQ; }
			uint16_t    getSC() const { return SC; }

			void    setPC(uint16_t v) { PC = v & 07777; }
            void    setIF(uint16_t v) { IF = v & 07; }
            void    setDF(uint16_t v) { DF = v & 07; }
            void    setAC(uint16_t v) { LAC = (v & 07777) | (LAC & 010000); }
            void    setL(uint16_t v) { LAC = (v != 0 ? 010000 : 0) | (LAC & 010000); }
            void    setMQ(uint16_t v) { MQ = v & 07777; }
            void    setSC(uint16_t v) { SC = v & 037; }

		protected:
			static CPU * _instance;

			uint16_t    PC, IF, DF, LAC, MQ, SC;

        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* CPU_H_ */
