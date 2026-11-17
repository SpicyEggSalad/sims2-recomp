#pragma once
#include <cstdint>

// Friendship decay. The game's friendships fall when a Sim is not talked to; the work is done by a behavior script
// of the game's data, "decay relationships" (a BHAV, the same node scripts the PC game runs), which the game loads
// into memory. A script's first node is changed so that it ends at once with a "true" result, which stops the decay,
// and put back when the player wants it to run. Only the copy in memory is touched, never the game's files.
//
// Speed is done by letting the decay run on only some of the time: the window is picked at random each second, so
// 50 % lets half the game's decay days through.
namespace frienddecay {

bool& Stopped();      // settings file: friend_decay_stop; any thread; the overlay sets it
uint32_t& Percent();  // settings file: friend_decay_percent, 10..100 of the game's own speed
constexpr uint32_t kMin = 10, kMax = 100;

// The game thread, once a tick: finds the script as the game loads it and sets it as the settings say.
void Tick();

}  // namespace frienddecay
