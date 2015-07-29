/*
 * Thread.h
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

#ifndef THREAD_H_
#define THREAD_H_

#include <time.h>
#include <pthread.h>
#include <stdint.h>

namespace ca
{
    namespace pdp8
    {
		class Lock
		{
		public:
			Lock( pthread_mutex_t * mutex );
			Lock( pthread_mutex_t & mutex );
			virtual ~Lock();
			
		private:
			pthread_muxtex_t * mutex;
			
		};

        class Thread
        {
        public:
            Thread();
            virtual ~Thread();

            int start();

            virtual int run() = 0;
		
		protected:
			pthread_t	thread;
			
        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* THREAD_H_ */
