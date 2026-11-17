#include "common/dpad_shape.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>

#include "common/game_options.h"

namespace dpadshape {

namespace {

using clock = std::chrono::steady_clock;
constexpr int kPorts = 4;
constexpr uint16_t kDpad = 0xF;  // up, down, left, right
constexpr int kBits = 4;

std::atomic<double> g_step{1.0 / 60.0};

struct Button {
    bool down = false;               // the real button
    clock::time_point pressed, next;  // when it went down, when it next repeats
    clock::time_point pulse_end;      // the game sees it until then
    bool repeating = false;
};
Button g_state[kPorts][kBits];

// the left stick on the first pad, as last polled: which way it leans decides a diagonal
std::atomic<int> g_lx{0}, g_ly{0};
struct Stick {
    uint32_t pad = 0;
    Button bit[4];
};
Stick g_stick[kPorts];

}  // namespace

bool& On() {
    static bool on = true;
    return on;
}

bool& StickOn() {
    static bool on = true;
    return on;
}

void FrameStep(double seconds) { g_step.store(seconds, std::memory_order_relaxed); }

void Apply(int port, input::Pad& pad) {
    if (port == 0) {
        g_lx.store(pad.lx, std::memory_order_relaxed);
        g_ly.store(pad.ly, std::memory_order_relaxed);
    }
    if (!On() || port < 0 || port >= kPorts) return;
    const auto now = clock::now();
    // two game ticks, never under 8 ms: the game has to poll it in at least one whole tick
    const auto pulse = std::chrono::duration_cast<clock::duration>(
        std::chrono::duration<double>(std::max(2.0 * g_step.load(std::memory_order_relaxed), 0.008)));
    const double percent = std::clamp<double>(gameopt::RepeatPercent(), gameopt::kRepeatMin, gameopt::kRepeatMax);
    // the game's own 10 and 3 counts at 60 a second, scaled the way sub_001BCA50 scales them
    const auto delay = std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(10.0 / 60.0 * percent / 100.0));
    const auto rate = std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(3.0 / 60.0 * percent / 100.0));
    uint16_t out = pad.buttons & ~kDpad;
    for (int i = 0; i < kBits; i++) {
        Button& b = g_state[port][i];
        const bool held = pad.buttons & (1u << i);
        if (held && !b.down) {
            b.pressed = now;
            b.pulse_end = now + pulse;
            b.next = now + delay;
        } else if (held && now >= b.next) {
            b.pulse_end = now + pulse;
            b.next = now + rate;
        }
        b.down = held;
        if (held && now < b.pulse_end) out |= uint16_t(1u << i);
    }
    pad.buttons = out;
}

uint32_t ShapeStick(uint32_t pad, uint32_t held, uint32_t* started) {
    *started = 0;
    if (!StickOn()) return held;
    // up 0x10000, right 0x20000, down 0x40000, left 0x80000 (read off the trace)
    uint32_t bits = held & kStickBits;
    const bool horizontal = bits & 0xA0000u, vertical = bits & 0x50000u;
    if (horizontal && vertical) {
        const int ax = std::abs(g_lx.load(std::memory_order_relaxed)), ay = std::abs(g_ly.load(std::memory_order_relaxed));
        bits &= (ax >= ay) ? 0xA0000u : 0x50000u;
    }
    const auto now = clock::now();
    const auto pulse = std::chrono::duration_cast<clock::duration>(
        std::chrono::duration<double>(std::max(2.0 * g_step.load(std::memory_order_relaxed), 0.008)));
    const double percent = std::clamp<double>(gameopt::RepeatPercent(), gameopt::kRepeatMin, gameopt::kRepeatMax);
    const auto delay = std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(10.0 / 60.0 * percent / 100.0));
    const auto rate = std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(3.0 / 60.0 * percent / 100.0));
    Stick* s = nullptr;
    for (Stick& c : g_stick)
        if (c.pad == pad) { s = &c; break; }
    if (!s)
        for (Stick& c : g_stick)
            if (!c.pad) { c.pad = pad; s = &c; break; }
    if (!s) return held;
    uint32_t out = held & ~kStickBits;
    for (int i = 0; i < 4; i++) {
        Button& b = s->bit[i];
        const bool down = bits & (0x10000u << i);
        if (down && !b.down) {
            b.pulse_end = now + pulse;
            b.next = now + delay;
            *started |= 0x10000u << i;
        } else if (down && now >= b.next) {
            b.pulse_end = now + pulse;
            b.next = now + rate;
            *started |= 0x10000u << i;
        }
        b.down = down;
        if (down && now < b.pulse_end) out |= 0x10000u << i;
    }
    return out;
}

}  // namespace dpadshape
