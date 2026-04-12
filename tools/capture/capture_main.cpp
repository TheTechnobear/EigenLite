#include <eigenapi.h>
#include "elcf_format.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <signal.h>
#include <string>
#include <thread>
#include <vector>

// ---- binary layout --------------------------------------------------------

static_assert(true,"dummy for clang"); // to handle clang bug with pack
#pragma pack(push, 1)
struct FileHeader {
    uint32_t magic;
    uint8_t  schema_version;
    char     device_type[8];
    int64_t  captured_at;
    uint8_t  reserved[15];
};
static_assert(sizeof(FileHeader) == 36, "FileHeader must be 36 bytes");

struct EventRecord {
    uint32_t t_ms;           // local elapsed ms for playback sync
    uint8_t  event_type;
    uint8_t  course;
    uint8_t  key;
    uint8_t  strip_idx;
    uint8_t  active;
    uint8_t  pedal_idx;
    uint16_t dev_id;
    float    pressure;
    float    roll;
    float    yaw;
    float    value;
    uint64_t t_us;           // device timestamp in microseconds
};
static_assert(sizeof(EventRecord) == 36, "EventRecord must be 36 bytes");
#pragma pack(pop)

// ---- live display state ---------------------------------------------------

struct DisplayState {
    std::mutex   mu;
    std::string  device_type;
    float        breath       = 0.f;
    float        strip_val    = 0.f;
    bool         strip_active = false;
    unsigned     last_course  = 0;
    unsigned     last_key     = 0;
    float        last_p = 0.f, last_r = 0.f, last_y = 0.f;
    bool         last_key_valid = false;
    int          keys_held    = 0;
    int          buttons_held = 0;
    size_t       event_count  = 0;
};

static std::string bar(float val, float lo, float hi, int w = 20) {
    float t = (val - lo) / (hi - lo);
    t = (t < 0.f) ? 0.f : (t > 1.f) ? 1.f : t;
    int n = (int)(t * w);
    return std::string(n, '#') + std::string(w - n, '.');
}

