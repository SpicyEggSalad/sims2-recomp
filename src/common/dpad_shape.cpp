#include "common/dpad_shape.h"

#include <algorithm>
#include <atomic>
#include <chrono>

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

}  // namespace

bool& On() {
    static bool on = true;
    return on;
}

void FrameStep(double seconds) { g_step.store(seconds, std::memory_order_relaxed); }

void Apply(int port, input::Pad& pad) {
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

}  // namespace dpadshape
