//
// Created by richard on 27/01/18.
//

#ifndef PIDP_FILEUTILITIES_H
#define PIDP_FILEUTILITIES_H

#include <iostream>
#include <fstream>
#include <memory>
#include <exception>
#include "Memory.h"

namespace pdp8 {

    class FileFormatError : public std::runtime_error {
    public:
        explicit FileFormatError(const char *what, size_t rp = 0)
                : std::runtime_error(what), readPoint(rp), fileName() {}

        explicit FileFormatError(const std::string && what, size_t rp = 0)
                : std::runtime_error(what), readPoint(rp), fileName() {}

        size_t readPoint;
        std::string fileName;
    };

    template<class Ct, class Tt, size_t Size, typename Base, size_t Width>
    std::basic_istream<Ct, Tt> &operator>>(std::basic_istream<Ct, Tt> &is,
                                           std::shared_ptr<hw_sim::Memory<Size, Base, Width>> &memory) {

        int c, hi = 0;
        size_t readPoint = 0;
        int field = 0;

        auto pc = memory->begin();

        enum FileState {
            FS_HI,
            FS_LO,
            FS_ORIG,
        };

        FileState fileState = FS_HI;

        while ((c = is.get()) != EOF) {
            if (c >= 0200) {
                // Leader for field address
                fileState = FS_HI;
                field = (c & 070) << 9;
            } else if (c != 0377) {
                switch (fileState) {
                    case FS_ORIG:
                        if (c <= 077) {
                            fileState = FS_HI;
                            hi |= c & 077;
                            pc = memory->begin() + field + hi;
                        } else
                            throw FileFormatError("File format error", readPoint);
                        break;
                    case FS_HI:
                        hi = (c & 077) << 6;
                        if (c <= 077) {
                            fileState = FS_LO;
                        } else {
                            if (c >= 0100 && c < 0200) {
                                fileState = FS_ORIG;
                            }
                        }
                        break;
                    case FS_LO:
                        if (c <= 077) {
                            fileState = FS_HI;
                            hi |= c & 077;
                            *pc = hi;
                            pc->setFlag(hw_sim::MemFlagInitialized);
                            ++pc;
                        } else
                            throw FileFormatError("File format error", readPoint);
                    default:
                        break;
                }
            }

            ++readPoint;
        }
        return is;
    };

    template<size_t Size, typename Base, size_t Width>
    void readFileToMemory( const char * fileName, std::shared_ptr<hw_sim::Memory<Size, Base, Width>> &memory) {
        try {
            std::fstream    fs{fileName, std::ios_base::in};

            fs >> memory;
        } catch (FileFormatError &e) {
            e.fileName = fileName;
            throw;
        }
    };

    template<typename Base, size_t Width>
    std::string disassemble(hw_sim::MemoryCell<Base, Width> &m, unsigned long address) {
        std::stringstream ss;
        switch (m() & 07000) {
            case 06000:
                switch (m() & 0770) {
                    case 0000:
                        switch (m() & 07) {
                            case 00:
                                ss << "SKON ";
                                break;
                            case 01:
                                ss << "ION ";
                                break;
                            case 02:
                                ss << "IOF ";
                                break;
                            case 03:
                                ss << "SRQ ";
                                break;
                            case 04:
                                ss << "GTF ";
                                break;
                            case 05:
                                ss << "RTF ";
                                break;
                            case 06:
                                ss << "SGT ";
                                break;
                            case 07:
                                ss << "CAF ";
                                break;
                        }
                        break;
                    default:
                        ss << "IOT "
                           << std::oct
                           << setw(6)
                           << setfill('0')
                           << ((m() & 0770) >> 3)
                           << setw(1)
                           << (m() & 03);
                }
                break;
            case 07000:
                if ((m() & 0400) == 0000) {
                    int opr = m() & 0377;

                    if (opr == 0) {
                        ss << "NOP ";
                    } else {
                        // Seq 1,2
                        switch (opr & 0120) {
                            case 0100:
                                ss << "CLL ";
                                break;
                            case 0020:
                                ss << "CML ";
                                break;
                            case 0120:
                                ss << "STL ";
                                break;
                            default:
                                break;
                        }
                        // Seq 1,2,3
                        switch (opr & 0241) {
                            case 0001:
                                ss << "IAC ";
                                break;
                            case 0040:
                                ss << "CMA ";
                                break;
                            case 0041:
                                ss << "CIA ";
                                break;
                            case 0200:
                                ss << "CLA ";
                                break;
                            case 0240:
                                ss << "STA ";
                                break;
                            default:
                                break;
                        }
                        switch (opr & 016) {
                            case 004:
                                ss << "RAL ";
                                break;
                            case 006:
                                ss << "RTL ";
                                break;
                            case 010:
                                ss << "RAR ";
                                break;
                            case 012:
                                ss << "RTR ";
                                break;
                            default:
                                break;
                        }
                        if (opr & 002) ss << "BSW ";
                    }
                } else if (m() & 0401 == 0400) {
                    int opr = m() & 0376;
                    switch (opr & 0130) {
                        case 010:
                            ss << "SKP ";
                            break;
                        case 020:
                            ss << "SNL ";
                            break;
                        case 030:
                            ss << "SZL ";
                            break;
                        case 040:
                            ss << "SZA ";
                            break;
                        case 050:
                            ss << "SNA ";
                            break;
                        case 0100:
                            ss << "SMA ";
                            break;
                        case 0110:
                            ss << "SPA ";
                            break;
                        default:
                            break;
                    }
                    if (opr & 0200)
                        ss << "CLA ";
                    if (opr & 04)
                        ss << "OSR ";
                    if (opr & 02)
                        ss << "HLT ";
                } else if ((m() & 0401) == 0401) {
                    switch (m()) {
                        case 07621:
                            ss << "CAM ";
                            break;
                        case 07501:
                            ss << "MQA ";
                            break;
                        case 07241:
                            ss << "MQL ";
                            break;
                        case 07521:
                            ss << "SWP ";
                            break;
                    }
                }
                break;
            default:
                switch (m() & 07000) {
                    case 00000:
                        ss << "AND ";
                        break;
                    case 01000:
                        ss << "TAD ";
                        break;
                    case 02000:
                        ss << "ISZ ";
                        break;
                    case 03000:
                        ss << "DCA ";
                        break;
                    case 04000:
                        ss << "JMS ";
                        break;
                    case 05000:
                        ss << "JMP ";
                        break;
                }

                if (m() & 00400) {
                    ss << "I ";
                } else {
                    ss << "  ";
                }

                int a = m() & 0177;
                unsigned long p = address & 07600;
                if (m() & 00200) {
                    a |= p;
                }
                ss << std::oct << setw(4) << setfill('0') << a;
        }

        return ss.str();
    }
}


#endif //PIDP_FILEUTILITIES_H
