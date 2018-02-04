/*
 * Thread.h
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
 */

#ifndef PIDP_THREAD_H
#define PIDP_THREAD_H

#include <ctime>
#include <pthread.h>
#include <cstdint>
#include <cerrno>
#include <exception>
#include <stdexcept>

namespace util
{
    class LockException : public std::runtime_error {
    public:
        explicit LockException(const char *what_arg) : std::runtime_error(what_arg) {}
        explicit LockException(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };

    class ConditionWait;
    class Lock;

    class Mutex
    {
        friend class Lock;
    public:
        Mutex() { pthread_mutex_init( &mutex, nullptr ); }
        virtual ~Mutex() noexcept { pthread_mutex_destroy( &mutex ); }

    protected:
        pthread_mutex_t		mutex{};
    };

    class Lock
    {
    public:
        Lock() = delete;

        explicit Lock( pthread_mutex_t * mutex );

        explicit Lock( pthread_mutex_t & mutex );

        explicit Lock( ConditionWait & conditionWait );

        explicit Lock( Mutex & mutex );

        virtual ~Lock() noexcept;

    private:

        pthread_mutex_t * mutex;
        void construct();
    };

    /**
     * The Thread class implements a Posix thread. When start() is
     * called it will execute the run() function on the thread. The
     * stop() function should be implemented to cause the run() function
     * to exit which will terminate and destroy the thread.
     */
    class Thread
    {
    public:
        Thread();
        virtual ~Thread() noexcept = default;

        int start();

        virtual void * run() = 0;
        virtual void stop() = 0;

        virtual bool waitCondition() { return true; }

        bool threadComplete() const { return thread_complete != 0; }

    protected:
        pthread_t	thread;
        bool        _runFlag;
        volatile __sig_atomic_t thread_complete;

        friend void * _threadStart(void *t);
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

} /* namespace util */

#endif //PIDP_THREAD_H
