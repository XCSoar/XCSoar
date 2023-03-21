// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Battery.hpp"
#include "util/StringAPI.hxx"

#if defined(HAVE_BATTERY) && !defined(ANDROID)
#include "PowerInfo.hpp"

#ifdef KOBO

#include "system/FileUtil.hpp"
#include "Kobo/Model.hpp"

#include <string.h>
#include <stdlib.h>

namespace Power {

Info
GetInfo() noexcept
{
  Info info;
  auto &battery = info.battery;
  auto &external = info.external;

  char line[256];

  switch (DetectKoboModel())
  {
  case KoboModel::GLO_HD:
  case KoboModel::CLARA_HD:
    if (File::ReadString(Path("/sys/class/power_supply/mc13892_bat/status"),
                         line, sizeof(line))) {
      if (StringIsEqual(line,"Not charging\n") ||
          StringIsEqual(line,"Charging\n"))
        external.status = Power::ExternalInfo::Status::ON;
      else if (StringIsEqual(line,"Discharging\n"))
        external.status = Power::ExternalInfo::Status::OFF;
    }

    if (File::ReadString(Path("/sys/class/power_supply/mc13892_bat/capacity"),
                         line, sizeof(line))) {
      int rem = atoi(line);
      battery.remaining_percent = rem;
    }
    break;

  case KoboModel::LIBRA2:
  case KoboModel::CLARA_2E:
    if (File::ReadString(Path("/sys/class/power_supply/battery/status"),
                         line, sizeof(line))) {
      if (StringIsEqual(line,"Not charging\n") ||
          StringIsEqual(line,"Charging\n") ||
          StringIsEqual(line,"Full\n"))
        external.status = Power::ExternalInfo::Status::ON;
      else if (StringIsEqual(line,"Discharging\n"))
        external.status = Power::ExternalInfo::Status::OFF;
    }

    if (File::ReadString(Path("/sys/class/power_supply/battery/capacity"),
                         line, sizeof(line))) {
      int rem = atoi(line);
      battery.remaining_percent = rem;
    }
    break;

  default:
    // code shamelessly copied from OS/SystemLoad.cpp
    if (!File::ReadString(Path("/sys/class/power_supply/mc13892_bat/uevent"),
                          line, sizeof(line)))
      return info;

    char field[80], value[80];
    int n;
    char* ptr = line;
    while (sscanf(ptr, "%[^=]=%[^\n]\n%n", field, value, &n)==2) {
      ptr += n;
      if (StringIsEqual(field,"POWER_SUPPLY_STATUS")) {
        if (StringIsEqual(value,"Not charging") ||
            StringIsEqual(value,"Charging")) {
          external.status = Power::ExternalInfo::Status::ON;
        } else if (StringIsEqual(value,"Discharging")) {
          external.status = Power::ExternalInfo::Status::OFF;
        }
      } else if (StringIsEqual(field,"POWER_SUPPLY_CAPACITY")) {
        int rem = atoi(value);
        battery.remaining_percent = rem;
      }
    }
    break;
  }

  return info;
}

} // namespace Power

#endif

#ifdef ENABLE_SDL

#include <SDL_power.h>

namespace Power {

Info
GetInfo() noexcept
{
  Info info;
  auto &battery = info.battery;
  auto &external = info.external;

  int remaining_percent;
  SDL_PowerState power_state = SDL_GetPowerInfo(NULL, &remaining_percent);
  if (remaining_percent >= 0) {
    battery.remaining_percent = remaining_percent;
  }

  switch (power_state) {
  case SDL_POWERSTATE_CHARGING:
  case SDL_POWERSTATE_CHARGED:
    external.status = Power::ExternalInfo::Status::ON;
    break;

  case SDL_POWERSTATE_ON_BATTERY:
    external.status = Power::ExternalInfo::Status::OFF;
    break;

  default:
    break;
  }

  return info;
}

} // namespace Power

#endif

#endif
