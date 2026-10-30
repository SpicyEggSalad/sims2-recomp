#include "common/input_trace.h"

#include <atomic>
#include <cstdint>
#include <cstdlib>

#include "core/log.h"

namespace inputtrace {

namespace {

constexpr int kPorts = 4;
constexpr int16_t kStickOn = 16384;  // a deflection the game's menus would take as a direction

std::atomic<uint32_t> g_dpad[kPorts], g_stick[kPorts];
std::atomic<uint32_t> g_polls[kPorts];  // polls since the last change, so a held button is not a silence

uint32_t Stick(const input::Pad& p) {
    return (p.lx < -kStickOn ? 1u : 0u) | (p.lx > kStickOn ? 2u : 0u) | (p.ly > kStickOn ? 4u : 0u) |
           (p.ly < -kStickOn ? 8u : 0u);
}

}  // namespace

bool On() {
    static const bool on = std::getenv("XBE_INPUT_TRACE") != nullptr;
    return on;
}

void Pad(int port, const input::Pad& pad) {
    if (!On() || port < 0 || port >= kPorts) return;
    const uint32_t dpad = pad.buttons & 0xF, stick = Stick(pad);
    const uint32_t polls = g_polls[port].fetch_add(1, std::memory_order_relaxed) + 1;
    const uint32_t was_dpad = g_dpad[port].exchange(dpad, std::memory_order_relaxed);
    const uint32_t was_stick = g_stick[port].exchange(stick, std::memory_order_relaxed);
    if (dpad == was_dpad && stick == was_stick) return;
    // masks: D-pad 1 up, 2 down, 4 left, 8 right; stick 1 left, 2 right, 4 up, 8 down
    LOGR("input: port %d dpad %X -> %X, stick %X -> %X, after %u polls", port, was_dpad, dpad, was_stick, stick, polls);
    g_polls[port].store(0, std::memory_order_relaxed);
}

}  // namespace inputtrace
