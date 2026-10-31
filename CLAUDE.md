@AGENTS.md

# Session guide

## What this project is

A static recompilation of *Fire Emblem: Three Houses* (Switch). The game's ARM64 code is lifted to C, compiled for x86-64, and linked with a host runtime that stands in for the Switch OS and GPU. Players bring their own dump and keys; nothing from the game is ever committed.

The method comes from **totk-pc-recomp**, a working port of *Tears of the Kingdom* built the same way. This project takes that port's code wherever it is not specific to TotK. The longer-term goal is to find out which parts of a Switch recomp are game independent, so a third game costs much less than the second.

## Where to get code from

- `refs/totk-pc-recomp/`: the TotK port at a pinned commit (`tools/fetch_refs.sh`). Cite and attribute it from here.
- `CLAUDE.local.md` (ignored) may name a local working checkout with newer changes; read that one for the latest state, but attribute copies to the commit they came from.
- Its `docs/architecture.md` explains the whole pipeline. Read it first.

What in totk-pc-recomp is expected to carry over, and what is not:

| Part | Where in totk-pc-recomp | Expected here |
|---|---|---|
| ARM64 lifter, NEON lowering, code generator | `recomp/lifter.py`, `simd_c.py`, `gen.py`, `module.py`, `nso.py`, `flags.py`, `regs.py` | Reuse as is |
| Jump table recovery | `recomp/jumptables.py`, `tests/jumptables_check.py` | Reuse; re-validate on this game |
| Module bases | `recomp/layout.py` | Adapt to this game's modules |
| Differential tests against Unicorn | `tests/diff/`, `docs/building.md` "Checks" | Reuse |
| Guest memory, dispatch, loader | `runtime/memory.c`, `dispatch.c`, `loader.c`, `hostfn.c` | Mostly reuse |
| Horizon host: threads, SVCs, IPC, fs, RomFS, time, audio renderer | `runtime/kernel.c`, `svc.c`, `ipc.c`, `fs_host.c`, `romfs.c`, `audren.c`, `ov_*.c` | Reuse; this game's older SDK will differ in places |
| NVN front end and Vulkan back end | `runtime/nvn_*.c`, `vk_*.c`, `tex_*.c`, `docs/maxwell.md` | Reuse if the game uses NVN (Phase 0 decides) |
| Extraction | `tools/extract.py`, `tools/vendor/macnativegames/` | Adapt: different title ids, no TotK hash list |
| Version tables, patches, shader list, ultrawide, frame-rate patches, zstd override | `recomp/patches-v*.txt`, `runtime/frame_rate.c`, `ov_zstd.c`, `tools/shader_list.py`, ... | TotK only. Do not copy |

totk-pc-recomp is MIT (Copyright (c) 2026 Valiantsin Dzerakh). Every copy gets a row in `THIRD_PARTY_NOTICES.md` with the commit, and a header in the file naming the source. Its own upstreams (LASRecomp, MacNativeGames, d2r-mobile, nxnative) keep their attribution too: copy the matching rows from its `THIRD_PARTY_NOTICES.md` when their code comes along.

## Status

Phase 0, the survey (`docs/plan.md`). No port code exists yet. Next steps:

1. The user fills in `config.local.toml` from `config.example.toml`.
2. ~~Extract the ExeFS into `build/`, list the modules and their sizes.~~ Done: `tools/extract_exefs.py` writes `build/extracted/v0/exefs` and `build/extracted/v262144/exefs`; title id `010055D009F78000` confirmed.
3. ~~Find the SDK version string and the imports: NVN or another API, middleware, `nn::*` functions.~~ Done (`tools/survey_imports.py`): SDK 6.5.0, NVN plus the runtime GLSL compiler `glslc`, KTGL engine.
4. ~~Run the TotK lifter over `main` and count unknown instructions and unresolved jump tables.~~ Done: no unsupported instructions; 18 jump tables the resolver misses (see `docs/plan.md`).
5. ~~Compare the `nn::*` imports with what the TotK host implements.~~ First pass done with step 3; see `docs/plan.md`.
6. Write the answers into the table in `docs/plan.md` (facts only, no game data).

## Open questions

- Target update: 1.1.1 (v262144), decided. It is the latest update the user can dump; 1.2.0 (Cindered Shadows) is not available to them, so build around 1.1.1.
- Graphics API: NVN, confirmed. The game also links glslc and compiles GLSL at run time; find out how many shaders go that way and whether the rest ship precompiled.
- Add-on content (Expansion Pass, Cindered Shadows) needs `aoc:u` and an extra RomFS mount, which TotK did not need.
- Game logic may assume 30 fps.

## Environment notes

- The user works in WSL (Linux), where the interpreter is `python3`; the TotK port builds its C on Windows with MSYS2 UCRT64 Clang. Decide the build host for this project when there is C to build.
- Never commit without the content check passing; never bypass the hook.
