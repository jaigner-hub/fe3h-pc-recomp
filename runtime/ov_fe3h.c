// SDK overrides that FE3H needs and the TotK host does not have. They live here, apart from the files taken from
// totk-pc-recomp (ov_system.c, ov_input.c, ...), so those stay close to their source; an override that turns out to
// be game independent can move into them later.
//
// Each entry names the FE3H call that needs it. The pattern follows totk-pc-recomp runtime/ov_system.c: the SDK
// function is replaced by symbol (host.h HOST_OVERRIDE), so the service behind it is never opened.
#include <string.h>

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

// ==== nn::nifm: no network. The port plays offline, so every network request ends without a connection and the
// calls that ask about one fail; nifm:u is never opened. FE3H's startup (main+0x5420bc) calls Initialize.
// The failure is a nifm-module result (module 110) standing in for "no connection"; the game only tests for success.
#define NIFM_R_NO_CONNECTION (110u | (1u << 9))

static void ov_nifm_ok(Cpu *c) { ret_x(c, 0); }                       // Result Initialize()
static void ov_nifm_none(Cpu *c) { (void)c; }                         // void Submit...AndWait(), CancelNetworkRequest()
static void ov_nifm_unavailable(Cpu *c) { ret_x(c, 0); }              // bool IsNetworkAvailable()
static void ov_nifm_fail(Cpu *c) { ret_x(c, NIFM_R_NO_CONNECTION); }  // Result Get...(out), HandleNetworkRequestErrorResult()

HOST_OVERRIDE("_ZN2nn4nifm10InitializeEv", ov_nifm_ok);
HOST_OVERRIDE("_ZN2nn4nifm18IsNetworkAvailableEv", ov_nifm_unavailable);
HOST_OVERRIDE("_ZN2nn4nifm27SubmitNetworkRequestAndWaitEv", ov_nifm_none);
HOST_OVERRIDE("_ZN2nn4nifm20CancelNetworkRequestEv", ov_nifm_none);
HOST_OVERRIDE("_ZN2nn4nifm31HandleNetworkRequestErrorResultEv", ov_nifm_fail);
HOST_OVERRIDE("_ZN2nn4nifm26GetCurrentPrimaryIpAddressEPNS_6socket6InAddrE", ov_nifm_fail);
HOST_OVERRIDE("_ZN2nn4nifm26GetCurrentPrimaryIpAddressEP7in_addr", ov_nifm_fail);
HOST_OVERRIDE("_ZN2nn4nifm22GetCurrentProxySettingEPNS0_12ProxySettingE", ov_nifm_fail);

// ==== nn::socket: no network (see nn::nifm above). Initialize succeeds without opening bsd:u; every call that would
// reach a socket fails with ENETDOWN, which GetLastErrno reports. The address helpers (InetHtons, InetNtop, ...) are
// local SDK code and stay lifted. FE3H's startup (main+0x5420dc) calls Initialize after nifm.
#define SOCKET_ENETDOWN 50   // the BSD errno numbering nn::socket uses
#define SOCKET_EAI_FAIL 4    // GetAddrInfo: non-recoverable failure

static void ov_socket_ok(Cpu *c) { ret_x(c, 0); }                          // Result Initialize(config), Finalize()
static void ov_socket_fail(Cpu *c) { ret_x(c, (uint64_t)(int64_t)-1); }    // int / ssize_t calls: -1, errno ENETDOWN
static void ov_socket_errno(Cpu *c) { ret_x(c, SOCKET_ENETDOWN); }         // int GetLastErrno()
static void ov_socket_addrinfo(Cpu *c) {                                   // int GetAddrInfo(node, service, hints, res)
    uint64_t *res = arg_p(c, 3);
    if (res) *res = 0;
    ret_x(c, SOCKET_EAI_FAIL);
}
static void ov_socket_none(Cpu *c) { (void)c; }                            // void FreeAddrInfo(addrinfo *)

