#pragma once
#include <cstdint>
#include <vector>

#include "app/app.h"

// The game's own developer options (game_options.cpp): skip_startup_movies, enable_all_player_cheats and the rest,
// as the title registers them at boot (src/game/options.cpp), set from the F1 panel and the settings file.
namespace gameopt {

// one the panel offers by name, kept in the settings file under its game name
struct Managed {
    const char* key;    // the game's name for it, and the settings file key
    const char* label;  // what the panel calls it
    const char* note;   // what it does, as far as this port has seen
    bool restart;       // read once at boot: takes effect the next time the game starts
    bool on;            // the setting
    uint8_t* var;       // the game's byte, null until the game registers the option
};

// every on/off option the game registered, the managed ones included
struct Registered {
    const char* name;  // in the game's image
    const char* desc;
    uint8_t* var;
};

// app::GameInfo::options: the managed options as settings file flags, and the repeat delay below
std::vector<app::Option> Options();

// Held D-pad and button repeat (src/game/hooks.cpp sub_001BCA50), as a percentage of the game's own 10/3 counts at
// 60 polls a second: 200 is the console at 30 frames a second. Any thread; the overlay sets it.
constexpr uint32_t kRepeatMin = 100, kRepeatMax = 400;
uint32_t& RepeatPercent();

// guest thread, from the registration hook: the game registered name (type 2 is on/off) for the byte at var
void Register(const char* name, const char* desc, uint32_t type, uint8_t* var);

// the overlay, on the render thread; nothing here allocates
enum class Group { Startup, Cheats };
Managed* Curated(Group g, size_t& count);  // the group's options, in panel order
size_t Count();                            // options the game registered so far
Registered At(size_t i);                   // in registration order, i below Count()
void Set(Managed& m, bool on);             // the setting, and the game's byte once it is known
void Hold();                               // each frame: the managed options that are on stay on

}  // namespace gameopt
