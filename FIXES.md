# Fixes for Fire Emblem: Three Houses

Four bugs found while playing FE3H through this runtime, plus the diagnostics that
found them. Everything here was measured on the game's own build (v327680, US) on
a GTX 1060 6 GB under Windows 11, at the game's fixed 30 fps.

The commits apply on top of `47ef991`. Nothing changes unless the settings listed
at the end are set.

## 1. Textures that sample through a format-reinterpreting view came out washed out

**Symptom.** Characters looked grey and flat next to the console original, while
terrain and backgrounds looked right. Raising the upscaler's sharpness did
nothing, and rendering at native scale changed nothing, so the loss was not in
the scaling chain.

**Cause.** `nvnTextureViewSetFormat` was parsed into `nvn_obj.h` (`set_format`,
`format`) but `get_view()` in `vk_exec_draw.c` built every colour view with the
image's own format. Koei Tecmo titles read an sRGB albedo texture that was
created as `BC3_UNORM` through a `BC3_SRGB` view. Without the view's format the
hardware sRGB to linear decode never ran, so those textures sampled about 2.2x
too bright - grey, contrast-free characters. Textures sampled without such a
view kept the right colours, which is why only part of the scene looked wrong.

**Fix.** The view resolves its own format when the image is created
`MUTABLE_FORMAT` (it already is), the view asks for a colour format, and the
format fits the image. If `vkCreateImageView` rejects it anyway, the image's
format is used and the case is logged once.

Three pairs appear in FE3H, all sRGB variants of the image's format:

```
image 37  R8G8B8A8_UNORM    -> NVN 56 -> VK 43  R8G8B8A8_SRGB     (interface art)
image 133 BC1_RGBA_UNORM    -> NVN 70 -> VK 134 BC1_RGBA_SRGB     (albedo)
image 137 BC3_UNORM         -> NVN 72 -> VK 138 BC3_SRGB          (albedo)
```

`FE3H_VIEWFMT` grades the fix: unset or `3` for every image, `1` for
block-compressed images only, `2` for uncompressed 8-bit images only, `0` off.
`1` keeps the albedo correction and leaves the interface paths alone.

## 2. `nvnTexturePoolRegisterImage` aborted the process

**Symptom.** A hard abort after about 40000 frames, in the middle of the
opening:

```
FATAL: NVN nvnTexturePoolRegisterImage is not implemented
```

**Cause.** `nvn_procs.inc` lists the name, but nothing implements it, so it fell
through to the weak stub in `nvn_procs.c`, which calls `rt_fatal`. A comment in
`nvn_api_cmd.c` claimed the game never calls `RegisterImage`; FE3H does, on a
960x540 RGBA8 image behind a fullscreen post-process pass.

**Fix.** Images and textures share one descriptor pool and one id space:
`nvnDeviceGetImageHandle` returns the pool index, the replay resolves a texture
handle and an image handle through the same slot table, and `type_of_handle`
consults the same entries. Registering an image is therefore registering a
texture at that slot, which is what the new `nvnTexturePoolRegisterImage` does.
A null descriptor only logs.

## 3. Recording copies were never retired, until the process ran out of memory

**Symptom.** Memory grew linearly, about 0.45 MB per frame, and the game died
after 20 to 25 minutes:

```
FATAL: nvn: command copy allocation
[host] fatal: memory commit: this process 22271 MB, the system 32629 of 32634 MB
```

**Cause.** Every command buffer recording keeps a host copy of its commands
(`flatten_recording`), and `release_copies()` is called from one place only:
`nvnCommandBufferAddControlMemory`, on the assumption that control memory gets
reused and an address that comes back retires what it held. FE3H's control
memory only moves forward and its `Finalize` never retires a handle, so nothing
was ever freed. The memory trend line showed it plainly:

```
recording copies 3585 -> 7215 -> 10817 -> 14419, released 0
bytes            65 MB -> 362 MB -> 784 MB -> 1166 MB      (one minute apart)
AddControlMemory +1800/min, Finalize 425 total, never growing
```

**Fix.** `retire_old_copies()` sweeps every 256 submissions and frees the copies
whose last submission is more than 7200 submissions old (about two minutes at
30 fps), maintaining `copy_live` and the zombie list. A handle the game still
uses is submitted every frame, so a handle idle for two minutes is dead; copies
recorded but never submitted (`used_at == 0`) are left alone. In play the live
count now oscillates between 4500 and 7000 instead of climbing, and commit stays
between 6 and 8 GB.

## 4. Primitive restart with 16-bit indices was dropped for the whole strip

**Cause.** The restart index was compared as recorded:

```c
if (ex->prim_restart.index == all_ones[itype]) return 1;   // else the strip draws without restart
```

FE3H sets `0xffffffff` and draws 16-bit indices. The hardware compares the low
16 bits, so the value it restarts on is `0xffff` - exactly Vulkan's fixed restart
index. The comparison failed, `restart_for()` returned 0, and the strip was
replayed with restart disabled: `0xffff` was fetched as vertex 65535 and the
triangles around it disappeared.

**Fix.** Compare `index & all_ones[itype]`. This showed up as a map cursor
highlight vanishing intermittently; with the mask in place the count of refused
restarts stays at zero.

## 5. Diagnostics

Off unless asked for, and cheap:

- a memory trend line every 60 s: commit/private/working set, VRAM used and
  budget, live recording copies and bytes, live and released textures, the
  command buffer counters;
- the view format pairs and their counts;
- conditional render and skipped-draw counts, by reason;
- texture slots that resolve to another texture at the same storage, split into
  "by swizzle" (legitimate - interface text) and "by address only" (suspicious).

These are what located 2 and 3 and ruled out the rest; without the trend line
the leak looked like texture growth, and the first guess (ASTC decoding to RGBA8)
was wrong.

## Also in this tree: an unfinished resolution experiment

`res_init()` in `game_patch_stubs.c` rewrites FE3H's own screen size table
(`main+0xcb9f50`, `(1920,1080)(1280,720)`) when `FE3H_OUTPUT_RES` asks for more
than 1920x1080. **It does not work yet.** The table feeds the viewport, camera and
interface maths (about 97 call sites for the width), but not every render target,
so a 2560x1440 request draws a picture that is positioned wrong - the geometry is
laid out for 1440p and lands on a 1080p surface. A second source of the size
looks like the 1920/1080 literal pair in `.text` at `main+0x148690c`. It is off by
default; do not enable it.

## Settings used with these fixes

```ini
[display]
render_scale = 100
sharpness = 35

[performance]
astc_bc = 1              # the GPU has no native ASTC sampling: decode to BC3 instead of RGBA8
tex_vram_pressure = 80   # drop textures earlier, so less spills out of VRAM
viewfmt = 1              # block-compressed views only: keeps the colour fix, leaves interface art alone
```

`astc_bc` matters on any GPU without `textureCompressionASTC_LDR`: FE3H's romfs
holds 12.4 GB of ASTC textures, and decoding them to RGBA8 inflates them 4x to
9x, which alone can exceed 6 GB of VRAM.

## Measured result

30.0 fps mean, 1% low 26 to 27, no aborts over multi-hour sessions, commit
6 to 8 GB, VRAM 2.0 GB of 5.4 GB, and a `pipeline-cache.bin` that removes the one
long first-use stall (12.9 s of driver pipeline compilation on a new scene).
