// The title's developer options. Sims 2 registers about eighty of them at boot in sub_0002A300, each through
// sub_0002CAC0(ecx name, edi description, type, &variable, flag, flag): type 2 is an on/off byte the game reads
// where it needs it. The registration only records where the byte lives and never writes it, so the host can set
// one at any time; the ones read once at boot (skip_startup_movies) are set the moment they are registered, which
// is before the game reads them.
#include "common/game_options.h"
#include "common/dpad_shape.h"

#include <cstring>
#include <iterator>
#include <mutex>

#include "core/log.h"

namespace gameopt {

namespace {

constexpr uint32_t kTypeFlag = 2;

// The panel's list. Notes say what the game's own description says; effects past that are untested on this port.
Managed g_startup[] = {
    {"skip_startup_movies", "Skip intro movies", "The EA and intro movies are not played.", true, false, nullptr},
};
Managed g_cheats[] = {
    {"enable_all_player_cheats", "Cheat gnome, all cheats",
     "The gnome appears with every player cheat on it, no button codes needed: money, motives, skills, unlocks.",
     false, false, nullptr},
    {"god_mode", "God mode",
     "No motive decay, the general cheats on the gnome, and every item and money cheat on.", false, false, nullptr},
    {"enable_unlock_all_lots", "Unlock all lots", "", false, false, nullptr},
    {"enable_unlock_all_lot_shopping_items", "Unlock all shop items", "", false, false, nullptr},
    {"enable_free_items", "Free items", "", false, false, nullptr},
    {"enable_freeplay_available", "Freeplay available", "", false, false, nullptr},
    {"display_motive_values", "Show motive values", "", false, false, nullptr},
    // force_rain is not here: it registers as type 4, a number whose values were not traced, not an on/off byte
};

std::mutex g_lock;  // g_all, and the managed var pointers the guest thread fills in
std::vector<Registered> g_all;

template <typename F>
void EachManaged(F f) {
    for (Managed& m : g_startup) f(m);
    for (Managed& m : g_cheats) f(m);
}

}  // namespace

uint32_t& RepeatPercent() {
    static uint32_t percent = 200;
    return percent;
}

std::vector<app::Option> Options() {
    std::vector<app::Option> out;
    EachManaged([&](Managed& m) { out.push_back({m.key, &m.on, nullptr}); });
    out.push_back({"dpad_repeat_percent", nullptr, &RepeatPercent()});
    out.push_back({"dpad_shape", &dpadshape::On(), nullptr});
    return out;
}

void Register(const char* name, const char* desc, uint32_t type, uint8_t* var) {
    if (type != kTypeFlag || !name || !var) return;
    std::lock_guard lock(g_lock);
    g_all.push_back({name, desc ? desc : "", var});
    EachManaged([&](Managed& m) {
        if (std::strcmp(m.key, name)) return;
        m.var = var;
        // only what the player turned on: an option left off keeps the game's own value
        if (m.on) *var = 1;
        LOGI("game options: %s at %p%s", name, static_cast<void*>(var), m.on ? ", on" : "");
    });
}

Managed* Curated(Group g, size_t& count) {
    if (g == Group::Startup) {
        count = std::size(g_startup);
        return g_startup;
    }
    count = std::size(g_cheats);
    return g_cheats;
}

size_t Count() {
    std::lock_guard lock(g_lock);
    return g_all.size();
}

Registered At(size_t i) {
    std::lock_guard lock(g_lock);
    return g_all[i];
}

void Set(Managed& m, bool on) {
    std::lock_guard lock(g_lock);
    m.on = on;
    if (m.var) *m.var = on ? 1 : 0;
}

void Hold() {
    std::lock_guard lock(g_lock);
    EachManaged([](Managed& m) {
        if (m.on && m.var && !*m.var) *m.var = 1;
    });
}

}  // namespace gameopt
