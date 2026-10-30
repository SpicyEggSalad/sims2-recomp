#pragma once

#include "core/input.h"

// Diagnostic for doubled menu inputs (input_trace.cpp): with XBE_INPUT_TRACE set, every change of a controller's
// D-pad or left stick is written to the log with its time, next to the game-side lines hooks.cpp writes from the
// pad poll, so one physical press can be followed from the device to the menu that counted it twice.
namespace inputtrace {

bool On();

// app::GameInfo::pad, before the quit prompt: any guest thread, each poll
void Pad(int port, const input::Pad& pad);

}  // namespace inputtrace
