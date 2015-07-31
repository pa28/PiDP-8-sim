/*
 * Thread.h
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
 */

#ifndef THREAD_H_
#define THREAD_H_

#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include <errno.h>
#include <exception>

namespace ca
{
    namespace pdp8
    {
		class LockException : public std::exception {
		public:
			LockException(bool l, int c) : locking(l), code(c) {}
			virtual ~MemoryException() throw() {}
			virtual const char* what() const throw() {
				if (locking)
					switch (code) {
						case EINVAL:
							return "Lock on invalid mutex.";
						case EBUSY:
							return "Already locked."
						case EAGAIN:
							return "Too many recursive locks."
						case EDEADLK:
							return "Thread already owns lock."
						default:
							return "Unknown locking error.";
					}
				else
					switch (code) {
						case EINVAL:
							return "Unock on invalid mutex.";
						case EAGAIN:
							return "Too many recursive locks."
						case EPERM:
							return "Thread does not own lock."
						default:
							return "Unknown unlocking error.";
					}
			}
		protected:
			bool locking;
			int	code;
		};

		class Lock
		{
		public:
			Lock( pthread_mutex_t * mutex );
			Lock( pthread_mutex_t & mutex );
			Lock( ConditionWait & conditionWait );
			virtual ~Lock();
			
		private:
			pthread_mutex_t * mutex;
			
		};
		
        class Thread
        {
        public:
            Thread();
            virtual ~Thread();

            int start();

            virtual int run() = 0;
			virtual void stop() = 0;
			
			virtual bool waitCondition() { return true; }
		
		protected:
			pthread_t	thread;
			
        };

		class ConditionWait
		{
			friend class Thread;
			friend class Lock;
			
			ConditionWait(Thread *thread);
			
		public:
			virtual ~ConditionWait();
			
			void releaseOnCondition(bool all = false);
			
		private:
			Thread			&	thread;
			pthread_mutex_t		mutex;
			pthread_cond_t		condition;

			
			bool waitOnCondition();
			
		};

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* THREAD_H_ */
