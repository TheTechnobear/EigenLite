// eigenlite-convert — convert .elcf binary captures to JSON or CSV
// No eigenapi dependency; reads raw bytes only.

#include "elcf_format.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// ---- byte-order reads -----------------------------------------------------

static uint32_t u32le(const uint8_t* b, int o) {
    return (uint32_t)b[o] | ((uint32_t)b[o+1]<<8)
         | ((uint32_t)b[o+2]<<16) | ((uint32_t)b[o+3]<<24);
}
static uint16_t u16le(const uint8_t* b, int o) {
    return (uint16_t)b[o] | (uint16_t)((uint16_t)b[o+1]<<8);
}
static float f32le(const uint8_t* b, int o) {
    uint32_t u = u32le(b, o);
    float f; memcpy(&f, &u, 4); return f;
}

// ---- event record ---------------------------------------------------------

struct Event {
    uint32_t t_ms;
    uint8_t  type;
    uint8_t  course;
    uint8_t  key;
    uint8_t  strip_idx;
    bool     active;
    uint8_t  pedal_idx;
    uint16_t dev_id;
    float    pressure;
    float    roll;
    float    yaw;
    float    value;
    uint64_t t_us;
};

static uint64_t u64le(const uint8_t* b, int o) {
    return (uint64_t)b[o] | ((uint64_t)b[o+1]<<8)
         | ((uint64_t)b[o+2]<<16) | ((uint64_t)b[o+3]<<24)
         | ((uint64_t)b[o+4]<<32) | ((uint64_t)b[o+5]<<40)
         | ((uint64_t)b[o+6]<<48) | ((uint64_t)b[o+7]<<56);
}

static Event parseEvent(const uint8_t* rec) {
    Event ev;
    ev.t_ms      = u32le(rec, ELCF::REC_T_MS);
    ev.type      = rec[ELCF::REC_TYPE];
    ev.course    = rec[ELCF::REC_COURSE];
    ev.key       = rec[ELCF::REC_KEY];
    ev.strip_idx = rec[ELCF::REC_STRIP];
    ev.active    = rec[ELCF::REC_ACTIVE] != 0;
    ev.pedal_idx = rec[ELCF::REC_PEDAL];
    ev.dev_id    = u16le(rec, ELCF::REC_DEV_ID);
    ev.pressure  = f32le(rec, ELCF::REC_PRESSURE);
    ev.roll      = f32le(rec, ELCF::REC_ROLL);
    ev.yaw       = f32le(rec, ELCF::REC_YAW);
    ev.value     = f32le(rec, ELCF::REC_VALUE);
    ev.t_us      = u64le(rec, ELCF::REC_T_US);
    return ev;
}

// ---- output formats -------------------------------------------------------

static void writeCSV(std::ostream& out, const std::string& device_type,
                     const std::vector<Event>& events) {
    out << "t_ms,t_us,event_type,device_type,course,key,strip_idx,active,"
           "pedal_idx,dev_id,pressure,roll,yaw,value\n";
    for (const auto& ev : events) {
        out << ev.t_ms << ','
            << ev.t_us << ','
            << ELCF::eventTypeName(ev.type) << ','
            << device_type << ','
            << (unsigned)ev.course << ','
            << (unsigned)ev.key << ','
            << (unsigned)ev.strip_idx << ','
            << (ev.active ? 1 : 0) << ','
            << (unsigned)ev.pedal_idx << ','
            << ev.dev_id << ','
            << ev.pressure << ','
            << ev.roll << ','
            << ev.yaw << ','
            << ev.value << '\n';
    }
}

// minimal JSON float: avoid scientific notation for small values
static void writeF(std::ostream& out, float v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6g", v);
    out << buf;
}

static void writeJSON(std::ostream& out, const std::string& device_type,
                      const std::vector<Event>& events) {
    out << "{\n";
    out << "  \"device_type\": \"" << device_type << "\",\n";
    out << "  \"event_count\": " << events.size() << ",\n";
    out << "  \"events\": [\n";
    for (size_t i = 0; i < events.size(); ++i) {
        const auto& ev = events[i];
        out << "    {"
            << "\"t_ms\":" << ev.t_ms
            << ",\"type\":\"" << ELCF::eventTypeName(ev.type) << "\""
            << ",\"course\":" << (unsigned)ev.course
            << ",\"key\":" << (unsigned)ev.key
            << ",\"strip_idx\":" << (unsigned)ev.strip_idx
            << ",\"active\":" << (ev.active ? "true" : "false")
            << ",\"pedal_idx\":" << (unsigned)ev.pedal_idx
            << ",\"pressure\":"; writeF(out, ev.pressure);
        out << ",\"roll\":"; writeF(out, ev.roll);
        out << ",\"yaw\":"; writeF(out, ev.yaw);
        out << ",\"value\":"; writeF(out, ev.value);
        out << "}";
        if (i + 1 < events.size()) out << ',';
        out << '\n';
    }
    out << "  ]\n}\n";
}

// ---- main -----------------------------------------------------------------

static void usage(const char* prog) {
    std::cerr << "usage: " << prog
              << " --input <file.elcf> [--format json|csv] [--out <file>]\n"
              << "  format defaults to csv; --out defaults to stdout\n";
}

int main(int argc, char** argv) {
    std::string inPath, outPath, format = "csv";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--input" && i + 1 < argc)       inPath  = argv[++i];
        else if (arg == "--format" && i + 1 < argc)  format  = argv[++i];
        else if (arg == "--out" && i + 1 < argc)     outPath = argv[++i];
        else { usage(argv[0]); return 1; }
    }
    if (inPath.empty()) { usage(argv[0]); return 1; }
    if (format != "json" && format != "csv") {
        std::cerr << "error: format must be json or csv\n"; return 1;
    }

    std::ifstream f(inPath, std::ios::binary);
    if (!f) { std::cerr << "error: cannot open " << inPath << "\n"; return 1; }

    // header
    uint8_t hdr[ELCF::HEADER_SIZE];
    if (!f.read(reinterpret_cast<char*>(hdr), ELCF::HEADER_SIZE)) {
        std::cerr << "error: truncated header\n"; return 1;
    }
    uint32_t magic = u32le(hdr, ELCF::HDR_MAGIC);
    if (magic != ELCF::MAGIC) {
        std::cerr << "error: bad magic (not an ELCF file)\n"; return 1;
    }
    if (hdr[ELCF::HDR_VERSION] != ELCF::SCHEMA_VERSION) {
        std::cerr << "error: unsupported schema version "
                  << (int)hdr[ELCF::HDR_VERSION]
                  << " (expected " << (int)ELCF::SCHEMA_VERSION << ")\n"; return 1;
    }
    std::string device_type(reinterpret_cast<const char*>(hdr + ELCF::HDR_DEVTYPE), 8);
    auto nul = device_type.find('\0');
    if (nul != std::string::npos) device_type.erase(nul);

    // events
    std::vector<Event> events;
    while (f) {
        uint8_t rec[ELCF::RECORD_SIZE];
        if (!f.read(reinterpret_cast<char*>(rec), ELCF::RECORD_SIZE)) break;
        events.push_back(parseEvent(rec));
    }

    // output
    std::ofstream outFile;
    std::ostream* out = &std::cout;
    if (!outPath.empty()) {
        outFile.open(outPath);
        if (!outFile) { std::cerr << "error: cannot open " << outPath << "\n"; return 1; }
        out = &outFile;
    }

    if (format == "csv") writeCSV(*out, device_type, events);
    else                 writeJSON(*out, device_type, events);

    std::cerr << "converted " << events.size() << " events (" << format << ")\n";
    return 0;
}
