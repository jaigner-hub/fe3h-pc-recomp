# Plan

## Phase 0: survey

Answer these from the user's dump before writing any port code. Record answers here (facts about the game only, no game data).

| Question | Answer |
|---|---|
| Target update and title ids | Target 1.1.1 (v262144), the latest update the user can obtain; 1.2.0 is not available. Base `010055D009F78000`, update `010055D009F78800` (both read from the dump's CNMT) |
| ExeFS modules and their sizes | `rtld`, `main`, `subsdk0`, `subsdk1`, `sdk`. One more subsdk than TotK. 1.1.1 changes only `main` (and `main.npdm`); the other four are identical to 1.0.0. Loaded segment sizes in 1.1.1 (text / ro / data / bss, bytes): `main` 11,285,104 / 15,511,840 / 849,032 / 7,531,384; `sdk` 5,359,104 / 6,511,340 / 394,296 / 801,736; `subsdk0` 5,038,848 / 2,880,572 / 907,832 / 9,672; `subsdk1` 3,633,424 / 2,464,580 / 202,040 / 1,166,024; `rtld` 6,640 / 3,344 / 640 / 3,456 |
| SDK version string in `main` | NintendoSDK 6.5.0 (`nnSdk-6_5_0-Release` in `sdk`; `main` carries the 6.5.0 tags of gfx, libcurl, libz, NintendoWare Font and Ui2d, and NEX 4.6.2). TotK is built on a much newer SDK, so mangled names and service commands may differ |
| Graphics API: NVN, Vulkan or OpenGL imports | NVN, reached through `nvnBootstrapLoader` as in TotK, plus `nv::InitializeGraphics` and `nv::SetGraphicsAllocator`. Unlike TotK, the game also loads the runtime GLSL compiler: `subsdk0` is `glslc`, and `main` imports `glslcInitialize`, `glslcCompile`, `glslcCompileSpecialized` and related calls, and holds GLSL `#version 430` source fragments. The shaders ship precompiled. Base RomFS 1.0.0 is `DATA0.bin` (index of 31,160 entries: offset, size, compressed size, flag) and `DATA1.bin` (6.9 GB, chunked zlib) plus sound banks and WebM movies. A scan of all 26,350 non-empty entries finds shader code only in 173 KTGL shader packages (`SPKG`, 130 standalone and the rest inside other containers): glslc output with Maxwell code (the `EXIT` encoding is present) and reflection names, no GLSL source except one debug package that also carries its GLSL and NV assembly listing. `main` loads its eight glslc entry points in one place, right after `nvnBootstrapLoader`, and never calls them directly. So the TotK path applies: Maxwell to SPIR-V through the shader translator (`docs/maxwell.md` there) |
| Middleware (audio, video, compression, physics) | Koei Tecmo's KTGL engine inside `main` (no CRI, Havok, Wwise or Bink strings). Audio through `nn::audio` (audio renderer) and `nn::codec` Opus multistream decoders, hardware and software. Video through `subsdk1` = `multimedia` (`nn::movie`, WebM), which links Android `libstagefright`-style classes. NEX 4.6.2 and libcurl for online features |
| `nn::*` functions imported, and how many the TotK host already implements | `main`, `subsdk0` and `subsdk1` import 423 `nn::*` and `movie::*` functions; 80 match a symbol the TotK host replaces (`tools/survey_imports.py`). Most of the rest are lifted SDK code that needs no host work (`nn::os`, `nn::util`, `nn::mem`, and `nn::audio`, which talks to the real `audren:u` service in TotK). Not covered and needing host work: `nn::socket` (28), `nn::nifm` (8), `nn::ssl`, `nn::nsd`, `nn::friends`, `nn::ec` (online features, can be stubbed as offline); `nn::account` network service account calls; hid vibration and six-axis calls and five `GetNpadState` overloads; `nn::codec` Opus decoders; `nn::fs` `MountAddOnContent`, `QueryMountAddOnContentCacheSize`, `EnsureSaveData`, `CommitSaveData`, `GetDirectoryEntryCount`, `SetAllocator`; most `movie::` calls. TotK's overrides target its own SDK version, so even the 80 matches must be checked against 6.5.0 behaviour |
| Lifter run over `main`: unknown instructions, unresolved jump tables | TotK `recomp/gen.py` @ `c944f9f`, unchanged, over all four modules of 1.1.1 in 12 s: `main` 35,020 functions, 186 MB of C; `subsdk0` 11,616, 86 MB; `subsdk1` 13,641, 63 MB; `sdk` 33,586, 94 MB. Unsupported instructions: 0 in every module. Every `rt_udf` trap sits on a padding word, the `e7ffdefe` trap word or data inside `.text`. Jump tables resolved: 1,790 / 1,088 / 342 / 516; all-label fallbacks (computed but unresolved, slower but correct): 11 / 19 / 2 / 15. `tests/jumptables_check.py` against the Unicorn oracle: 0 disagreements in any module. It still fails, for two reasons. 18 tables the oracle finds and the resolver misses (`main` 3, `subsdk0` 1, `subsdk1` 2, `sdk` 12) would go through `rt_dispatch` and stop with "not a function start" if reached. 4 hoisted table bases are not verified (`subsdk1` 1, `sdk` 3), though in 3 of them resolver and oracle agree. Both are Phase 1 work in `recomp/jumptables.py` |
| Add-on content layout (Expansion Pass, Cindered Shadows) | |
| Frame-rate assumptions in game logic | |

## Phase 1: boot to the title screen

Lift all modules, link them on the host, bring up the services the game calls, and draw the first frames.

**Reached (2026-11-01):** `fe3h.exe` boots 1.1.1 to the title screen ("PRESS ANY BUTTON", the throne room with its
light shafts) in about 58 s, and opens the opening movie after idling there. What stood between the first black frames
and the title, in the order found:

- `nvnCommandBufferBindProgram` with a subset of stages: FE3H binds its vertex and fragment programs separately; the
  replay now combines the stages' programs (`vk_exec_draw.c` `bind_program`).
- `nn::account::OpenPreselectedUser` returns a `Result` in SDK 6.5.0; the `bool` of TotK's `TryOpenPreselectedUser`
  read as a failure and the game skipped mounting its save data, so the title never finished loading.
- The back end drained the queued, display-paced present before freeing any memory pool and in `nvnQueueFinish`;
  FE3H's loader makes a pool and finishes the queue per resource, so loading ran at one resource per frame. The drain
  now applies only to a pool holding a window texture (`vk_backend.c`).
- Smaller: the NVN depth mode defaults to NEAR_IS_MINUS_W (FE3H never sets one), `SetCopyImageStride`, the older
  `nvnQueueAcquireTexture`, offline stubs for nifm, socket and the network service account, older `nn::fs` calls.

- After the title: the main menu drew its panes as flat boxes a frame after appearing. FE3H binds two texture pools
  (4,096 and 12,544 descriptors) and two sampler pools whose ids overlap; the host kept one slot table per kind, keyed
  by id alone, so the 3D renderer's registrations replaced the menu's. Texture and sampler ids are now slots of their
  pool (`nvn_api_obj.c` `nvn_tpool_slot`, `vk_exec_draw.c` `pool_handle`); TotK, with one pool of each kind, is unchanged.

- No music, no voices, only sound effects (first noticed in the opening movie of a new game, whose video plays through
  the host VP9 decoder as TotK's do). FE3H decodes its streams with the SDK's software `OpusMultiStreamDecoder`
  (lifted; it never touches `hwopus`) into PCM wave buffers of one renderer voice. Three things in the TotK host's
  audio renderer lost them, all shapes TotK's own audio never produced: (1) the server voice was looked up by a
  record's id field, and this game's SDK 6.5 client leaves the id of every unused record at 0, so each update reset
  voice 0 right after starting it; (2) the channel resources likewise, which zeroed the left channel's send gains;
  the DSP keeps both by record position (`audren.c` `update_voice`, `request_update`). (3) The music submix reaches
  the final mix through a splitter, and the fold order ranked a splitter-routed mix by its (absent) destination mix,
  folding it after the mix it feeds (`audio_mixer.c` `mix_depth`). `FE3H_TRACE=audio` now logs the voices, the mix
  routing with the splitters and the Opus decoder's calls (`ov_fe3h.c`).

- After the opening movie, the game froze a few seconds into the first scene, every time at a different line, with
  the sound looping. The thread dump showed a scheduling deadlock of the host's making: FE3H pins its render thread and
  job workers to core 2 alone (mask 0x4) and lets the main thread run anywhere; placed on core 2 while cores 0 and 1
  were busy, the main thread then drained the job queue in a loop without a syscall, waiting for a worker that had
  given its core up for an NVN call and could get no other. A waiter with a better priority was there, but the main
  thread held a mutex a better thread waited on, so priority inheritance ranked it above every waiter and it was never
  asked to move; and the whole game runs under the `nnMain` observer, which counted as a host call and made the main
  thread uninterruptible anyway. Three changes in `kernel.c` and `hostfn.c`: a thread takes its ideal core, then the
  free core with the fewest threads pinned to it; a holder that can move to a free core gives way to a waiter that can
  use no other, priorities aside; guest code run by an observer (`rt_original`) counts as game code.
  `FE3H_THREAD_DUMP=s<seconds>` dumps by wall-clock time (a stalled game presents no frame), with every thread's host
  stack, the core owners and the present/replay/fence state; `tools/resolve_map.py` turns the host addresses into
  symbols with the linker map.

- The first attack in a battle: fatal, `nvnCommandBufferDrawArraysIndirect` had no implementation (TotK never issued a
  non-indexed indirect draw; the indexed one existed). Added as `OP_DRAW_IND`, replayed with `vkCmdDrawIndirect` from
  the pool buffer like its indexed sibling (`nvn_api_cmd.c`, `vk_exec_draw.c`). The NVN and Vulkan probes
  (`--probe-nvn`, `--probe-vkexec`) run again here: the parts that need TotK's shader census now skip by name.

- A GPU fault in battle (`vkQueueSubmit2` failed with device lost; the driver's fault record says an invalid write at
  address 0), some way into the first battle, with no host warning before it. Pool finalization waited for the GPU with
  `vkQueueWaitIdle`, which covers submitted work only; FE3H's loader threads free a pool per resource while the render
  thread records and submits, so a pool's buffers could be destroyed under a submission being recorded. The
  finalization (and virtual remapping) now hold the submit lock (`vk_sync.c` `vk_sync_exclusive`). Diagnostics:
  `FE3H_CRUMBS=ops` writes a GPU breadcrumb at every pass boundary and before draws and dispatches outside a pass, with
  a line saying what it was (`vk_crumbs.c`); refused on NVIDIA, where a marker or a checkpoint between draws loses
  the device at the first frame with that same signature (measured on an RTX 4070, driver 1.4.351).

Next: loading is still about three times slower than hardware; the lifter passes the differential tests on `main`
(`tests/diff/func.py`, `lift.py`) but `subsdk1` and `sdk` were not sampled, and `sdk`'s libopus runs lifted for every
stream.

## Shared core

As pieces turn out to be identical to totk-pc-recomp's, list them here. They are candidates for a shared Switch recomp library.
