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
}


#endif //PIDP_FILEUTILITIES_H
