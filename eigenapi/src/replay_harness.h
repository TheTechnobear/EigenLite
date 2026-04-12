#pragma once
#include <string>
#include <vector>
#include <functional>

namespace EigenApi { class Callback; }

namespace EigenLite {

// ReplayHarness — load and replay .elcf binary captures into any EigenApi::Callback.
//
// ## Timing model
//
// Each event record carries two timestamps:
//
//   t_ms  — wall-clock elapsed milliseconds since the first event, measured by
//            the capture tool's local steady clock (std::chrono::steady_clock).
//            Reflects the timing the *application* experienced: includes poll-loop
//            scheduling, OS jitter, and USB latency on the capture machine.
//
//   t_us  — the Eigenharp device's own hardware timestamp in microseconds,
//            passed through verbatim from EigenApi::Callback's `t` parameter.
//            Reflects the timing the *device* saw: USB packet timestamps from the
//            driver, independent of the host OS scheduler.
//
// These two clocks can diverge:
//   - t_ms grows at wall-clock rate; t_us grows at device-clock rate.
//   - High host load during capture inflates t_ms gaps but leaves t_us unchanged.
//   - t_us may have a different epoch (e.g. device boot or connection time).
//
// replayTimed uses t_ms — it reproduces the session as the application saw it,
// which is appropriate for integration tests that model host-side behaviour.
//
// A future replayTimedUs (not yet implemented) would use t_us deltas to reproduce
// hardware-accurate inter-event gaps — useful for jitter-sensitive tests.
// Implement when a concrete need arises (e.g. EigenHost timing tests).

class ReplayHarness {
public:
    bool loadCapture(const std::string& path);

    // Fire all events synchronously — ignores both t_ms and t_us.
    // target must not be null.
    void replaySynchronous(EigenApi::Callback* target) const;

    // Fire events honouring t_ms (wall-clock) delays.
    // sleepFn(ms) called between events; nullptr uses std::this_thread::sleep_for.
    // See timing model above for t_ms vs t_us trade-offs.
    void replayTimed(EigenApi::Callback* target,
                     std::function<void(double ms)> sleepFn = nullptr) const;

    int eventCount() const;
    std::string deviceType() const;
    bool isLoaded() const;

private:
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
        uint64_t t_us;           // device timestamp in microseconds
    };

    void dispatch(const Event& ev, EigenApi::Callback* target) const;

    std::vector<Event> events_;
    std::string        deviceType_;
    bool               loaded_ = false;
};

} // namespace EigenLite
