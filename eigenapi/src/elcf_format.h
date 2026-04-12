#pragma once
#include <cstdint>

// ELCF — EigenLite Capture File format constants.
// Single source of truth for magic, schema version, record size, field offsets,
// and event type enum. Include this in any code that reads or writes .elcf files.
// See docs/reference/capture-format.md for the full format specification.

namespace ELCF {

// ---- file-level constants -------------------------------------------------

static constexpr uint32_t MAGIC          = 0x454C4346u; // 'ELCF' LE
static constexpr uint8_t  SCHEMA_VERSION = 3;
static constexpr int      HEADER_SIZE    = 36;
static constexpr int      RECORD_SIZE    = 36;

// ---- header field offsets (bytes from start of file) ----------------------

static constexpr int HDR_MAGIC    = 0;   // uint32LE
static constexpr int HDR_VERSION  = 4;   // uint8
static constexpr int HDR_DEVTYPE  = 5;   // char[8], null-padded
static constexpr int HDR_CAPTIME  = 13;  // int64LE — Unix timestamp, seconds

// ---- event record field offsets (bytes from start of record) --------------

static constexpr int REC_T_MS     = 0;   // uint32LE — wall-clock elapsed ms
static constexpr int REC_TYPE     = 4;   // uint8 — EventType
static constexpr int REC_COURSE   = 5;   // uint8
static constexpr int REC_KEY      = 6;   // uint8
static constexpr int REC_STRIP    = 7;   // uint8
static constexpr int REC_ACTIVE   = 8;   // uint8 (0 or 1)
static constexpr int REC_PEDAL    = 9;   // uint8
static constexpr int REC_DEV_ID   = 10;  // uint16LE
static constexpr int REC_PRESSURE = 12;  // float32LE
static constexpr int REC_ROLL     = 16;  // float32LE
static constexpr int REC_YAW      = 20;  // float32LE
static constexpr int REC_VALUE    = 24;  // float32LE
static constexpr int REC_T_US     = 28;  // uint64LE — device timestamp, microseconds

// ---- event type enum ------------------------------------------------------

enum EventType : uint8_t {
    ET_KEY          = 0,
    ET_BREATH       = 1,
    ET_STRIP        = 2,
    ET_BUTTON       = 3,
    ET_PEDAL        = 4,
    ET_CONNECTED    = 5,
    ET_DISCONNECTED = 6
};

inline const char* eventTypeName(uint8_t t) {
    switch (t) {
        case ET_KEY:          return "key";
        case ET_BREATH:       return "breath";
        case ET_STRIP:        return "strip";
        case ET_BUTTON:       return "button";
        case ET_PEDAL:        return "pedal";
        case ET_CONNECTED:    return "connected";
        case ET_DISCONNECTED: return "disconnected";
        default:              return "unknown";
    }
}

} // namespace ELCF
