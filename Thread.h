/*
 * Thread.h
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
 */

#ifndef PIDP_THREAD_H
#define PIDP_THREAD_H


#ifndef THREAD_H_
#define THREAD_H_

#include <ctime>
#include <pthread.h>
#include <cstdint>
#include <cerrno>
#include <exception>

namespace pdp8
{
    class LockException : public std::exception {
    public:
        LockException(bool l, int c) : locking(l), code(c) {}
        virtual ~LockException() throw() {}
        virtual const char* what() const throw() {
            if (locking)
                switch (code) {
                    case EINVAL:
                        return "Lock on invalid mutex.";
                    case EBUSY:
                        return "Already locked.";
                    case EAGAIN:
                        return "Too many recursive locks.";
                    case EDEADLK:
                        return "Thread already owns lock.";
                    default:
                        return "Unknown locking error.";
                }
            else
                switch (code) {
                    case EINVAL:
                        return "Unock on invalid mutex.";
                    case EAGAIN:
                        return "Too many recursive locks.";
                    case EPERM:
                        return "Thread does not own lock.";
                    default:
                        return "Unknown unlocking error.";
                }
        }
    protected:
        bool locking;
        int	code;
    };

    class ConditionWait;
    class Lock;

    class Mutex
    {
        friend class Lock;
    public:
        Mutex() { pthread_mutex_init( &mutex, NULL ); }
        virtual ~Mutex() { pthread_mutex_destroy( &mutex ); }

    protected:
        pthread_mutex_t		mutex;
    };

    class Lock
    {
    public:
        Lock( pthread_mutex_t * mutex );
        Lock( pthread_mutex_t & mutex );
        Lock( ConditionWait & conditionWait );
        Lock( Mutex & mutex );
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

    public:
        ConditionWait();
        virtual ~ConditionWait();

        void releaseOnCondition(bool all = false);
        bool waitOnCondition();

    private:
        bool				test;           // true when
        pthread_mutex_t		mutex;
        pthread_cond_t		condition;

    };

} /* namespace pdp8 */

#endif //PIDP_THREAD_H
