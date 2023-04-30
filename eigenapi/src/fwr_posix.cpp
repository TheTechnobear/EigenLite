#include <eigenapi.h>
#include "eigenlite_impl.h"

namespace EigenApi {
    FWR_Posix::FWR_Posix(const std::string fwPath)
    {
        this->path = fwPath;
    }

    bool FWR_Posix::open(const std::string filename, int oFlags, std::intptr_t *fd)
    {
        *fd = ::open((path + filename).c_str(), oFlags);
        return *fd > -1;
    }

    ssize_t FWR_Posix::read(std::intptr_t pos, void *data, size_t byteCount)
    {
        return ::read(pos, data, byteCount);
    }

    void FWR_Posix::close(intptr_t fd)
    {
        ::close(fd);
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

    std::intptr_t FWR_Posix::getFD()
    {
        std::intptr_t fd;
        return fd;
    }
}