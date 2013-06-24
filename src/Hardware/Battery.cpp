/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifdef HAVE_BATTERY

#if (defined(_WIN32_WCE) && !defined(GNAV))
#include <windows.h>

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
  SYSTEM_POWER_STATUS_EX2 sps;

  // request the power status
  DWORD result = GetSystemPowerStatusEx2(&sps, sizeof(sps), TRUE);
  if (result >= sizeof(sps)) {
    if (sps.BatteryLifePercent != BATTERY_PERCENTAGE_UNKNOWN){
      Power::Battery::RemainingPercent = sps.BatteryLifePercent;
      Power::Battery::RemainingPercentValid = true;
    }
    else
      Power::Battery::RemainingPercentValid = false;

    switch (sps.BatteryFlag) {
      case BATTERY_FLAG_HIGH:
        Power::Battery::Status = Power::Battery::HIGH;
        break;
      case BATTERY_FLAG_LOW:
        Power::Battery::Status = Power::Battery::LOW;
        break;
      case BATTERY_FLAG_CRITICAL:
        Power::Battery::Status = Power::Battery::CRITICAL;
        break;
      case BATTERY_FLAG_CHARGING:
        Power::Battery::Status = Power::Battery::CHARGING;
        break;
      case BATTERY_FLAG_NO_BATTERY:
        Power::Battery::Status = Power::Battery::NOBATTERY;
        break;
      case BATTERY_FLAG_UNKNOWN:
      default:
        Power::Battery::Status = Power::Battery::UNKNOWN;
    }

    switch (sps.ACLineStatus) {
      case AC_LINE_OFFLINE:
        Power::External::Status = Power::External::OFF;
        break;
      case AC_LINE_BACKUP_POWER:
      case AC_LINE_ONLINE:
        Power::External::Status = Power::External::ON;
        break;
      case AC_LINE_UNKNOWN:
      default: 
        Power::External::Status = Power::External::UNKNOWN;
    }
  } else {
    Power::Battery::Status = Power::Battery::UNKNOWN;
    Power::External::Status = Power::External::UNKNOWN;
  }
}

#endif

#ifdef KOBO
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
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

  // code shamelessly copied from OS/SystemLoad.cpp
  int fd = open("/sys/bus/platform/drivers/pmic_battery/pmic_battery.1/power_supply/mc13892_bat/uevent", 
		O_RDONLY|O_NOCTTY);
  if (fd < 0) 
    return;

  char line[256];
  ssize_t nbytes = read(fd, line, sizeof(line) - 1);
  close(fd);

  if (nbytes <= 0)
    return;

  line[nbytes] = 0;
  char field[80], value[80];
  int n;
  char* ptr = line;
  while (sscanf(ptr, "%[^=]=%[^\n]\n%n", field, value, &n)==2) {
    ptr += n;
    if (!strcmp(field,"POWER_SUPPLY_STATUS")) {
      if (!strcmp(value,"Not charging") || !strcmp(value,"Charging")) {
	Power::External::Status = Power::External::ON;
      } else if (!strcmp(value,"Discharging")) {
	Power::External::Status = Power::External::OFF;
      }
    } else if (!strcmp(field,"POWER_SUPPLY_CAPACITY")) {
      int rem = atoi(value);
      Power::Battery::RemainingPercentValid = true;
      Power::Battery::RemainingPercent = rem;
      if (Power::External::Status == Power::External::OFF) {
	if (rem>30) {
	  Power::Battery::Status = Power::Battery::HIGH;
	} else if (rem>10) {
	  Power::Battery::Status = Power::Battery::LOW;
	} else if (rem<10) {
	  Power::Battery::Status = Power::Battery::CRITICAL;
	}
      } else {
	Power::Battery::Status = Power::Battery::CHARGING;
      }
    }
  }
}

#endif

#endif
