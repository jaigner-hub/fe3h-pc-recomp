# Plan

## Phase 0: survey

Answer these from the user's dump before writing any port code. Record answers here (facts about the game only, no game data).

| Question | Answer |
|---|---|
| Target update and title ids | Target 1.1.1 (v262144), the latest update the user can obtain; 1.2.0 is not available. Title ids: to confirm from the dump |
| ExeFS modules and their sizes | |
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
