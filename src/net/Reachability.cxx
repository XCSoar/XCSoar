// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Reachability.hxx"

#ifdef HAVE_NET_STATE

#include "UniqueSocketDescriptor.hxx"
#include "IPv4Address.hxx"
#include "time/PeriodClock.hpp"

#include <atomic>
#include <thread>
#include <chrono>
#include <cerrno>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace {

/* Re-check interval for #PollInternetReachable: 30s was too slow when
   #GetNetState still says "connected" after WiFi off; the map icon only
   updated on the next TCP probe. */
static constexpr std::chrono::seconds kPollProbeInterval{4};

std::atomic<bool> cached_reachable{false};
std::atomic<bool> probe_pending{false};
PeriodClock last_probe;

/**
 * Last #GetNetState in #PollInternetReachable (map draw / main; single-thread
 * w.r.t. this function).  When the value changes, the TCP-probe clock is
 * reset so a new probe can run without waiting for #kPollProbeInterval
 * (e.g. UNKNOWN or cached state flipping without a DISCONNECT sample).
 */
static NetState last_poll_link_net_state{NetState::UNKNOWN};

bool
ProbeOnce(uint8_t a, uint8_t b, uint8_t c, uint8_t d) noexcept
{
  UniqueSocketDescriptor sd;
  if (!sd.CreateNonBlock(AF_INET, SOCK_STREAM, IPPROTO_TCP))
    return false;

  IPv4Address addr(a, b, c, d, 443);
  auto sa = SocketAddress(addr);

  int ret = ::connect(sd.Get(), sa.GetAddress(), sa.GetSize());
  if (ret == 0)
    return true;

#ifdef _WIN32
  int err = WSAGetLastError();
  if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS)
    return false;
#else
  if (errno != EINPROGRESS && errno != EWOULDBLOCK)
    return false;
#endif

  constexpr int TIMEOUT_MS = 1500;
  if (sd.WaitWritable(TIMEOUT_MS) <= 0)
    return false;

  return sd.GetError() == 0;
}

bool
ProbeInternet() noexcept
{
  return ProbeOnce(1, 1, 1, 1)       /* Cloudflare */
      || ProbeOnce(8, 8, 8, 8);      /* Google DNS (fallback) */
}

void
ProbeThread() noexcept
{
  if (ProbeInternet()) {
    cached_reachable.store(true, std::memory_order_relaxed);
  } else {
    /* One failed probe is enough: e.g. WiFi radio off must drop the
       map icon on the first check, not after two slow-spaced attempts. */
    cached_reachable.store(false, std::memory_order_relaxed);
  }

  probe_pending.store(false, std::memory_order_release);
}

} // anonymous namespace

bool
IsInternetReachable() noexcept
{
  const NetState ns = GetNetState();
  if (ns == NetState::DISCONNECTED) {
    cached_reachable.store(false, std::memory_order_relaxed);
    return false;
  }

  if (!probe_pending.exchange(true, std::memory_order_acq_rel)) {
    try {
      std::thread(ProbeThread).detach();
    } catch (...) {
      probe_pending.store(false, std::memory_order_release);
    }
  }

  return cached_reachable.load(std::memory_order_relaxed);
}

bool
PollInternetReachable() noexcept
{
  const NetState ns = GetNetState();

  /* Any change in link-level state: allow a new #IsInternetReachable probe
     soon.  Without this, only a DISCONNECTED→!DISCONNECTED path reset the
     clock, so e.g. UNKNOWN or cached state flips could leave a stale
     #cached_reachable for one #kPollProbeInterval. */
  if (ns != last_poll_link_net_state) {
    last_probe.Reset();
    last_poll_link_net_state = ns;
  }

  /* No link at OS level: do not use a stale "reachable" from TCP cache. */
  if (ns == NetState::DISCONNECTED) {
    cached_reachable.store(false, std::memory_order_relaxed);
    return false;
  }

  if (!last_probe.CheckUpdate(kPollProbeInterval))
    return cached_reachable.load(std::memory_order_relaxed);

  return IsInternetReachable();
}

#endif // HAVE_NET_STATE
