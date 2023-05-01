#include <eigenapi.h>
#include "eigenlite_impl.h"

namespace EigenApi {
    FWR_Posix::FWR_Posix(const std::string fwPath)
    {
        this->path = fwPath;
    }

    bool FWR_Posix::open(const std::string filename, int oFlags, void* *fd)
    {
        std::intptr_t fileDesc = ::open((path + filename).c_str(), oFlags);
        *fd = (void*)fileDesc;
        return fileDesc > -1;
    }

    ssize_t FWR_Posix::read(void* fd, void *data, size_t byteCount)
    {
        return ::read((std::intptr_t)fd, data, byteCount);
    }

    void FWR_Posix::close(void* fd)
    {
        ::close((std::intptr_t)fd);
    }

    void FWR_Posix::setPath(const std::string path)
    {
        this->path = path;
    }

    std::string FWR_Posix::getPath()
    {
        return path;
    }

    bool FWR_Posix::confirmResources()
    {
        int pico = ::open((path + PICO_FIRMWARE).c_str(), O_RDONLY);
        ::close(pico);
        int base = ::open((path + BASESTATION_FIRMWARE).c_str(), O_RDONLY);
        ::close(base);
        int psu = ::open((path + PSU_FIRMWARE).c_str(), O_RDONLY);
        ::close(psu);
        return pico > -1 && base > -1 && psu > -1;
    }
}