// Quit to desktop from a controller. An Xbox title never quits: the player switched the console off. On a PC played
// from the couch with a pad, Alt+F4 is out of reach, so holding kCombo (L3, R3 and Start) for kHoldMs opens a prompt;
// A then closes the game window (WM_CLOSE, the same way out as the window's own close button), B goes back.
//
// The combination is only read here, never taken from the game until the prompt is up: holding both for a second
// and a half is nothing the game asks for. While the prompt is up every polled pad reaches the game neutral, so the
// A that confirms or the B that cancels does nothing in the game behind it, and the prompt only takes A or B once
// the combination is let go, so the hold that opened it cannot answer it. Start and Back alone are avoided: some
// controllers and launchers take that pair for their own overlay or an alt-tab.
#include "common/quit_prompt.h"

#include <atomic>
#include <chrono>

#include <imgui.h>

#include "app/app.h"
#include "core/log.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <SDL3/SDL_events.h>
#endif

namespace quitprompt {

namespace {

// XINPUT_GAMEPAD wButtons: the buttons held together to open the prompt, and what the prompt calls them
constexpr uint16_t kStart = 0x0010, kLeftThumb = 0x0040, kRightThumb = 0x0080;
constexpr uint16_t kCombo = kLeftThumb | kRightThumb | kStart;
constexpr const char* kComboName = "L3, R3 and Start";
constexpr int kA = 0, kB = 1;                        // Pad::analog
constexpr uint8_t kPressed = 0x40;                   // an analog button counts as down past this
constexpr int64_t kHoldMs = 1500;

int64_t NowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

std::atomic<bool> g_open{false};
std::atomic<int> g_port{-1};            // the controller that opened it: only it answers
std::atomic<bool> g_armed{false};       // the combination was let go since the prompt opened
std::atomic<bool> g_draining{false};    // cancelled: the game gets that pad back once B is let go
std::atomic<int64_t> g_held_since{0};   // when the combination went down, 0 while it is not
std::atomic<bool> g_quitting{false};

void Neutral(input::Pad& pad) {
    const bool connected = pad.connected;
    pad = input::Pad{};
    pad.connected = connected;
}

void Quit() {
    if (g_quitting.exchange(true)) return;
    LOGR("quit: to desktop from the controller prompt");
#if defined(_WIN32)
    PostMessageW(static_cast<HWND>(app::Window()), WM_CLOSE, 0, 0);
#else
    SDL_Event e{};
    e.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&e);
#endif
}

}  // namespace

void Pad(int port, input::Pad& pad) {
    const bool combo = (pad.buttons & kCombo) == kCombo;
    const bool mine = port == g_port.load(std::memory_order_relaxed);
    // a cancel's B stays the prompt's until it is let go, or the game behind would take it as its own back
    if (g_draining.load(std::memory_order_relaxed) && mine) {
        if (pad.analog[kB] > kPressed) return Neutral(pad);
        g_draining.store(false, std::memory_order_relaxed);
    }
    if (!g_open.load(std::memory_order_relaxed)) {
        if (!combo) {
            if (mine || g_port.load(std::memory_order_relaxed) < 0) g_held_since.store(0, std::memory_order_relaxed);
            return;
        }
        int64_t now = NowMs(), since = g_held_since.load(std::memory_order_relaxed);
        if (!since || !mine) {
            g_port.store(port, std::memory_order_relaxed);
            g_held_since.store(now, std::memory_order_relaxed);
        } else if (now - since >= kHoldMs) {
            g_armed.store(false, std::memory_order_relaxed);
            g_open.store(true, std::memory_order_relaxed);
        }
        return;
    }
    // the prompt is up: its controller answers, and the game behind it sees nothing from any
    if (mine) {
        if (!(pad.buttons & kCombo)) g_armed.store(true, std::memory_order_relaxed);
        if (g_armed.load(std::memory_order_relaxed)) {
            if (pad.analog[kA] > kPressed) Quit();
            else if (pad.analog[kB] > kPressed) {
                g_draining.store(true, std::memory_order_relaxed);
                g_open.store(false, std::memory_order_relaxed);
                g_held_since.store(0, std::memory_order_relaxed);
            }
        }
    }
    Neutral(pad);
}

void Draw() {
    if (!g_open.load(std::memory_order_relaxed)) return;
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f}, ImGuiCond_Always, {0.5f, 0.5f});
    ImGui::SetNextWindowBgAlpha(0.92f);
    constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav |
                                        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing;
    if (ImGui::Begin("##sims2_quit", nullptr, kFlags)) {
        ImGui::SetWindowFontScale(1.4f);
        ImGui::TextUnformatted("Quit to desktop?");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::Spacing();
        ImGui::TextDisabled("Anything not saved in the game is lost.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        if (g_armed.load(std::memory_order_relaxed)) ImGui::TextUnformatted("A  Quit        B  Back to the game");
        else ImGui::TextDisabled("Let go of %s", kComboName);
    }
    ImGui::End();
}

}  // namespace quitprompt