HOST_OVERRIDE("_ZN2nn6socket10InitializeERKNS0_6ConfigE", ov_socket_ok);
HOST_OVERRIDE("_ZN2nn6socket8FinalizeEv", ov_socket_ok);
HOST_OVERRIDE("_ZN2nn6socket12GetLastErrnoEv", ov_socket_errno);
HOST_OVERRIDE("_ZN2nn6socket11GetAddrInfoEPKcS2_PK8addrinfoPPS3_", ov_socket_addrinfo);
HOST_OVERRIDE("_ZN2nn6socket12FreeAddrInfoEP8addrinfo", ov_socket_none);
HOST_OVERRIDE("_ZN2nn6socket6SocketEiii", ov_socket_fail);
HOST_OVERRIDE("_ZN2nn6socket5CloseEi", ov_socket_fail);
HOST_OVERRIDE("_ZN2nn6socket4BindEiPK8sockaddrj", ov_socket_fail);
HOST_OVERRIDE("_ZN2nn6socket6ListenEii", ov_socket_fail);
HOST_OVERRIDE("_ZN2nn6socket6AcceptEiP8sockaddrPj", ov_socket_fail);
HOST_OVERRIDE("_ZN2nn6socket7ConnectEiPK8sockaddrj", ov_socket_fail);
HOST_OVERRIDE("_ZN2nn6socket8ShutdownEii", ov_socket_fail);
HOST_OVERRIDE("_ZN2nn6socket4SendEiPKvmi", ov_socket_fail);
HOST_OVERRIDE("_ZN2nn6socket6SendToEiPKvmiPK8sockaddrj", ov_socket_fail);
HOST_OVERRIDE("_ZN2nn6socket4RecvEiPvmi", ov_socket_fail);
HOST_OVERRIDE("_ZN2nn6socket8RecvFromEiPvmiP8sockaddrPj", ov_socket_fail);
HOST_OVERRIDE("_ZN2nn6socket4PollEP6pollfdmi", ov_socket_fail);
HOST_OVERRIDE("_ZN2nn6socket5FcntlEiiz", ov_socket_fail);
HOST_OVERRIDE("_ZN2nn6socket10GetSockOptEiiiPvPj", ov_socket_fail);
HOST_OVERRIDE("_ZN2nn6socket10SetSockOptEiiiPKvj", ov_socket_fail);
HOST_OVERRIDE("_ZN2nn6socket11GetSockNameEiP8sockaddrPj", ov_socket_fail);

// ==== nn::settings::LanguageCode::Make(Language): the SDK asks the set service to turn a Language number into its
// code. The table is the Switch's fixed one (nn::settings::Language order). LanguageCode is char[8], returned in x0.
// FE3H's startup (main+0x4d9ca0) builds the codes for its language selection.
static void ov_language_code_make(Cpu *c) {
    static const char *const codes[] = {"ja", "en-US", "fr", "de", "it", "es", "zh-CN", "ko", "nl", "pt", "ru",
                                        "zh-TW", "en-GB", "fr-CA", "es-419", "zh-Hans", "zh-Hant", "pt-BR"};
    uint64_t n = arg_x(c, 0) & 0xffffffff;
    char code[8] = {0};
    if (n < sizeof codes / sizeof *codes) memcpy(code, codes[n], strlen(codes[n]));
    else rt_log("settings: LanguageCode::Make of unknown language %llu", (unsigned long long)n);
    uint64_t v;
    memcpy(&v, code, 8);
    ret_x(c, v);
}

HOST_OVERRIDE("_ZN2nn8settings12LanguageCode4MakeENS0_8LanguageE", ov_language_code_make);

// ==== nn::account network service account: none, as the port plays offline (see nn::nifm). The local user itself
// comes from ov_system.c (OpenPreselectedUser, GetUserId, GetNickname). Availability reads false; the calls that would
// make or fetch an online identity fail with an account-module result (module 124) standing in for "unavailable".
#define ACCOUNT_R_NSA_UNAVAILABLE (124u | (1u << 9))

static void ov_nsa_available(Cpu *c) {   // Result IsNetworkServiceAccountAvailable(bool *out, const UserHandle &)
    uint8_t *out = arg_p(c, 0);
    if (out) *out = 0;
    ret_x(c, 0);
}
static void ov_nsa_fail(Cpu *c) { ret_x(c, ACCOUNT_R_NSA_UNAVAILABLE); }

HOST_OVERRIDE("_ZN2nn7account32IsNetworkServiceAccountAvailableEPbRKNS0_10UserHandleE", ov_nsa_available);
HOST_OVERRIDE("_ZN2nn7account36EnsureNetworkServiceAccountAvailableERKNS0_10UserHandleE", ov_nsa_fail);
HOST_OVERRIDE("_ZN2nn7account26GetNetworkServiceAccountIdEPNS0_23NetworkServiceAccountIdERKNS0_10UserHandleE", ov_nsa_fail);
HOST_OVERRIDE("_ZN2nn7account37LoadNetworkServiceAccountIdTokenCacheEPmPcmRKNS0_10UserHandleE", ov_nsa_fail);
HOST_OVERRIDE("_ZN2nn7account44EnsureNetworkServiceAccountIdTokenCacheAsyncEPNS0_12AsyncContextERKNS0_10UserHandleE", ov_nsa_fail);
