# Plan

## Phase 0: survey

Answer these from the user's dump before writing any port code. Record answers here (facts about the game only, no game data).

| Question | Answer |
|---|---|
| Target update and title ids | Target 1.1.1 (v262144), the latest update the user can obtain; 1.2.0 is not available. Base `010055D009F78000`, update `010055D009F78800` (both read from the dump's CNMT) |
| ExeFS modules and their sizes | `rtld`, `main`, `subsdk0`, `subsdk1`, `sdk`. One more subsdk than TotK. 1.1.1 changes only `main` (and `main.npdm`); the other four are identical to 1.0.0. Loaded segment sizes in 1.1.1 (text / ro / data / bss, bytes): `main` 11,285,104 / 15,511,840 / 849,032 / 7,531,384; `sdk` 5,359,104 / 6,511,340 / 394,296 / 801,736; `subsdk0` 5,038,848 / 2,880,572 / 907,832 / 9,672; `subsdk1` 3,633,424 / 2,464,580 / 202,040 / 1,166,024; `rtld` 6,640 / 3,344 / 640 / 3,456 |
| SDK version string in `main` | |
| Graphics API: NVN, Vulkan or OpenGL imports | |
| Middleware (audio, video, compression, physics) | |
| `nn::*` functions imported, and how many the TotK host already implements | |
| Lifter run over `main`: unknown instructions, unresolved jump tables | |
| Add-on content layout (Expansion Pass, Cindered Shadows) | |
| Frame-rate assumptions in game logic | |

## Phase 1: boot to the title screen

Lift all modules, link them on the host, bring up the services the game calls, and draw the first frames.

## Shared core

As pieces turn out to be identical to totk-pc-recomp's, list them here. They are candidates for a shared Switch recomp library.
