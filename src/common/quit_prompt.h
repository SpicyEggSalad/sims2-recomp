#pragma once

#include "core/input.h"

// Quit to desktop from a controller (quit_prompt.cpp): the console had no way out, a couch PC needs one.
namespace quitprompt {

// app::GameInfo::pad, last: any guest thread, each poll. L3, R3 and Start held together open the prompt; while it is
// up this pad reaches the game neutral, A quits and B cancels.
void Pad(int port, input::Pad& pad);

// overlay::Draw, every frame: the render thread, inside the ImGui frame
void Draw();

}  // namespace quitprompt
