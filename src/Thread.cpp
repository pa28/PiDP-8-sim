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
					throw s;
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
					throw s;
			}
		}
		
		Lock::~Lock() {
			int s = pthread_mutex_unlock( mutex );
			switch (s) {
				case 0:
					Console::instance()->printf("Mutex %0X unlocked\n", mutex);
					break;
				default:
					throw s;
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

    } /* namespace pdp8 */
} /* namespace ca */
