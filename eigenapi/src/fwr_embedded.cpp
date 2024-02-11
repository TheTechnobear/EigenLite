#include <eigenapi.h>

#include "eigenlite_impl.h"

extern unsigned char ___ihx_pico_ihx[];
extern unsigned int ___ihx_pico_ihx_len;

extern unsigned char* ___ihx_bs_mm_fw_0103_ihx[];
extern unsigned int ___ihx_bs_mm_fw_0103_ihx_len;

extern unsigned char* ___ihx_psu_mm_fw_0102_ihx[];
extern unsigned int ___ihx_psu_mm_fw_0102_ihx_len;

namespace EigenApi {
FWR_Embedded::FWR_Embedded() {
}

FWR_Embedded::~FWR_Embedded() {
}

bool FWR_Embedded::open(const char* devicefile, int oFlags, void** fd) {
    std::string filename = devicefile;
    position_ = 0;
    if (filename == PICO_FIRMWARE) {
        *fd = static_cast<void*>(&___ihx_pico_ihx);
        maxLen_ = ___ihx_pico_ihx_len;

    } else if (filename == BASESTATION_FIRMWARE) {
        *fd = static_cast<void*>(&___ihx_bs_mm_fw_0103_ihx);
        maxLen_ = ___ihx_bs_mm_fw_0103_ihx_len;

    } else if (filename == PSU_FIRMWARE) {
        *fd = static_cast<void*>(&___ihx_psu_mm_fw_0102_ihx);
        maxLen_ = ___ihx_psu_mm_fw_0102_ihx_len;

    } else {
        fd = nullptr;
        maxLen_ = 0;
        return false;
    }
    return true;
}

long FWR_Embedded::read(void* fd, void* data, long byteCount) {
    size_t maxRead = maxLen_ - position_;
    size_t readCount = byteCount < maxRead ? byteCount : maxRead;

    if (readCount > 0) {
        void* src = static_cast<char*>(fd) + position_;
        memcpy(data, src, byteCount);
        position_ += byteCount;
    }
    return readCount;
}

void FWR_Embedded::close(void* fd) {
    position_ = 0;
    maxLen_ = 0;
}

bool FWR_Embedded::confirmResources() {
    return true;
}
}  // namespace EigenApi