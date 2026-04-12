#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <fstream>
#include <cstdio>

#include "eigenapi.h"
#include "src/replay_harness.h"

// --- binary builder helpers ---

static void writeU32LE(std::vector<uint8_t>& buf, uint32_t v) {
    buf.push_back(v & 0xFF);
    buf.push_back((v >> 8) & 0xFF);
    buf.push_back((v >> 16) & 0xFF);
    buf.push_back((v >> 24) & 0xFF);
}

static void writeU16LE(std::vector<uint8_t>& buf, uint16_t v) {
    buf.push_back(v & 0xFF);
    buf.push_back((v >> 8) & 0xFF);
}

static void writeF32LE(std::vector<uint8_t>& buf, float v) {
    uint32_t u;
    memcpy(&u, &v, 4);
    writeU32LE(buf, u);
}

static void writeU64LE(std::vector<uint8_t>& buf, uint64_t v) {
    buf.push_back(v & 0xFF);
    buf.push_back((v >> 8) & 0xFF);
    buf.push_back((v >> 16) & 0xFF);
    buf.push_back((v >> 24) & 0xFF);
    buf.push_back((v >> 32) & 0xFF);
    buf.push_back((v >> 40) & 0xFF);
    buf.push_back((v >> 48) & 0xFF);
    buf.push_back((v >> 56) & 0xFF);
}

struct TestEvent {
    uint32_t t_ms;
    uint8_t  type;
    uint8_t  course;
    uint8_t  key;
    uint8_t  strip_idx;
    uint8_t  active;
    uint8_t  pedal_idx;
    float    pressure;
    float    roll;
    float    yaw;
    float    value;
    uint64_t t_us;
};

static std::vector<uint8_t> makeCapture(const std::string& deviceType,
                                         const std::vector<TestEvent>& events) {
    std::vector<uint8_t> buf;
    // Header: 36 bytes
    writeU32LE(buf, 0x454C4346u); // magic
    buf.push_back(3);              // schema_version
    char dt[8] = {};
    strncpy(dt, deviceType.c_str(), 8);
    for (int i = 0; i < 8; ++i) buf.push_back(static_cast<uint8_t>(dt[i]));
    // captured_at (int64) + reserved[15] = 23 bytes
    for (int i = 0; i < 23; ++i) buf.push_back(0);

    // Events: 36 bytes each
    for (const auto& ev : events) {
        writeU32LE(buf, ev.t_ms);         // 0
        buf.push_back(ev.type);           // 4
        buf.push_back(ev.course);         // 5
        buf.push_back(ev.key);            // 6
        buf.push_back(ev.strip_idx);      // 7
        buf.push_back(ev.active);         // 8
        buf.push_back(ev.pedal_idx);      // 9
        writeU16LE(buf, 0);               // dev_id 10-11
        writeF32LE(buf, ev.pressure);     // 12
        writeF32LE(buf, ev.roll);         // 16
        writeF32LE(buf, ev.yaw);          // 20
        writeF32LE(buf, ev.value);        // 24
        writeU64LE(buf, ev.t_us);         // 28
    }
    return buf;
}

static std::string writeTempFile(const std::vector<uint8_t>& data) {
    char path[] = "/tmp/elcf_test_XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) return {};
    write(fd, data.data(), data.size());
    ::close(fd);
    return path;
}

// --- counting callback ---

struct CountingCallback : EigenApi::Callback {
    int keys = 0, breaths = 0, strips = 0, buttons = 0, pedals = 0;
    int connected_ = 0, disconnected_ = 0;

    // last values
    unsigned lastCourse = 0, lastKey = 0;
    float lastPressure = 0, lastRoll = 0, lastYaw = 0;
    float lastBreath = 0, lastStrip = 0, lastPedal = 0;
    bool  lastActive = false;
    unsigned lastStripIdx = 0, lastPedalIdx = 0;

    void key(const char*, unsigned long long, unsigned course, unsigned key_,
             bool a, float p, float r, float y) override {
        ++keys;
        lastCourse = course; lastKey = key_; lastActive = a;
        lastPressure = p; lastRoll = r; lastYaw = y;
    }
    void breath(const char*, unsigned long long, float v) override {
        ++breaths; lastBreath = v;
    }
    void strip(const char*, unsigned long long, unsigned idx, float v, bool a) override {
        ++strips; lastStripIdx = idx; lastStrip = v; lastActive = a;
    }
    void button(const char*, unsigned long long, unsigned k, bool a) override {
        ++buttons; lastKey = k; lastActive = a;
    }
    void pedal(const char*, unsigned long long, unsigned idx, float v) override {
        ++pedals; lastPedalIdx = idx; lastPedal = v;
    }
    void connected(const char*, DeviceType) override { ++connected_; }
    void disconnected(const char*) override { ++disconnected_; }
};

// --- tests ---

TEST(ReplayHarness, MissingFile) {
    EigenLite::ReplayHarness h;
    EXPECT_FALSE(h.loadCapture("/tmp/does_not_exist_elcf_xyzzy.elcf"));
    EXPECT_FALSE(h.isLoaded());
    EXPECT_EQ(h.eventCount(), 0);
}

TEST(ReplayHarness, WrongMagic) {
    std::vector<uint8_t> data(32, 0);
    data[0] = 0xDE; data[1] = 0xAD; data[2] = 0xBE; data[3] = 0xEF;
    data[4] = 1;
    auto path = writeTempFile(data);
    EigenLite::ReplayHarness h;
    EXPECT_FALSE(h.loadCapture(path));
    EXPECT_FALSE(h.isLoaded());
    std::remove(path.c_str());
}

