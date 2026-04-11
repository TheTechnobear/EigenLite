# Task: Discover Thread Cleanup
<!-- status: active -->
<!-- created: 2026-04-11  refs: docs/known-issues.md#discoverprocessrun-global, docs/roadmap.md -->

## Goal
Move `discoverProcessRun` from a `volatile bool` global to a `std::atomic<bool>` member of `EigenLite`, and add a condition variable so `destroy()` wakes the thread immediately instead of blocking up to 10s.

## Context
`discoverProcessRun` was added in July 2019 (commit 28b4d20) as a quick stop-flag when background USB discovery was first introduced. As a global, it would break if two `EigenLite` instances existed simultaneously (plugin host edge case). More practically, `destroy()` calls `join()` immediately after setting the flag, but the thread may be mid-sleep for up to 10s — making `stop()` silently slow.

## Scope
In: `eigenlite_impl.h`, `eigenapi/src/eigenlite.cpp` — flag, thread lambda, create/destroy
Out: spinlock (`usbDevCheckSpinLock`) protecting `checkUsbDev`, public API, driver code

## Proposals

Two independent improvements. Can do (1) alone or (1)+(2) together.

### 1. Move flag to member (fixes global / multi-instance)

Replace the global `volatile bool discoverProcessRun` with an `std::atomic<bool>` member of `EigenLite`:

```cpp
// eigenlite_impl.h — add member:
std::atomic<bool> discoverRunning_{false};
```

Convert `discoverProcess` free function to a lambda or member function capturing `this`. Replace all references to the global with `discoverRunning_`.

### 2. Condition variable for immediate wakeup (fixes 10s stop() block)

```cpp
// eigenlite_impl.h — add members:
std::mutex discoverMutex_;
std::condition_variable discoverCv_;

// thread body:
while (discoverRunning_.load()) {
    checkUsbDev();
    std::unique_lock<std::mutex> lock(discoverMutex_);
    discoverCv_.wait_for(lock, std::chrono::seconds(10),
                         [this]{ return !discoverRunning_.load(); });
}

// destroy():
discoverRunning_ = false;
discoverCv_.notify_one();
discoverThread_.join();  // returns promptly
```

### Tradeoff summary

| | Global volatile | Atomic member only | + Condition var |
|---|---|---|---|
| Multi-instance safe | no | yes | yes |
| `stop()` latency | up to 10s | up to 10s | immediate |
| Complexity | minimal | minimal | moderate |

## Decisions
<!-- Updated during work -->

## Outcome
<!-- Filled on completion -->
