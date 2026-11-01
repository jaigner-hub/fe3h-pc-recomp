# Fire Emblem: Three Houses PC Recomp

Static recompilation of *Fire Emblem: Three Houses* (Switch, ARM64) into a native Windows program. The game's code
is lifted to C, compiled for x86-64 and linked with a host runtime that stands in for the Switch OS and for NVN, its
graphics API, on Vulkan.

The method, the lifter, the runtime and the renderer come from [totk-pc-recomp](https://github.com/valentderah/totk-pc-recomp),
the same kind of port of *Tears of the Kingdom*. This project takes that code wherever it is not specific to TotK,
to find out how much of a Switch recompilation is game independent. [docs/plan.md](docs/plan.md) is the plan and
the record of what was found.

## Status

The game boots to its title screen: logos, the title with "Press any button", and the opening movie starts after
idling there. Nothing past the title has been tried. Loading is about three times slower than on the console.

Version 1.1.1 is the target; it is the latest update available to the author.

## What you need

- Your own dump of the game (XCI or NSP), its 1.1.1 update NSP, and the keys from your own console. This repository
  contains no game content, and the content check (`tools/check_content.py`, run by the pre-commit hook and CI) keeps
  it that way.
- Windows 10 or later, a Vulkan 1.3 GPU, and the toolchain of totk-pc-recomp's `docs/building.md`: MSYS2 UCRT64
  (clang, lld, ninja, SDL3, opus, libvpx, Vulkan), the .NET 10 SDK and Visual Studio Build Tools for the shader
  translator, Python 3.12.

## Building

1. Copy `config.example.toml` to `config.local.toml` and point it at your game, update and keys.
2. `python tools/build_hactool.py`, then `python tools/extract_exefs.py <game> --keys <keys> --output build/extracted/v0/exefs`
   for the base and, with `--update`, for the update; `python tools/extract_romfs.py` merges the update's RomFS.
3. `python recomp/gen.py --exefs build/extracted/v262144/exefs --out generated/v262144 --modules main,subsdk0,subsdk1,sdk --patches recomp/patches-v262144.txt`
4. `python tools/build_xlate.py`, then `python tools/build_game.py` (Windows Python; MSYS2 clang does the compiling).
5. Run `build\v262144\fe3h.exe`, in the foreground: the game pauses when its window loses focus.

## Third-party code

Everything taken from elsewhere is listed with its source, commit and license in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). totk-pc-recomp is MIT, Copyright (c) 2026 Valiantsin Dzerakh; its
own upstreams (LAS Recomp, MacNativeGames, d2r-mobile, nxnative, Ryujinx's shader translator) keep their notices.
