// SDK overrides that FE3H needs and the TotK host does not have. They live here, apart from the files taken from
// totk-pc-recomp (ov_system.c, ov_input.c, ...), so those stay close to their source; an override that turns out to
// be game independent can move into them later.
//
// Each entry names the FE3H call that needs it. The pattern follows totk-pc-recomp runtime/ov_system.c: the SDK
// function is replaced by symbol (host.h HOST_OVERRIDE), so the service behind it is never opened.
#include "host.h"

// ==== nn::prepo: play reports stay local, as in ov_system.c, which already replaces PlayReport::Save.
// nnMain (main+0x4db820) calls nn::prepo::Initialize, which would connect to prepo:u. Building a report (PlayReport
// constructors, SetBuffer, SetEventId, Add) is local SDK code and runs lifted.
static void ov_prepo_initialize(Cpu *c) { (void)c; }

HOST_OVERRIDE("_ZN2nn5prepo10InitializeEv", ov_prepo_initialize);

// ==== nn::aoc: no add-on content yet, matching ov_system.c's Initialize and ListAddOnContent (which lists none).
// FE3H's startup (main+0x9a2a4c) counts the add-on content first; lifted, CountAddOnContent reads the aoc:u session
// that the overridden Initialize never opened.
static void ov_aoc_count(Cpu *c) { ret_x(c, 0); }

HOST_OVERRIDE("_ZN2nn3aoc17CountAddOnContentEv", ov_aoc_count);
