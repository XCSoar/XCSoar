// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "State.hpp"

#ifdef ANDROID
#define NO_SCREEN
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"

NetState
GetNetState()
{
  return native_view != nullptr
    ? NetState(native_view->GetNetState(Java::GetEnv()))
    : NetState::UNKNOWN;
}

#elif defined(__linux__)

#include <cstring>
#include <cstdio>
#include <mutex>
#include <dirent.h>
#include <chrono>
#include "system/FileUtil.hpp"
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
  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_name[0] == '.' || strcmp(entry->d_name, "lo") == 0)
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

NetState
GetNetState() noexcept
{
  std::lock_guard<std::mutex> lock(net_state_mutex);
  
  if (last_update.CheckUpdate(std::chrono::seconds(2)))
    cached_net_state = PollNetState();

  return cached_net_state;
}
#endif
