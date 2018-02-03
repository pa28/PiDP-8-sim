//
// Created by richard on 02/02/18.
//

#ifndef PIDP_ENCODER_H
#define PIDP_ENCODER_H


#include <streambuf>
#include <cstring>
#include <stdexcept>

using namespace std;

namespace util {

    class EncodingError : public std::logic_error {
    public:
        EncodingError() = delete;

        explicit EncodingError(const char *w) : std::logic_error(w) {}

        explicit EncodingError(const std::string &w) : std::logic_error(w) {}
    };


    class DecodingError : public std::logic_error {
    public:
        DecodingError() = delete;

        explicit DecodingError(const char *w) : std::logic_error(w) {}

        explicit DecodingError(const std::string &w) : std::logic_error(w) {}
    };


    template <class CharT, class Traits = std::char_traits<CharT>>
    class Encoder : public std::basic_streambuf<CharT>
    {
    public:

        typedef CharT char_type;
        typedef Traits traits_type;
        typedef typename Traits::int_type int_type;

        explicit Encoder(std::basic_streambuf<CharT> &sb) :
                _sb(sb),
                outIdle{true},
                inIdle{true},
                atEnd{false} {
            this->setp(obuf, obuf+obuf_len);
            this->setg(ibuf, ibuf+putback_len, ibuf+putback_len);

            memset(obuf, 0, obuf_len);
        }

    protected:
        enum Escaped {
            STX = 1,     // Start of text
            ETX = 2,     // End of text
            SO = 3,      // Shift Out
            SI = 4,      // Shift In
            SYN = 5,     // Sychronous idle

        };

        constexpr static size_t buffer_len = 64;
        constexpr static size_t putback_len = 8;
        constexpr static size_t obuf_len = buffer_len;
        constexpr static size_t ibuf_len = buffer_len + putback_len;
        constexpr static size_t obuf_size = obuf_len * sizeof(CharT);
        constexpr static size_t ibuf_size = ibuf_len * sizeof(CharT);
        constexpr static size_t putback_size = putback_len * sizeof(CharT);

        CharT obuf[obuf_len];
        CharT ibuf[ibuf_len];
        std::basic_streambuf<CharT> &_sb;
        bool outIdle, inIdle, atEnd;

        static CharT leader[2];
        static CharT trailer[2];


    public:
        bool isAtEnd() const { return (this->gptr() == this->egptr() ? atEnd : false); }


        int beginEncoding() {
            if (!outIdle)
                throw EncodingError("Begin must be in idle mode.");

            if (this->sputc(SYN) == traits_type::eof())
                return -1;
            if (this->sputc(STX) == traits_type::eof())
                return -1;
            outIdle = false;
        }


        int endEncoding() {
            if (outIdle)
                throw EncodingError("End must not be in idle mode.");

            outIdle = true;
            if (this->sputc(ETX) == traits_type::eof())
                return -1;
            if (this->sputc(SYN) == traits_type::eof())
                return -1;

            return 0;
        }


    protected:
        int sync() {
            if (realSync() < 0)
                return -1;

            return _sb.pubsync();
        }


        int realSync() {
            std::streamsize n = _sb.sputn(obuf, this->pptr() - obuf);

            if (n < 0) {
                return -1;
            } else if (n == (this->pptr() - obuf)) {
                this->setp(obuf, obuf + obuf_len);
            } else {
                for (std::streamsize cp = n; cp < (obuf_len - n); ++cp) {
                    obuf[cp - n] = obuf[cp];
                }
                this->setp(obuf, obuf + obuf_len);
                this->pbump(static_cast<int>(obuf_len-n));
            }
            return 0;
        }


        int_type overflow( int_type c ) {
            if (realSync() < 0)
                return traits_type::eof();

            if (traits_type::not_eof(c)) {
                char_type cc = traits_type::to_char_type(c);
                this->xsputn(&cc, 1);
            }

            return traits_type::to_int_type(c);
        }


        std::streamsize xsputn(const CharT* s, std::streamsize n) {
            std::streamsize i{};

            if (outIdle)
                throw EncodingError("Can not send data in idle mode.");

            for (i = 0; i < n; ++i) {
                CharT e = traits_type::to_char_type(SO);

                switch (traits_type::to_int_type(s[i])) {
                    case SO:
                        e = traits_type::to_char_type(SI);
                    case SI:
                    case STX:
                    case ETX:
                    case SYN:
                        if (this->pptr() == this->epptr()-1) {
                            this->pptr()[0] = e;
                            this->pbump(1);
                            if (!traits_type::not_eof(overflow(s[i]))) {
                                return -1;
                            }
                        } else {
                            this->pptr()[0] = e;
                            this->pptr()[1] = s[i];
                            this->pbump(2);
                        }
                        break;
                    default:
                        if (this->pptr() == this->epptr()-1) {
                            if (!traits_type::not_eof(overflow(s[i]))) {
                                return -1;
                            }
                        } else {
                            this->pptr()[0] = s[i];
                            this->pbump(1);
                        }
                }
            }
            return i;
        }


    public:
        int beginDecoding() {
            if (!inIdle)
                throw DecodingError("Begin must be in idle mode.");

            int_type c;
            std::streamsize n = _sb.in_avail();
            while ((c = _sb.sbumpc()) >= 0) {
                if (c == STX) {
                    inIdle = false;
                    return c;
                }

                if (c != SYN)
                    throw DecodingError("Data or unexpected protocol in idle mode.");
            }

            atEnd = false;
            return c;
        }


        int endDecoding() {
            if (inIdle)
                throw DecodingError("End must not be in idle mode.");

            if (atEnd) {
                atEnd = false;
                inIdle = true;
                return 0;
            }

            int_type c;
            while ((c = _sb.sgetc()) >= 0) {
                if (c == ETX) {
                    inIdle = true;
                    return c;
                }

                throw DecodingError("Unexpected data or protocol at end.");
            }

            return 0;
        }


    protected:
        int_type underflow( ) {
            std::streamsize n = 0;
            bool escape{false};

            if (atEnd)
                return ETX;

            if (inIdle)
                throw DecodingError("Input can not be in idle mode.");

            CharT *put = ibuf + putback_len;
            CharT *cur = put;
            CharT *end = ibuf + ibuf_len + putback_len;

            while (put != end && !atEnd) {
                CharT c = _sb.sbumpc();
                if (c == traits_type::eof())
                    return traits_type::to_int_type(c);

                if (escape) {
                    *put = c;
                    ++put;
                    escape = false;
                } else {
                    switch (c) {
                        case SO:
                        case SI:
                            escape = true;
                            break;
                        case ETX:
                            atEnd = true;
                            break;
                        case STX:
                        case SYN:
                            throw DecodingError("Unexpected protocol in data.");
                        default:
                            *put = c;
                            ++put;
                    }
                }
            }

            this->setg(ibuf, cur, put);

            return traits_type::to_int_type(*(ibuf + putback_len));
        }

    };

    template <class CharT, class Traits>
    CharT Encoder<CharT,Traits>::leader[2]{ '<', '-' };

    template <class CharT, class Traits>
    CharT Encoder<CharT,Traits>::trailer[2]{ '!', '>' };

}

#endif //PIDP_ENCODER_H
