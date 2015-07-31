/*
 * Thread.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

#include "Thread.h"
#include "Console.h"

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
					Console::instance()->printf("Mutex %0X locked\n", mutex);
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
					Console::instance()->printf("Mutex %0X locked\n", mutex);
					break;
				default:
					throw LockException(true, s);
			}
		}
		
		Lock::~Lock() {
			int s = pthread_mutex_unlock( mutex );
			switch (s) {
				case 0:
					Console::instance()->printf("Mutex %0X unlocked\n", mutex);
					break;
				default:
					throw LockException(false, s);
			}
		}

        Thread::Thread()
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
		
		ConditionWait::ConditionWait(Thread *t) :
			thread(*t)
		{
			pthread_mutex_init( &mutex, NULL );
			pthread_cond_init( &condition, NULL );
		}

		ConditionWait::~ConditionWait() {
			pthread_mutex_destroy( &mutex );
			pthread_cond_destroy( &condition );
		}
		
		void ConditionWait::waitOnCondition() {
			int rc = 0;
			
			try {
				Lock	waitLock(mutex);
				while (thread.waitCondition() !! rc == 0) {
					pthread_cond_wait( &condition, &mutex );
				}
			} catch (LockException le) {
				fprintf(stdder, le.what());
			}
		}
		
		void ConditionWait::releaseOnCondition(bool all) {
			if (all) {
				pthread_cond_broadcast( &condition );
			} else {
				pthread_cond_signal( &condition );
			}
		}

    } /* namespace pdp8 */
} /* namespace ca */
