/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Util/StringAPI.hxx"

#ifdef HAVE_BATTERY

#ifdef KOBO

#include "OS/FileUtil.hpp"
#include "Kobo/Model.hpp"

#include <string.h>
#include <stdlib.h>

namespace Power
{
  namespace Battery{
    unsigned Temperature = 0;
    unsigned RemainingPercent = 0;
    bool RemainingPercentValid = false;
    batterystatus Status = UNKNOWN;
  };

  namespace External{
    externalstatus Status = UNKNOWN;
  };
};

void
UpdateBatteryInfo()
{
  // assume failure at entry
  Power::Battery::RemainingPercentValid = false;
  Power::Battery::Status = Power::Battery::UNKNOWN;
  Power::External::Status = Power::External::UNKNOWN;
  char line[256];

  if (DetectKoboModel() == KoboModel::GLO_HD) {
    if (File::ReadString(Path("/sys/class/power_supply/mc13892_bat/status"),
                         line, sizeof(line))) {
      if (StringIsEqual(line,"Not charging\n") ||
          StringIsEqual(line,"Charging\n"))
        Power::External::Status = Power::External::ON;
      else if (StringIsEqual(line,"Discharging\n"))
        Power::External::Status = Power::External::OFF;
    }

    if (File::ReadString(Path("/sys/class/power_supply/mc13892_bat/capacity"),
                         line, sizeof(line))) {
      int rem = atoi(line);
      Power::Battery::RemainingPercentValid = true;
      Power::Battery::RemainingPercent = rem;
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
          Power::External::Status = Power::External::ON;
        } else if (StringIsEqual(value,"Discharging")) {
          Power::External::Status = Power::External::OFF;
        }
      } else if (StringIsEqual(field,"POWER_SUPPLY_CAPACITY")) {
        int rem = atoi(value);
        Power::Battery::RemainingPercentValid = true;
        Power::Battery::RemainingPercent = rem;
      }
    }
  }

  if (Power::External::Status == Power::External::OFF) {
    if (Power::Battery::RemainingPercentValid) {
      if (Power::Battery::RemainingPercent>30)
        Power::Battery::Status = Power::Battery::HIGH;
      else if (Power::Battery::RemainingPercent>10)
        Power::Battery::Status = Power::Battery::LOW;
      else if (Power::Battery::RemainingPercent<10)
        Power::Battery::Status = Power::Battery::CRITICAL;
    }
  } else if (Power::External::Status == Power::External::ON)
    Power::Battery::Status = Power::Battery::CHARGING;
}

#endif

#ifdef ENABLE_SDL

#include <SDL_power.h>

namespace Power
{
  namespace Battery{
    unsigned Temperature = 0;
    unsigned RemainingPercent = 0;
    bool RemainingPercentValid = false;
    batterystatus Status = UNKNOWN;
  };

  namespace External{
    externalstatus Status = UNKNOWN;
  };
};

void
UpdateBatteryInfo()
{
  int remaining_percent;
  SDL_PowerState power_state = SDL_GetPowerInfo(NULL, &remaining_percent);
  if (remaining_percent >= 0) {
    Power::Battery::RemainingPercent = remaining_percent;
    Power::Battery::RemainingPercentValid = true;
  } else {
    Power::Battery::RemainingPercentValid = false;
  }

  switch (power_state) {
  case SDL_POWERSTATE_CHARGING:
  case SDL_POWERSTATE_CHARGED:
    Power::External::Status = Power::External::ON;
    Power::Battery::Status = Power::Battery::CHARGING;
  case SDL_POWERSTATE_ON_BATTERY:
    Power::External::Status = Power::External::OFF;
    if (remaining_percent >= 0) {
      if (remaining_percent > 30) {
        Power::Battery::Status = Power::Battery::HIGH;
      } else if (remaining_percent > 30) {
        Power::Battery::Status = Power::Battery::LOW;
      } else {
        Power::Battery::Status = Power::Battery::CRITICAL;
      }
    } else {
      Power::Battery::Status = Power::Battery::UNKNOWN;
    }
  default:
    Power::External::Status = Power::External::UNKNOWN;
  }
}

#endif

#endif
