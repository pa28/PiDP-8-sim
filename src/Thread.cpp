/*
 * Thread.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

#include "Thread.h"

namespace ca
{
    namespace pdp8
    {
        void * _threadStart(void *t) {
            Thread *thread = (Thread*)t;

            return (void *)(thread->run());
        }

        Thread::Thread()
        {
            // TODO Auto-generated constructor stub

        }

        Thread::~Thread()
        {
            // TODO Auto-generated destructor stub
        }

    } /* namespace pdp8 */
} /* namespace ca */
