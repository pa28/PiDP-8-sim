/**
 * @file NullStream.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-28
 */

#pragma once

#include <ext/stdio_filebuf.h>
#include <iostream>

namespace null_stream {

    using stdio_filebuf = __gnu_cxx::stdio_filebuf<char>;   ///< Used to create NullStreamBuffer.

    /**
     * @class NullStreamBuffer
     * @brief A Stream buffer that doesnt buffer data.
     * @details A NullStreamBuffer may be used where you have to pass a std::istream or std::ostream but
     * don't want any output. Create the required stream with a NullStreamBuffer and pass it wehre required.
     */
    class NullStreamBuffer : public std::streambuf {
    protected:
        int overflow(int c) override { return c; }
    };

}

