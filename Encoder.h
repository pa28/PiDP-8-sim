//
// Created by richard on 02/02/18.
//

#ifndef PIDP_ENCODER_H
#define PIDP_ENCODER_H


#include <streambuf>
#include <cstring>

using namespace std;

namespace util {

    template <class CharT, class Traits = std::char_traits<CharT>>
    class Encoder : public std::basic_streambuf<CharT>
    {
    public:

        typedef CharT char_type;
        typedef Traits traits_type;
        typedef typename Traits::int_type int_type;

        explicit Encoder(std::basic_streambuf<CharT> &sb) : _sb(sb), outIdle{true}, inIdle{true} {
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

        constexpr static size_t buffer_len = 4;
        constexpr static size_t putback_len = 8;
        constexpr static size_t obuf_len = buffer_len;
        constexpr static size_t ibuf_len = buffer_len + putback_len;
        constexpr static size_t obuf_size = obuf_len * sizeof(CharT);
        constexpr static size_t ibuf_size = ibuf_len * sizeof(CharT);
        constexpr static size_t putback_size = putback_len * sizeof(CharT);

        CharT obuf[obuf_len];
        CharT ibuf[ibuf_len];
        std::basic_streambuf<CharT> &_sb;
        bool outIdle, inIdle;

        static CharT leader[2];
        static CharT trailer[2];

        int sync() {
            if (!outIdle) {
                outIdle = true;
                if (this->sputc(ETX) == traits_type::eof())
                    return -1;
                if (this->sputc(SYN) == traits_type::eof())
                    return -1;
            }

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

            if (outIdle) {
                if (this->sputc(SYN) == traits_type::eof())
                    return -1;
                if (this->sputc(STX) == traits_type::eof())
                    return -1;
                outIdle = false;
            }

            outIdle = false;

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

        int_type underflow( ) {
            std::streamsize n = 0;
            bool escape{false};

            while (n == 0) {
                n = _sb.sgetn(ibuf + putback_len + 1, ibuf_len - putback_len - 1);

                if (n == 0) {
                    return traits_type::eof();
                } else if (n < 0) {
                    return n;
                }

                this->setg(ibuf, ibuf + putback_len, ibuf + putback_len + n);

                CharT *put = ibuf + putback_len;
                CharT *cur = put + 1;
                CharT *end = cur + n;

                while (cur != end) {
                    if (inIdle) {
                        inIdle = *cur != STX;
                    } else {
                        if (escape) {
                            *put = *cur;
                            escape = false;
                            ++put;
                        } else {
                            if (*cur == SO || *cur == SI) {
                                escape = true;
                            } else if (*cur == ETX) {
                                inIdle = true;
                            } else if (*cur == STX || *cur == SYN) {
                                return -1;
                            } else {
                                *put = *cur;
                                ++put;
                            }
                        }
                    }
                    ++cur;
                }

                if (escape) {
                    CharT c = _sb.sgetc();
                    if (c == traits_type::eof())
                        return c;
                    *put = c;
                    ++put;
                }

                this->setg(ibuf, ibuf + putback_len, put);

                return traits_type::to_int_type(*(ibuf + putback_len));
            }
        }

    };

    template <class CharT, class Traits>
    CharT Encoder<CharT,Traits>::leader[2]{ '<', '-' };

    template <class CharT, class Traits>
    CharT Encoder<CharT,Traits>::trailer[2]{ '!', '>' };

}

#endif //PIDP_ENCODER_H
