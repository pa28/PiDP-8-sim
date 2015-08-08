/*
 * Thread.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
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
