//
// Created by richard on 30/01/18.
//

#ifndef PIDP_SIGNALHANDLER_H
#define PIDP_SIGNALHANDLER_H

#include <csignal>
#include <cstring>

namespace util
{

    /**
     * @brief A template class to install a signal handler that is only active while in scope.
     * @tparam SigNum The signal number to handle.
     */
    template <int SigNum>
    struct SignalHandler
    {
        /**
         * (constructor)
         * @brief Install a default signal handler that records that the signal has occurred.
         * @param f a user provided handler function
         */
        explicit SignalHandler(__sighandler_t f = nullptr) :
                oldAction{}
        {
            signaled = false;
            struct sigaction sa{};

            std::memset(&sa, 0, sizeof(sa));
            std::memset(&oldAction, 0, sizeof(oldAction));

            if (f)
                sa.sa_handler = f;
            else
                sa.sa_handler = &SignalHandler<SigNum>::sigHandler;
            sigaction( SigNum, &sa, &oldAction );
        }

        /**
         * (constructor)
         * @brief Install a default signal handler that records that the signal has occurred,
         * but the user provides the sigaction struct into which sa_handler is written.
         * @param sa user provided sigaction structure
         * @param f a user provided handler function
         */
        explicit SignalHandler(struct sigaction &sa, __sighandler_t f = nullptr) :
                oldAction{}
        {
            signaled = false;

            std::memset(&sa, 0, sizeof(sa));
            std::memset(&oldAction, 0, sizeof(oldAction));

            if (f)
                sa.sa_handler = f;
            else
                sa.sa_handler = &SignalHandler<SigNum>::sigHandler;
            sigaction( SigNum, &sa, &oldAction );
        }

        /**
         * (destructor)
         * Removes the signal handler when the object goes out of scope, restores the previous
         * action.
         */
        ~SignalHandler() {
            sigaction( SigNum, &oldAction, nullptr );
        }

        /**
         * Access the signal state and reset it to false.
         * @return
         */
        explicit operator bool () {
            if (!signaled)
                return false;

            signaled = false;
            return true;
        }

        /**
         * The signal handler function
         */
        static void sigHandler(int) {
            signaled = true;
        }

        /**
         * Member fields.
         */
        struct sigaction oldAction;
        static bool signaled;
    };

    /**
     * Static member fields
     * @tparam SigNum The signal number.
     */
    template <int SigNum> bool SignalHandler<SigNum>::signaled{false};

}

#endif //PIDP_SIGNALHANDLER_H
