#include "common/friend_decay.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <random>
#include <vector>

#include "core/log.h"
#include "kernel/memory.h"

namespace frienddecay {

namespace {

using clock = std::chrono::steady_clock;

// "decay relationships" is the script the game runs for one decay. In memory a script is "BHAV", four bytes, four
// bytes, the 64-byte name field (the name starts 12 bytes in), then a 12-byte header (version 0x8002) and the
// 12-byte nodes: opcode u16, true target u8, false target u8, eight operand bytes. Target 0xFE returns true.
constexpr char kName[] = "decay relationships";
constexpr uint32_t kNameAt = 12, kHeaderAt = 12 + 0x40, kNode0 = kHeaderAt + 12;
constexpr uint32_t kScanLow = 0x80000000u, kScanHigh = 0x84000000u, kChunk = 1u << 20;

struct Script {
    uint32_t at = 0;  // guest address of the "BHAV" magic
    uint8_t was_true = 0, was_false = 0;
    bool stopped = false;
};
std::vector<Script> g_scripts;
uint32_t g_scan = kScanLow;  // the next chunk to look at
clock::time_point g_checked, g_window;
bool g_window_runs = true;  // the decay may run in this window
std::mt19937 g_rng{std::random_device{}()};

uint8_t* P(uint32_t va) { return mem::Base() + va; }

bool Valid(uint32_t at) {
    if (!mem::IsCommitted(at) || !mem::IsCommitted(at + kNode0 + 12)) return false;
    return std::memcmp(P(at), "BHAV", 4) == 0 && std::memcmp(P(at + kNameAt), kName, sizeof kName) == 0 &&
           P(at + kHeaderAt)[0] == 0x02 && P(at + kHeaderAt)[1] == 0x80;
}

void Set(Script& s, bool stop) {
    if (s.stopped == stop) return;
    uint8_t* node = P(s.at + kNode0);
    node[2] = stop ? 0xFE : s.was_true;
    node[3] = stop ? 0xFE : s.was_false;
    s.stopped = stop;
}

// a megabyte of the contiguous memory per tick, looking for the script
void Scan() {
    const uint32_t from = g_scan, to = std::min(from + kChunk, kScanHigh);
    g_scan = to >= kScanHigh ? kScanLow : to;
    for (uint32_t page = from; page < to; page += mem::kPage) {
        if (!mem::IsCommitted(page)) continue;
        const uint8_t* p = P(page);
        for (uint32_t i = 0; i < mem::kPage;) {
            const void* hit = std::memchr(p + i, 'B', mem::kPage - i);
            if (!hit) break;
            i = uint32_t(static_cast<const uint8_t*>(hit) - p);
            const uint32_t at = page + i;
            i++;
            // the header and name may lie over the page's end; Valid() reads them only when committed
            if (!Valid(at)) continue;
            if (std::any_of(g_scripts.begin(), g_scripts.end(), [&](const Script& s) { return s.at == at; })) continue;
            Script s;
            s.at = at;
            s.was_true = P(at + kNode0)[2];
            s.was_false = P(at + kNode0)[3];
            g_scripts.push_back(s);
            LOGR("friend decay: found the decay script at %08X", at);
        }
    }
}

}  // namespace

bool& Stopped() {
    static bool v = false;
    return v;
}

uint32_t& Percent() {
    static uint32_t v = kMax;
    return v;
}

void Tick() {
    const bool active = Stopped() || Percent() < kMax;
    // while the player leaves it at the game's own, only what was changed is put back
    if (!active && g_scripts.empty()) return;
    const auto now = clock::now();
    if (active) Scan();
    // every second: a script the game freed and reused is dropped, and the window is picked again
    if (now - g_checked >= std::chrono::seconds(1)) {
        g_checked = now;
        std::erase_if(g_scripts, [](const Script& s) { return !Valid(s.at); });
    }
    if (now - g_window >= std::chrono::seconds(1)) {
        g_window = now;
        g_window_runs = (g_rng() % 100) < std::clamp(Percent(), kMin, kMax);
    }
    const bool stop = active && (Stopped() || !g_window_runs);
    for (Script& s : g_scripts) Set(s, stop);
    if (!active) g_scripts.clear();
}

}  // namespace frienddecay
