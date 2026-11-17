#pragma once
#include "core/input.h"

// D-pad taps as the console game saw them. The menus read the D-pad as held as well as pressed, and the console
// ticked 30 times a second, so a tap was held for a tick or two. At a PC frame rate the same tap is held for
// ten or more ticks, and a menu that reads "held" opens a view and closes it again on the one press.
// Each press is passed to the game for two of its ticks, then dropped even while the button stays down; a button
// kept down is repeated from here at the delay and rate the repeat setting gives (gameopt::RepeatPercent).
namespace dpadshape {

bool& On();                       // settings file: dpad_shape; any thread; the overlay sets it
void FrameStep(double seconds);   // the game thread, each tick: how long a game tick really lasts
void Apply(int port, input::Pad& pad);  // every polled pad, after the keyboard has been merged in

// The left stick, shaped the same way but on the game's side. The game turns the stick into its own held bits (the
// four above its D-pad and button bits, 0xF0000), and the analog values stay for the cursor, so only those bits are
// pulsed and the cursor keeps its smooth movement. A diagonal keeps the direction the stick leans further to.
bool& StickOn();                  // settings file: stick_shape
constexpr uint32_t kStickBits = 0xF0000;
// pad: the game's pad object (a key), held: the game's held bits for it. Returns them shaped.
// started: the bits whose pulse began on this call (a press or one of its repeats).
uint32_t ShapeStick(uint32_t pad, uint32_t held, uint32_t* started);

}  // namespace dpadshape
