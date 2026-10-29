// The developer option registration, sub_0002CAC0 (flat image, guest base 0x10000), called about eighty times from
// sub_0002A300 at boot:
//
//   ecx  the option's name, e.g. 0x2F4B24 "skip_startup_movies", 0x2F4F1C "enable_all_player_cheats"
//   edi  its description, e.g. 0x2F4B38 "If enabled the intro movies won't be played"
//   esi  the entry it fills; [esp+4] type (2 on/off, stored at entry+0x4C), [esp+8] the variable (entry+0x50),
//        [esp+0xC] and [esp+0x10] two flag bytes (entry+0x55, +0x54); retn 0x10
//
// The body only records these and never writes the variable, so the host can learn every option's address here and
// set it whenever it likes (common/game_options.cpp).
#include "common/game_options.h"
#include "cpu/x86_ops.h"
#include "xdk/hle.h"

X86_FUNC(__imp__sub_0002CAC0);
X86_FUNC(sub_0002CAC0) {
    const uint32_t name = ctx.ecx, desc = ctx.edi;
    const uint32_t type = X86_MEM32(ctx.esp + 0x4u), var = X86_MEM32(ctx.esp + 0x8u);
    __imp__sub_0002CAC0(ctx, base);
    gameopt::Register(name ? reinterpret_cast<const char*>(base + name) : nullptr,
                      desc ? reinterpret_cast<const char*>(base + desc) : nullptr, type,
                      var ? reinterpret_cast<uint8_t*>(base + var) : nullptr);
}
