/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Battery.hpp"
#include "util/StringAPI.hxx"

#ifdef HAVE_BATTERY
#include "PowerInfo.hpp"
#include "PowerGlobal.hpp"

#ifdef KOBO

#include "system/FileUtil.hpp"
#include "Kobo/Model.hpp"

#include <string.h>
#include <stdlib.h>

void
UpdateBatteryInfo()
{
  auto &info = Power::global_info;
  auto &battery = info.battery;
  auto &external = info.external;

  // assume failure at entry
  battery.remaining_percent_valid = false;
  battery.status = Power::BatteryInfo::Status::UNKNOWN;
  external.status = Power::ExternalInfo::Status::UNKNOWN;
  char line[256];

  if (DetectKoboModel() == KoboModel::GLO_HD) {
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
      battery.remaining_percent_valid = true;
      battery.remaining_percent = rem;
    }
  } else {
    // code shamelessly copied from OS/SystemLoad.cpp
    if (!File::ReadString(Path("/sys/class/power_supply/mc13892_bat/uevent"),
                          line, sizeof(line)))
      return;

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
        battery.remaining_percent_valid = true;
        battery.remaining_percent = rem;
      }
    }
  }

  if (external.status == Power::ExternalInfo::Status::OFF) {
    if (battery.remaining_percent_valid) {
      if (battery.remaining_percent>30)
        battery.status = Power::BatteryInfo::Status::HIGH;
      else if (battery.remaining_percent>10)
        battery.status = Power::BatteryInfo::Status::LOW;
      else if (battery.remaining_percent<10)
        battery.status = Power::BatteryInfo::Status::CRITICAL;
    }
  } else if (external.status == Power::ExternalInfo::Status::ON)
    battery.status = Power::BatteryInfo::Status::CHARGING;
}

#endif

#ifdef ENABLE_SDL

#include <SDL_power.h>

void
UpdateBatteryInfo()
{
  auto &info = Power::global_info;
  auto &battery = info.battery;
  auto &external = info.external;

  int remaining_percent;
  SDL_PowerState power_state = SDL_GetPowerInfo(NULL, &remaining_percent);
  if (remaining_percent >= 0) {
    battery.remaining_percent = remaining_percent;
    battery.remaining_percent_valid = true;
  } else {
    battery.remaining_percent_valid = false;
  }

  switch (power_state) {
  case SDL_POWERSTATE_CHARGING:
  case SDL_POWERSTATE_CHARGED:
    external.status = Power::ExternalInfo::Status::ON;
    battery.status = Power::BatteryInfo::Status::CHARGING;
    break;

  case SDL_POWERSTATE_ON_BATTERY:
    external.status = Power::ExternalInfo::Status::OFF;
    if (remaining_percent >= 0) {
      if (remaining_percent > 30) {
        battery.status = Power::BatteryInfo::Status::HIGH;
      } else if (remaining_percent > 30) {
        battery.status = Power::BatteryInfo::Status::LOW;
      } else {
        battery.status = Power::BatteryInfo::Status::CRITICAL;
      }
    } else {
      battery.status = Power::BatteryInfo::Status::UNKNOWN;
    }
    break;

  default:
    external.status = Power::ExternalInfo::Status::UNKNOWN;
  }
}

#endif

#endif