static void displayLoop(DisplayState& ds, const std::atomic<bool>& stop,
                        int durationS,
                        std::chrono::steady_clock::time_point startTime) {
    std::cout << "\033[2J\033[H" << std::flush; // clear screen once

    while (!stop) {
        // snapshot state under lock
        std::string dev;
        float breath, strip_val;
        bool strip_active, last_key_valid;
        unsigned last_course, last_key;
        float last_p, last_r, last_y;
        int keys_held, buttons_held;
        size_t event_count;
        {
            std::lock_guard<std::mutex> lk(ds.mu);
            dev           = ds.device_type;
            breath        = ds.breath;
            strip_val     = ds.strip_val;
            strip_active  = ds.strip_active;
            last_key_valid= ds.last_key_valid;
            last_course   = ds.last_course;
            last_key      = ds.last_key;
            last_p        = ds.last_p;
            last_r        = ds.last_r;
            last_y        = ds.last_y;
            keys_held     = ds.keys_held;
            buttons_held  = ds.buttons_held;
            event_count   = ds.event_count;
        }

        // elapsed / remaining
        auto now = std::chrono::steady_clock::now();
        int elapsed = (int)std::chrono::duration_cast<std::chrono::seconds>(
                          now - startTime).count();
        char timebuf[48];
        if (durationS > 0) {
            int rem = durationS - elapsed;
            if (rem < 0) rem = 0;
            snprintf(timebuf, sizeof(timebuf), "%02d:%02d  (%02d:%02d left)",
                     elapsed/60, elapsed%60, rem/60, rem%60);
        } else {
            snprintf(timebuf, sizeof(timebuf), "%02d:%02d",
                     elapsed/60, elapsed%60);
        }

        char line[128];
        std::cout << "\033[H"; // move cursor to top-left

        snprintf(line, sizeof(line), "EigenLite Capture  [%-5s]  %s\033[K\n",
                 dev.empty() ? "..." : dev.c_str(), timebuf);
        std::cout << line;
        std::cout << "----------------------------------------------------\033[K\n";

        snprintf(line, sizeof(line), "Breath:  [%s] %+.3f\033[K\n",
                 bar(breath, -1.f, 1.f).c_str(), breath);
        std::cout << line;

        snprintf(line, sizeof(line), "Strip:   [%s] %.3f  %s\033[K\n",
                 bar(strip_val, 0.f, 1.f).c_str(), strip_val,
                 strip_active ? "active" : "      ");
        std::cout << line;

        std::cout << "----------------------------------------------------\033[K\n";

        if (last_key_valid) {
            snprintf(line, sizeof(line),
                     "Key:  c=%-2u k=%-3u  p=%+.3f r=%+.3f y=%+.3f\033[K\n",
                     last_course, last_key, last_p, last_r, last_y);
        } else {
            snprintf(line, sizeof(line), "Key:  (none yet)\033[K\n");
        }
        std::cout << line;

        snprintf(line, sizeof(line), "Keys held: %-3d   Buttons held: %-3d\033[K\n",
                 keys_held, buttons_held);
        std::cout << line;

        std::cout << "----------------------------------------------------\033[K\n";

        snprintf(line, sizeof(line), "Events: %zu\033[K\n", event_count);
        std::cout << line;

        std::cout << "----------------------------------------------------\033[K\n";
        std::cout << "Ctrl-C to stop\033[K" << std::flush;

        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    // leave cursor below the display block
    std::cout << "\033[12;0H\n" << std::flush;
}

// ---- capture callback -----------------------------------------------------

static std::atomic<bool> gStop(false);

class CaptureCallback : public EigenApi::Callback {
public:
    explicit CaptureCallback(std::string outPath)
        : outPath_(std::move(outPath)), started_(false) {
        events_.reserve(100000);
    }

    DisplayState& display() { return disp_; }

    void connected(const char*, DeviceType dt) override {
        {
            std::lock_guard<std::mutex> lk(disp_.mu);
            if (disp_.device_type.empty()) {
                switch (dt) {
                    case PICO:  devType_ = disp_.device_type = "PICO";  break;
                    case TAU:   devType_ = disp_.device_type = "TAU";   break;
                    case ALPHA: devType_ = disp_.device_type = "ALPHA"; break;
                }
            }
            ++disp_.event_count;
        }
        EventRecord r{};
        r.t_ms = elapsedMs(); r.event_type = ELCF::ET_CONNECTED; r.t_us = 0;
        events_.push_back(r);
    }

    void disconnected(const char*) override {
        { std::lock_guard<std::mutex> lk(disp_.mu); ++disp_.event_count; }
        EventRecord r{};
        r.t_ms = elapsedMs(); r.event_type = ELCF::ET_DISCONNECTED; r.t_us = 0;
        events_.push_back(r);
    }

    void key(const char*, unsigned long long t, unsigned course, unsigned key,
             bool a, float p, float r, float y) override {
        {
            std::lock_guard<std::mutex> lk(disp_.mu);
            disp_.last_course = course; disp_.last_key = key;
            disp_.last_p = p; disp_.last_r = r; disp_.last_y = y;
            disp_.last_key_valid = true;
            disp_.keys_held = std::max(0, disp_.keys_held + (a ? 1 : -1));
            ++disp_.event_count;
        }
        EventRecord rec{};
        rec.t_ms = elapsedMs(); rec.event_type = ELCF::ET_KEY; rec.t_us = t;
        rec.course = (uint8_t)course; rec.key = (uint8_t)key;
        rec.active = a ? 1 : 0;
        rec.pressure = p; rec.roll = r; rec.yaw = y;
        events_.push_back(rec);
    }

    void button(const char*, unsigned long long t, unsigned key, bool a) override {
        {
            std::lock_guard<std::mutex> lk(disp_.mu);
            disp_.buttons_held = std::max(0, disp_.buttons_held + (a ? 1 : -1));
            ++disp_.event_count;
        }
        EventRecord rec{};
        rec.t_ms = elapsedMs(); rec.event_type = ELCF::ET_BUTTON; rec.t_us = t;
        rec.key = (uint8_t)key; rec.active = a ? 1 : 0;
        events_.push_back(rec);
    }

    void breath(const char*, unsigned long long t, float val) override {
        { std::lock_guard<std::mutex> lk(disp_.mu); disp_.breath = val; ++disp_.event_count; }
        EventRecord rec{};
        rec.t_ms = elapsedMs(); rec.event_type = ELCF::ET_BREATH; rec.t_us = t; rec.value = val;
        events_.push_back(rec);
    }

    void strip(const char*, unsigned long long t, unsigned idx, float val, bool a) override {
        {
            std::lock_guard<std::mutex> lk(disp_.mu);
            disp_.strip_val = val; disp_.strip_active = a; ++disp_.event_count;
        }
        EventRecord rec{};
        rec.t_ms = elapsedMs(); rec.event_type = ELCF::ET_STRIP; rec.t_us = t;
        rec.strip_idx = (uint8_t)idx; rec.active = a ? 1 : 0; rec.value = val;
        events_.push_back(rec);
    }

    void pedal(const char*, unsigned long long t, unsigned pedal, float val) override {
        { std::lock_guard<std::mutex> lk(disp_.mu); ++disp_.event_count; }
        EventRecord rec{};
        rec.t_ms = elapsedMs(); rec.event_type = ELCF::ET_PEDAL; rec.t_us = t;
        rec.pedal_idx = (uint8_t)pedal; rec.value = val;
        events_.push_back(rec);
    }

    void writeBinary() {
        std::ofstream f(outPath_, std::ios::binary);
        if (!f) {
            std::cerr << "error: cannot open output file: " << outPath_ << "\n";
            return;
        }
        FileHeader hdr{};
        hdr.magic          = ELCF::MAGIC;
        hdr.schema_version = ELCF::SCHEMA_VERSION;
        std::strncpy(hdr.device_type, devType_.c_str(), sizeof(hdr.device_type));
        hdr.captured_at    = (int64_t)std::time(nullptr);
        f.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
        if (!events_.empty())
            f.write(reinterpret_cast<const char*>(events_.data()),
                    (std::streamsize)(events_.size() * sizeof(EventRecord)));
       
       
        std::cerr << "wrote " << events_.size() << " events to " << outPath_ << "\n";
    }

private:
    uint32_t elapsedMs() {
        auto now = std::chrono::steady_clock::now();
        if (!started_) { start_ = now; started_ = true; }
        return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(
            now - start_).count();
    }

    std::string outPath_;
    std::string devType_;
    std::vector<EventRecord> events_;
    std::chrono::steady_clock::time_point start_;
    bool started_;
    DisplayState disp_;
};

// ---- signal / poll loop ---------------------------------------------------

static void sigHandler(int) { gStop = true; }

static void pollLoop(EigenApi::Eigenharp* eh) {
    while (!gStop) {
        if (!eh->process())
            std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}

// ---- arg parsing ----------------------------------------------------------

static void usage(const char* prog) {
    std::cerr << "usage: " << prog
              << " [--device TAU|ALPHA|PICO] [--duration-s N] --out <file.elcf>\n";
}

int main(int argc, char** argv) {
    std::string outPath;
    int durationS = 0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--out" && i + 1 < argc) {
            outPath = argv[++i];
        } else if (arg == "--duration-s" && i + 1 < argc) {
            durationS = std::atoi(argv[++i]);
        } else if (arg == "--device" && i + 1 < argc) {
            ++i; // auto-detected from connected()
        } else {
            usage(argv[0]); return 1;
        }
    }
    if (outPath.empty()) { usage(argv[0]); return 1; }

    signal(SIGINT,  sigHandler);
    signal(SIGTERM, sigHandler);

    EigenApi::FWR_Embedded fwr;
    EigenApi::Eigenharp eh(&fwr);
    eh.setPollTime(100);

    CaptureCallback cb(outPath);
    eh.addCallback(&cb);

    if (!eh.start()) {
        std::cerr << "error: failed to start EigenApi\n";
        return 1;
    }

    auto startTime = std::chrono::steady_clock::now();

    std::thread poller(pollLoop, &eh);
    std::thread display(displayLoop, std::ref(cb.display()),
                        std::ref(static_cast<const std::atomic<bool>&>(gStop)),
                        durationS, startTime);

    if (durationS > 0) {
        auto deadline = startTime + std::chrono::seconds(durationS);
        while (!gStop && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        gStop = true;
    }

    poller.join();
    display.join();
    eh.stop();
    cb.writeBinary();
    return 0;
}
