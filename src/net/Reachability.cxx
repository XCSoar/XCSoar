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

std::atomic<bool> cached_reachable{false};
std::atomic<bool> probe_pending{false};
unsigned fail_count{0};
PeriodClock last_probe;

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
    fail_count = 0;
    cached_reachable.store(true, std::memory_order_relaxed);
  } else {
    /* require two consecutive failures before reporting unreachable */
    if (++fail_count >= 2)
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
  if (!last_probe.CheckUpdate(std::chrono::seconds(30)))
    return cached_reachable.load(std::memory_order_relaxed);

  return IsInternetReachable();
}

#endif // HAVE_NET_STATE
