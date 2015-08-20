/*
 * Thread.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley

   Portions of this program are based substantially on work by Robert M Supnik
   The license for Mr Supnik's work follows:

   Copyright (c) 1993-2013, Robert M Supnik

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
 */

#include <stdio.h>
#include "Thread.h"
#include "Console.h"

#define DEBUG_LEVEL 5
#include "PDP8.h"

namespace ca
{
    namespace pdp8
    {
        void * _threadStart(void *t) {
            Thread *thread = (Thread*)t;

            return (void *)(thread->run());
        }

		Lock::Lock( pthread_mutex_t * m ) : mutex(m)
		{
			int s = pthread_mutex_lock( mutex );
			switch (s) {
				case 0:
					debug(10, "Mutex %0X locked\n", mutex);
					break;
				default:
					throw LockException(true, s);
			}
		}

		Lock::Lock( pthread_mutex_t & m ) : mutex(&m)
		{
			int s = pthread_mutex_lock( mutex );
			switch (s) {
				case 0:
					debug(10, "Mutex %0X locked\n", mutex);
					break;
				default:
					throw LockException(true, s);
			}
		}

		Lock::Lock( ConditionWait & cw ) : mutex(&cw.mutex)
		{
			int s = pthread_mutex_lock( mutex );
			switch (s) {
				case 0:
					debug(10, "ConditionWait mutex %0X locked\n", mutex);
					break;
				default:
					throw LockException(true, s);
			}
		}

		Lock::Lock( Mutex & m ) : mutex(&m.mutex)
		{
			int s = pthread_mutex_lock( mutex );
			switch (s) {
				case 0:
					debug(10, "ConditionWait mutex %0X locked\n", mutex);
					break;
				default:
					throw LockException(true, s);
			}
		}

		Lock::~Lock() {
			int s = pthread_mutex_unlock( mutex );
			switch (s) {
				case 0:
					debug(10, "Mutex %0X unlocked\n", mutex);
					break;
				default:
					throw LockException(false, s);
			}
		}

        Thread::Thread() :
                thread(0)
        {
        }

        Thread::~Thread()
        {
        }

		int Thread::start() {
			int s = pthread_create( &thread, NULL, _threadStart, this );
			switch (s) {
				case 0:
					break;
				default:
					return -1;
			}

			return 0;
		}

		ConditionWait::ConditionWait() :
			test(true)
		{
			pthread_mutex_init( &mutex, NULL );
			pthread_cond_init( &condition, NULL );
		}

		ConditionWait::~ConditionWait() {
			pthread_mutex_destroy( &mutex );
			pthread_cond_destroy( &condition );
		}

		bool ConditionWait::waitOnCondition() {
			int rc = 0;
			bool r = false;

            debug(1, "test %d\n", test);

			try {
				Lock	waitLock(mutex);
				if (!test)
				while (!test && rc == 0) {
					rc = pthread_cond_wait( &condition, &mutex );
				}
				r = test = true;
			} catch (LockException &le) {
				Console::instance()->printf(le.what());
			}

			debug(1, "returning %d\n", r);
			return r;
		}

		void ConditionWait::releaseOnCondition(bool all) {
			try {
				Lock	waitLock(mutex);
				if (!test) {
					debug(1, "release %d\n", all);
					if (all) {
						pthread_cond_broadcast( &condition );
					} else {
						pthread_cond_signal( &condition );
					}
					test = true;
				}
			} catch (LockException &le) {
				Console::instance()->printf(le.what());
			}
		}

    } /* namespace pdp8 */
} /* namespace ca */
/* vim: set ts=4 sw=4  noet autoindent : */