TEST(ReplayHarness, WrongSchemaVersion) {
    std::vector<uint8_t> data(36, 0);
    // magic
    data[0] = 0x46; data[1] = 0x43; data[2] = 0x4C; data[3] = 0x45;
    // oops — write correct magic properly
    uint32_t m = 0x454C4346u;
    data[0] = m & 0xFF; data[1] = (m>>8)&0xFF; data[2] = (m>>16)&0xFF; data[3] = (m>>24)&0xFF;
    data[4] = 2; // unsupported version
    auto path = writeTempFile(data);
    EigenLite::ReplayHarness h;
    EXPECT_FALSE(h.loadCapture(path));
    EXPECT_FALSE(h.isLoaded());
    std::remove(path.c_str());
}

TEST(ReplayHarness, TruncatedHeader) {
    std::vector<uint8_t> data(20, 0); // too short for 36-byte header
    auto path = writeTempFile(data);
    EigenLite::ReplayHarness h;
    EXPECT_FALSE(h.loadCapture(path));
    EXPECT_FALSE(h.isLoaded());
    std::remove(path.c_str());
}

TEST(ReplayHarness, ValidHeaderZeroEvents) {
    auto data = makeCapture("TAU", {});
    auto path = writeTempFile(data);
    EigenLite::ReplayHarness h;
    EXPECT_TRUE(h.loadCapture(path));
    EXPECT_TRUE(h.isLoaded());
    EXPECT_EQ(h.eventCount(), 0);
    EXPECT_EQ(h.deviceType(), "TAU");
    std::remove(path.c_str());
}

TEST(ReplayHarness, UnknownEventTypeSkipped) {
    std::vector<TestEvent> evs = {
        {0,   5, 0, 0, 0, 1, 0, 0.f, 0.f, 0.f, 0.f},  // connected
        {10, 99, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f},  // unknown — should skip
        {20,  6, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f},  // disconnected
    };
    auto data = makeCapture("TAU", evs);
    auto path = writeTempFile(data);
    EigenLite::ReplayHarness h;
    ASSERT_TRUE(h.loadCapture(path));
    EXPECT_EQ(h.eventCount(), 3);

    CountingCallback cb;
    h.replaySynchronous(&cb);
    EXPECT_EQ(cb.connected_,    1);
    EXPECT_EQ(cb.disconnected_, 1);
    std::remove(path.c_str());
}

TEST(ReplayHarness, SynchronousDispatch) {
    std::vector<TestEvent> evs = {
        {0,   5, 0, 0, 0, 1, 0, 0.f,  0.f,  0.f,  0.f},   // connected
        {10,  0, 1, 3, 0, 1, 0, 0.8f, 0.1f, 0.2f, 0.f},   // key
        {20,  1, 0, 0, 0, 0, 0, 0.f,  0.f,  0.f,  0.5f},  // breath
        {30,  2, 0, 0, 1, 1, 0, 0.f,  0.f,  0.f,  0.7f},  // strip
        {40,  3, 0, 5, 0, 0, 0, 0.f,  0.f,  0.f,  0.f},   // button
        {50,  4, 0, 0, 0, 0, 2, 0.f,  0.f,  0.f,  0.3f},  // pedal
        {60,  6, 0, 0, 0, 0, 0, 0.f,  0.f,  0.f,  0.f},   // disconnected
    };
    auto data = makeCapture("TAU", evs);
    auto path = writeTempFile(data);
    EigenLite::ReplayHarness h;
    ASSERT_TRUE(h.loadCapture(path));
    EXPECT_EQ(h.eventCount(), 7);

    CountingCallback cb;
    h.replaySynchronous(&cb);

    EXPECT_EQ(cb.connected_,    1);
    EXPECT_EQ(cb.keys,          1);
    EXPECT_EQ(cb.breaths,       1);
    EXPECT_EQ(cb.strips,        1);
    EXPECT_EQ(cb.buttons,       1);
    EXPECT_EQ(cb.pedals,        1);
    EXPECT_EQ(cb.disconnected_, 1);

    EXPECT_EQ(cb.lastCourse,   1u);
    EXPECT_EQ(cb.lastKey,      5u); // last key-like = button key=5
    EXPECT_NEAR(cb.lastBreath, 0.5f, 1e-5f);
    EXPECT_NEAR(cb.lastStrip,  0.7f, 1e-5f);
    EXPECT_EQ(cb.lastStripIdx, 1u);
    EXPECT_NEAR(cb.lastPedal,  0.3f, 1e-5f);
    EXPECT_EQ(cb.lastPedalIdx, 2u);

    std::remove(path.c_str());
}

TEST(ReplayHarness, TimedReplayCallsSleepFn) {
    std::vector<TestEvent> evs = {
        {0,   5, 0, 0, 0, 1, 0, 0.f, 0.f, 0.f, 0.f},  // connected at t=0
        {100, 6, 0, 0, 0, 0, 0, 0.f, 0.f, 0.f, 0.f},  // disconnected at t=100
    };
    auto data = makeCapture("PICO", evs);
    auto path = writeTempFile(data);
    EigenLite::ReplayHarness h;
    ASSERT_TRUE(h.loadCapture(path));

    double totalSleep = 0.0;
    CountingCallback cb;
    h.replayTimed(&cb, [&](double ms) { totalSleep += ms; });

    EXPECT_NEAR(totalSleep, 100.0, 1e-3);
    EXPECT_EQ(cb.connected_,    1);
    EXPECT_EQ(cb.disconnected_, 1);
    std::remove(path.c_str());
}
