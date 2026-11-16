// Stand-in for totk-pc-recomp runtime/frame_rate.c, which holds the host half of TotK's 60 fps patch set and was not
// taken over. The rest of the runtime still calls its interface (host.h); these definitions keep the game at its own
// frame pacing:
//
// - frame step: no real-time step, so nvn_api_obj.c present_period() follows the game's present interval. The world
//   clock and the 60 Hz tick count (input scripts, profiler, A/B blocks) run from the first present.
//
// The resolution half (below) is not a stand-in: FE3H keeps its screen size in a table of its own, so it is written
// here instead of porting TotK's resolution.c (which grew a HeapDef byml for TotK's fixed heaps).
#include <windows.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cfg.h"
#include "host.h"

// ---------- Frame step

static _Atomic uint64_t g_first_present_ns;   // display time of the first present, 0 before it
static _Atomic uint64_t g_last_present_ns;

void frame_step_init(void) {}

int frame_step_realtime(void) { return 0; }

uint64_t frame_step_cap_ns(void) { return 0; }

double frame_step_limit_hz(void) { return 0; }   // the overlay shows no limit

void frame_step_present(uint64_t display_ns, int interval) {
    (void)interval;
    uint64_t zero = 0;
    atomic_compare_exchange_strong(&g_first_present_ns, &zero, display_ns ? display_ns : 1);
    atomic_store(&g_last_present_ns, display_ns);
}

int64_t frame_step_world_us(void) {
    uint64_t first = atomic_load(&g_first_present_ns), last = atomic_load(&g_last_present_ns);
    return first && last >= first ? (int64_t)((last - first) / 1000) : -1;
}

uint64_t frame_step_ticks(void) {
    int64_t us = frame_step_world_us();
    return us < 0 ? 0 : (uint64_t)us * 60 / 1000000;
}

// ---------- Resolution: FE3H_OUTPUT_RES
//
// FE3H keeps its screen sizes in a table of (width, height) pairs in main's .rodata: (1920,1080) then (1280,720),
// index 0 the console docked, 1 handheld. Two getters read it — main+0x53b720 returns table[*(u32 *)(main+0x1a361e8)].x
// (the width) and main+0x53b740 .y — one setter picks the index (main+0x53b760, called only from main+0x50f600, the
// operation mode), and 97 callers ask the width getter for render targets, viewports, the camera and the HUD. The UI
// scales itself by width / 1920.0f (main+0x53b780, the float right after the getters), so raising the entry raises the
// picture and the UI together. A table write therefore does what TotK's resolution patch set does with code patches and
// a grown HeapDef table, and it needs no recompiled guest code.
//
// The write happens before the guest starts (boot.c rt_boot: after ldr_load, before the modules' inits) and only when
// FE3H_OUTPUT_RES asks for another size, so the default run is untouched. .text and .rodata are sealed read-only by
// the loader (loader.c seal), so the page is unprotected for the write and sealed again after it.
//
// FE3H allocates its render targets from its own pools at run time, so no heap is grown here (res_heap_extra_*): if the
// pools cannot take a bigger picture the game fails on its own allocation and the log shows it, and FE3H_OUTPUT_RES
// goes back to 1920x1080.

static unsigned g_out[2] = {1920, 1080}, g_scene[2] = {1920, 1080};

static int parse_out_res(const char *v, unsigned out[2]) {
    unsigned w = 0, h = 0;
    if (!v || sscanf(v, "%ux%u", &w, &h) != 2) return 0;
    if (w < 1280 || h < 720 || w > 5120 || h > 2880) return 0;
    out[0] = w, out[1] = h;
    return 1;
}

// The (1920,1080),(1280,720) table in main's .rodata; 0 when this build has another layout or another game version.
static uint64_t find_res_table(const RtLoaded *m) {
    for (uint64_t o = (m->text_end + 3) & ~(uint64_t)3; o + 16 <= m->data_off; o += 4) {
        const uint32_t *p = (const uint32_t *)(uintptr_t)(m->base + o);
        if (p[0] == 1920 && p[1] == 1080 && p[2] == 1280 && p[3] == 720) return m->base + o;
    }
    return 0;
}

void res_init(void) {
    unsigned want[2] = {1920, 1080};
    if (!parse_out_res(cfg_get("FE3H_OUTPUT_RES"), want)) want[0] = want[1] = 0;
    if (!want[0]) want[0] = 1920, want[1] = 1080;
    if (want[0] == 1920 && want[1] == 1080) return;   // the game's own size: nothing to write
    const RtLoaded *main = NULL;
    for (int i = 0; i < ldr_count(); i++)
        if (!strcmp(ldr_module(i)->name, "main")) main = ldr_module(i);
    uint64_t t = main ? find_res_table(main) : 0;
    if (!t) {
        rt_log("res: FE3H's screen size table not found in main, staying at 1920x1080 (FE3H_OUTPUT_RES ignored)");
        return;
    }
    unsigned *e = (unsigned *)(uintptr_t)t;   // entry 0 docked, 1 handheld: both take the asked size, so the mode the
                                              // game settles on cannot put it back to 1080p
    unsigned was[2] = {e[0], e[1]}, was_hh[2] = {e[2], e[3]};
    void *pg = (void *)(uintptr_t)(t & ~(uint64_t)0xfff);
    DWORD old = 0;
    if (!VirtualProtect(pg, 0x1000, PAGE_READWRITE, &old))
        return rt_log("res: cannot unseal main's .rodata (error %lu), staying at %ux%u", GetLastError(), was[0], was[1]);
    e[0] = e[2] = want[0], e[1] = e[3] = want[1];
    DWORD ignored = 0;
    VirtualProtect(pg, 0x1000, old, &ignored);
    g_out[0] = g_scene[0] = want[0], g_out[1] = g_scene[1] = want[1];
    rt_log("res: FE3H's screen size %ux%u (handheld %ux%u) -> %ux%u (table main+%#llx); the game's own FSR and the "
           "host upscale follow it", was[0], was[1], was_hh[0], was_hh[1], want[0], want[1],
           (unsigned long long)(t - main->base));
}

uint64_t res_heap_extra_kib(void) { return 0; }

uint64_t res_heap_extra_bytes(void) { return 0; }

int res_heapdef_build(const void *in, size_t n, void **out, size_t *out_n) {
    (void)in, (void)n, (void)out, (void)out_n;
    return 0;   // never reached: romfs.c heapdef_overlay() returns first when res_heap_extra_kib() is 0
}

void res_heapdef_served(void) {}

int res_is_16x9(unsigned w, unsigned h) {
    // Within 1% of 16:9, the test resolution.c uses.
    double r = h ? (double)w / h : 0, want = 16.0 / 9.0;
    return r > want * 0.99 && r < want * 1.01;
}

int res_wants_wide_overlay(const unsigned out[2]) {
    (void)out;
    return 0;   // the sizes above that would need FE3H's wide UI layout are not offered
}

void res_wanted(unsigned out[2]) { out[0] = g_out[0], out[1] = g_out[1]; }

void res_sizes(unsigned out[2], unsigned scene[2]) {
    out[0] = g_out[0], out[1] = g_out[1];
    scene[0] = g_scene[0], scene[1] = g_scene[1];
}
