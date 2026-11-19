#!/usr/bin/env bash
# Syntax-only compile of the host code that does not need the game: the port's own files in src/common and the SDK
# runtime (kernel, GPU, audio, app). It finds a change that does not build on this platform, which is the thing a
# machine without the game files, such as a CI runner, can know.
#
# It does NOT check the recompiled game (generated/), src/game/, or sdk/runtime/xdk/*: those include
# generated/*/image_symbols.h, which is made from the game's own default.xbe. It also cannot say that anything
# RUNS: no game, no GPU, no audio device. A pass means "compiles", nothing more.
#
#   IMGUI    the Dear ImGui source folder         (required)
#   TOML     the toml++ source folder              (required)
#   SDL_INC  SDL3 include folders, space separated (Linux: the SDL sources and where cmake wrote SDL_revision.h)
#   CXX      the compiler, clang++ by default
set -u
cd "$(dirname "$0")/.."
CXX="${CXX:-clang++}"
: "${IMGUI:?set IMGUI to the Dear ImGui folder}" "${TOML:?set TOML to the toml++ folder}"

INC=(-I src -I sdk/runtime -isystem "$IMGUI" -isystem "$TOML/include")
for d in ${SDL_INC:-}; do INC+=(-isystem "$d"); done

# in no build of this platform, or needing the game's generated headers
SKIP=" main.cpp main_sdl.cpp game_files.cpp system_info.cpp "
case "$(uname -s)" in
    Linux*) SKIP+="d3d12.cpp xaudio2.cpp " ;;                                          # Windows only
    *) SKIP+="vulkan.cpp spirv.cpp sdl_audio.cpp image_decode.cpp " ;;    # Linux only
esac
[ -n "${SDL_INC:-}" ] || SKIP+="input.cpp "

checked=0
failed=0
for f in sdk/runtime/*/*.cpp src/common/*.cpp; do
    case "$f" in sdk/runtime/xdk/*) continue ;; esac
    case "$SKIP" in *" $(basename "$f") "*) continue ;; esac
    checked=$((checked + 1))
    if ! out=$("$CXX" -std=c++20 -fsyntax-only -w "${INC[@]}" "$f" 2>&1); then
        failed=$((failed + 1))
        echo "FAILED  $f"
        echo "$out" | grep -m 5 "error" | sed 's/^/        /'
    fi
done
echo "compile check: $checked files, $failed failed"
[ "$failed" -eq 0 ]
