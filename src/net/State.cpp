// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "State.hpp"

#ifdef ANDROID
#define NO_SCREEN
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#include "thread/Debug.hpp"

#include <cassert>

NetState
GetNetState()
{
  assert(InMainThread());

  return native_view != nullptr
    ? NetState(native_view->GetNetState(Java::GetEnv()))
    : NetState::UNKNOWN;
}

#else // non-Android platforms

#if defined(HAVE_NET_STATE)
#include <atomic>
#include <mutex>
#include <chrono>
#include "time/PeriodClock.hpp"

#if defined(__linux__)
#include <cstring>
#include <cstdio>
#include <dirent.h>
#include "system/FileUtil.hpp"
#  if defined(HAVE_NET_STATE_NM_DBUS)
#  include "StateNMDbus.hpp"
#  endif
#  if defined(HAVE_NET_STATE_CONNMAN_DBUS)
#  include "StateConnmanDbus.hpp"
#  endif
#  if defined(HAVE_NET_STATE_NM_DBUS) || \
    defined(HAVE_NET_STATE_CONNMAN_DBUS)
#  include <optional>
#  endif
#endif

#if defined(__APPLE__)
#include <TargetConditionals.h>
#include <Network/Network.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#include <wininet.h>
#endif

namespace {

static std::mutex net_state_mutex;
static NetState cached_net_state = NetState::UNKNOWN;
static PeriodClock last_update;

#if defined(__APPLE__)
static std::once_flag nw_path_monitor_once;
static std::atomic<NetState> nw_path_monitor_state{NetState::UNKNOWN};
static std::atomic<bool> nw_path_monitor_ready{false};

static void
InitPathMonitor() noexcept
{
  if (@available(iOS 12.0, macOS 10.14, *)) {
    nw_path_monitor_t monitor = nw_path_monitor_create();
    if (monitor == nullptr)
      return;

    dispatch_queue_t queue = dispatch_queue_create("org.xcsoar.net.path-monitor",
                                                   DISPATCH_QUEUE_SERIAL);
    if (queue == nullptr) {
      nw_path_monitor_cancel(monitor);
      return;
    }

    nw_path_monitor_set_queue(monitor, queue);
    nw_path_monitor_set_update_handler(monitor, ^(nw_path_t path) {
      const nw_path_status_t status = nw_path_get_status(path);
      const NetState state = status == nw_path_status_satisfied
        ? NetState::CONNECTED
        : status == nw_path_status_unsatisfied
          ? NetState::DISCONNECTED
          : NetState::UNKNOWN;

      nw_path_monitor_state.store(state, std::memory_order_release);
      nw_path_monitor_ready.store(true, std::memory_order_release);
    });
    nw_path_monitor_start(monitor);

    // Keep monitor and queue alive for the lifetime of the program
    static nw_path_monitor_t global_monitor = monitor;
    static dispatch_queue_t global_queue = queue;
    (void)global_monitor;
    (void)global_queue;
  }
}
#endif

/**
 * True if a non-loopback, non-virtual (docker/veth/…) interface in sysfs
 * reports #operstate "up".  Virtual bridges are often "up" while a WiFi
 * link is off; they must not keep #NetState "connected".  On #KOBO this
 * is the only link-level source (D-Bus NM path is not used there).
 */
#if defined(__linux__)
static bool
IsContainerOrBridgeIfName(const char *name) noexcept
{
  return strncmp(name, "docker", 6) == 0 || strncmp(name, "veth", 4) == 0 ||
         strncmp(name, "br-", 3) == 0 || strncmp(name, "virbr", 5) == 0 ||
         strncmp(name, "cni", 3) == 0;
}

static NetState
PollNetStateLinuxSysfs() noexcept
{
  DIR *dir = opendir("/sys/class/net");
  if (dir == nullptr)
    return NetState::UNKNOWN;

  NetState result = NetState::DISCONNECTED;
  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_name[0] == '.' || strcmp(entry->d_name, "lo") == 0)
      continue;
    if (IsContainerOrBridgeIfName(entry->d_name))
      continue;

    char path[64];
    snprintf(path, sizeof(path), "/sys/class/net/%s/operstate", entry->d_name);

    char buf[16] = {};
    if (File::ReadString(Path(path), buf, sizeof(buf)) && strncmp(buf, "up", 2) == 0) {
      result = NetState::CONNECTED;
      break;
    }
  }

  closedir(dir);
  return result;
}
#endif

static NetState
PollNetState() noexcept
{
#if defined(__linux__)
#  if defined(HAVE_NET_STATE_NM_DBUS)
  if (const std::optional<NetState> nm{
        TryGetNetStateFromNetworkManager()};
      nm.has_value()) {
    return *nm;
  }
#  endif
#  if defined(HAVE_NET_STATE_CONNMAN_DBUS)
  if (const std::optional<NetState> cm{TryGetNetStateFromConnMan()};
      cm.has_value()) {
    return *cm;
  }
#  endif
  return PollNetStateLinuxSysfs();

#elif defined(_WIN32)
  DWORD flags = 0;
  BOOL connected = InternetGetConnectedState(&flags, 0);
  return connected ? NetState::CONNECTED : NetState::DISCONNECTED;

#elif defined(__APPLE__)
  if (@available(iOS 12.0, macOS 10.14, *)) {
    std::call_once(nw_path_monitor_once, InitPathMonitor);
    if (!nw_path_monitor_ready.load(std::memory_order_acquire))
      return NetState::UNKNOWN;

    return nw_path_monitor_state.load(std::memory_order_acquire);
  }

  return NetState::UNKNOWN;

#else
  return NetState::UNKNOWN;
#endif
}

} // anonymous namespace

/* Shared GetNetState: caches PollNetState() for a short interval. */
NetState
GetNetState() noexcept
{
  std::lock_guard<std::mutex> lock(net_state_mutex);

#if defined(__linux__)
  /* Slightly faster refresh so link up/down and NM updates reach the
     map icon and reachability after toggling the radio, without waiting
     two 2s periods for a new sample. */
  if (last_update.CheckUpdate(std::chrono::seconds(1)))
    cached_net_state = PollNetState();
#else
  if (last_update.CheckUpdate(std::chrono::seconds(2)))
    cached_net_state = PollNetState();
#endif

  return cached_net_state;
}

#endif // defined(HAVE_NET_STATE)

#endif // non-Android
