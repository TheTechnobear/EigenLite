#include <eigenapi.h>
#include "eigenlite_impl.h"

namespace EigenApi {
    FWR_Posix::FWR_Posix(const char* fwPath) : path_(fwPath)
    {
    }

    bool FWR_Posix::open(const char* filename, int oFlags, void* *fd)
    {
        std::string filePath = std::string(path_) + std::string(filename);
        std::intptr_t fileDesc = ::open(filePath.c_str(), oFlags);
        *fd = (void*)fileDesc;
        return fileDesc > -1;
    }

    long FWR_Posix::read(void* fd, void *data, long byteCount)
    {
        return ::read((std::intptr_t)fd, data, byteCount);
    }

    void FWR_Posix::close(void* fd)
    {
        ::close((std::intptr_t)fd);
    }

    bool FWR_Posix::confirmResources()
    {
        std::string path = path_;
        int pico = ::open((path + PICO_FIRMWARE).c_str(), O_RDONLY);
        ::close(pico);
        int base = ::open((path + BASESTATION_FIRMWARE).c_str(), O_RDONLY);
        ::close(base);
        int psu = ::open((path + PSU_FIRMWARE).c_str(), O_RDONLY);
        ::close(psu);
        return pico > -1 && base > -1 && psu > -1;
    }
}