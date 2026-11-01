// FE3H_READY_PROBE=1: logs the engine's texture readiness checks (recomp/patches-v262144.txt, set ready_probe).
// The patched bl at main+0x989284 lands here instead of main+0x98f450, which returns the engine's GPU progress counter
// *(u32 *)(*(obj + 0x10) + 0x48); this does the same read, so the game behaves as without the probe, and logs the
// counter with the value the caller waits for (x19, set at main+0x989280). A diagnostic: not needed to play.
#include <stdint.h>

#include "dispatch.h"
#include "host.h"

#if __has_include("patches.h")
#include "patches.h"
#endif

#ifdef RT_PATCH_PAGE
#define HOST_READY_COUNTER (RT_PATCH_PAGE + 0xff0)

static void host_ready_counter(Cpu *c) {
    static volatile long n;
    static uint32_t last_counter;
    uint64_t obj = c->x[0], inner = LD64(obj + 0x10);
    uint32_t counter = LD32(inner + 0x48);
    uint64_t want = c->x[19];
    c->x[8] = inner;    // as main+0x98f450 leaves it
    c->x[0] = counter;
    long k = __atomic_add_fetch(&n, 1, __ATOMIC_RELAXED);
    static volatile long not_ready;
    if (counter < want) {
        long m = __atomic_add_fetch(&not_ready, 1, __ATOMIC_RELAXED);
        if (m <= 30 || m % 500 == 0)
            rt_log("ready probe: NOT READY #%ld: counter %u waits for %llu, caller lr %#llx (x19 %#llx)", m, counter,
                   (unsigned long long)want, (unsigned long long)c->x[30], (unsigned long long)c->x[19]);
    }
    if (k <= 10 || k % 2000 == 0 || counter != last_counter && k % 50 == 0)
        rt_log("ready probe %ld: counter %u (obj %#llx, +0x10 %#llx), waits for %llu: %s", k, counter,
               (unsigned long long)obj, (unsigned long long)inner, (unsigned long long)want,
               counter >= want ? "ready" : "not ready");
    last_counter = counter;
}

__attribute__((constructor)) static void ready_probe_register(void) {
    rt_register_host(HOST_READY_COUNTER, host_ready_counter);
}
#endif
