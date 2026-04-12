#include "replay_harness.h"
#include "elcf_format.h"
#include "../eigenapi.h"
#include <fstream>
#include <thread>
#include <chrono>
#include <cstdint>

namespace EigenLite {

// --- byte-order helpers ---

static uint32_t readU32LE(const uint8_t* buf, int off) {
    return static_cast<uint32_t>(buf[off])
         | (static_cast<uint32_t>(buf[off+1]) << 8)
         | (static_cast<uint32_t>(buf[off+2]) << 16)
         | (static_cast<uint32_t>(buf[off+3]) << 24);
}

static uint16_t readU16LE(const uint8_t* buf, int off) {
    return static_cast<uint16_t>(buf[off])
         | static_cast<uint16_t>(static_cast<uint16_t>(buf[off+1]) << 8);
}

static float readF32LE(const uint8_t* buf, int off) {
    uint32_t u = readU32LE(buf, off);
    float f;
    __builtin_memcpy(&f, &u, 4);
    return f;
}

static uint64_t readU64LE(const uint8_t* buf, int off) {
    return static_cast<uint64_t>(buf[off])
         | (static_cast<uint64_t>(buf[off+1]) << 8)
         | (static_cast<uint64_t>(buf[off+2]) << 16)
         | (static_cast<uint64_t>(buf[off+3]) << 24)
         | (static_cast<uint64_t>(buf[off+4]) << 32)
         | (static_cast<uint64_t>(buf[off+5]) << 40)
         | (static_cast<uint64_t>(buf[off+6]) << 48)
         | (static_cast<uint64_t>(buf[off+7]) << 56);
}

// --- file loading ---

bool ReplayHarness::loadCapture(const std::string& path) {
    loaded_ = false;
    events_.clear();
    deviceType_.clear();

    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    uint8_t header[ELCF::HEADER_SIZE];
    if (!f.read(reinterpret_cast<char*>(header), ELCF::HEADER_SIZE)) return false;

    uint32_t magic = readU32LE(header, ELCF::HDR_MAGIC);
    if (magic != ELCF::MAGIC) return false;

    uint8_t version = header[ELCF::HDR_VERSION];
    if (version != ELCF::SCHEMA_VERSION) return false;

    deviceType_ = std::string(reinterpret_cast<const char*>(header + ELCF::HDR_DEVTYPE), 8);
    auto nul = deviceType_.find('\0');
    if (nul != std::string::npos) deviceType_.erase(nul);

    while (f) {
        uint8_t rec[ELCF::RECORD_SIZE];
        if (!f.read(reinterpret_cast<char*>(rec), ELCF::RECORD_SIZE)) break;

        Event ev;
        ev.t_ms     = readU32LE(rec, ELCF::REC_T_MS);
        ev.type     = rec[ELCF::REC_TYPE];
        ev.course   = rec[ELCF::REC_COURSE];
        ev.key      = rec[ELCF::REC_KEY];
        ev.strip_idx= rec[ELCF::REC_STRIP];
        ev.active   = rec[ELCF::REC_ACTIVE] != 0;
        ev.pedal_idx= rec[ELCF::REC_PEDAL];
        ev.dev_id   = readU16LE(rec, ELCF::REC_DEV_ID);
        ev.pressure = readF32LE(rec, ELCF::REC_PRESSURE);
        ev.roll     = readF32LE(rec, ELCF::REC_ROLL);
        ev.yaw      = readF32LE(rec, ELCF::REC_YAW);
        ev.value    = readF32LE(rec, ELCF::REC_VALUE);
        ev.t_us     = readU64LE(rec, ELCF::REC_T_US);
        events_.push_back(ev);
    }

    loaded_ = true;
    return true;
}

// --- dispatch ---

static EigenApi::Callback::DeviceType resolveDeviceType(const std::string& dt) {
    if (dt == "TAU")   return EigenApi::Callback::TAU;
    if (dt == "ALPHA") return EigenApi::Callback::ALPHA;
    return EigenApi::Callback::PICO;
}

void ReplayHarness::dispatch(const Event& ev, EigenApi::Callback* target) const {
    const char* dev = deviceType_.c_str();
    auto t = static_cast<unsigned long long>(ev.t_ms);
    switch (ev.type) {
        case ELCF::ET_KEY:          target->key(dev, t, ev.course, ev.key, ev.active, ev.pressure, ev.roll, ev.yaw); break;
        case ELCF::ET_BREATH:       target->breath(dev, t, ev.value); break;
        case ELCF::ET_STRIP:        target->strip(dev, t, ev.strip_idx, ev.value, ev.active); break;
        case ELCF::ET_BUTTON:       target->button(dev, t, ev.key, ev.active); break;
        case ELCF::ET_PEDAL:        target->pedal(dev, t, ev.pedal_idx, ev.value); break;
        case ELCF::ET_CONNECTED:    target->connected(dev, resolveDeviceType(deviceType_)); break;
        case ELCF::ET_DISCONNECTED: target->disconnected(dev); break;
        default: break;
    }
}

// --- replay modes ---

void ReplayHarness::replaySynchronous(EigenApi::Callback* target) const {
    for (const auto& ev : events_) {
        dispatch(ev, target);
    }
}

void ReplayHarness::replayTimed(EigenApi::Callback* target,
                                std::function<void(double ms)> sleepFn) const {
    uint32_t prev_t = 0;
    for (const auto& ev : events_) {
        double gap = static_cast<double>(ev.t_ms) - static_cast<double>(prev_t);
        if (gap > 0.0) {
            if (sleepFn) {
                sleepFn(gap);
            } else {
                std::this_thread::sleep_for(
                    std::chrono::microseconds(static_cast<int64_t>(gap * 1000)));
            }
        }
        dispatch(ev, target);
        prev_t = ev.t_ms;
    }
}

// --- accessors ---

int ReplayHarness::eventCount() const { return static_cast<int>(events_.size()); }
std::string ReplayHarness::deviceType() const { return deviceType_; }
bool ReplayHarness::isLoaded() const { return loaded_; }

} // namespace EigenLite
