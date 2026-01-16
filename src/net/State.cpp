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

#elif defined(__linux__)

#include <dirent.h>
#include <cstring>
#include <cstdio>
#include <mutex>
#include "io/UniqueFileDescriptor.hxx"
#include "io/Open.hxx"
#include "time/PeriodClock.hpp"

static std::mutex net_state_mutex;
static NetState cached_net_state = NetState::UNKNOWN;
static PeriodClock last_update;

/**
 * Detect network connectivity by checking the operstate of network interfaces
 * in the sysfs (/sys/class/net) directory.
 */
static NetState
PollNetState() noexcept
{
  DIR *dir = opendir("/sys/class/net");
  if (dir == nullptr)
    return NetState::UNKNOWN;

  NetState result = NetState::DISCONNECTED;
  while (dirent *entry = readdir(dir)) {
    if (entry->d_name[0] == '.' || strcmp(entry->d_name, "lo") == 0)
      continue;

    char path[64];
    snprintf(path, sizeof(path), "/sys/class/net/%s/operstate", entry->d_name);

    auto fd = OpenReadOnly(path);
    if (fd.IsDefined()) {
      char buf[16] = {};
      ssize_t n = fd.Read(buf, sizeof(buf) - 1);
      if (n > 0 && strncmp(buf, "up", 2) == 0) {
        result = NetState::CONNECTED;
        break;
      }
    }
  }

  closedir(dir);
  return result;
}

NetState
GetNetState() noexcept
{
  std::lock_guard<std::mutex> lock(net_state_mutex);
  
  if (last_update.CheckUpdate(std::chrono::seconds(2)))
    cached_net_state = PollNetState();

  return cached_net_state;
}
#endif
