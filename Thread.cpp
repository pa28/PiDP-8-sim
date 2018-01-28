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

#include <sstream>
#include <cstring>
#include <cerrno>
#include "Thread.h"

#include "PDP8.h"
#include "Console.h"

namespace pdp8
{
    void * _threadStart(void *t) {
        auto *thread = (Thread*)t;

        return thread->run();
    }

    Lock::Lock( pthread_mutex_t * m ) : mutex(m) { construct(); }

    Lock::Lock( pthread_mutex_t & m ) : mutex(&m) { construct(); }

    Lock::Lock( ConditionWait & cw ) : mutex(&cw.mutex) { construct(); }

    Lock::Lock( Mutex & m ) : mutex(&m.mutex) { construct(); }

    void Lock::construct() {
        int s = pthread_mutex_lock( mutex );
        if (s) {
            std::stringstream ss;
            ss << "Lock failed "
               << strerror(errno);
            throw LockException(ss.str());
        }
    }

    Lock::~Lock() {
        int s = pthread_mutex_unlock( mutex );
        if (s) {
            ERROR("Mutex unlock error %s", strerror(errno));
        }
    }

    Thread::Thread() :
            thread(0)
    {
    }

    int Thread::start() {
        int s = pthread_create( &thread, nullptr, _threadStart, this );
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
        pthread_mutex_init( &mutex, nullptr);
        pthread_cond_init( &condition, nullptr);
    }

    ConditionWait::~ConditionWait() {
        pthread_mutex_destroy( &mutex );
        pthread_cond_destroy( &condition );
    }

    bool ConditionWait::waitOnCondition() {
        int rc = 0;
        bool r = false;

        try {
            Lock	waitLock(mutex);
            if (!test)
                while (!test && rc == 0) {
                    rc = pthread_cond_wait( &condition, &mutex );
                }
            r = test = true;
        } catch (LockException &le) {
            auto console = Console::getConsole();
            if (console != nullptr)
                console->printf(le.what());
        }

        return r;
    }

    void ConditionWait::releaseOnCondition(bool all) {
        try {
            Lock	waitLock(mutex);
            if (!test) {
                if (all) {
                    pthread_cond_broadcast( &condition );
                } else {
                    pthread_cond_signal( &condition );
                }
                test = true;
            }
        } catch (LockException &le) {
            auto console = Console::getConsole();
            if (console != nullptr)
                console->printf(le.what());
        }
    }

} /* namespace pdp8 */
