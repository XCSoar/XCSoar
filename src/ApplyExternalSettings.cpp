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

#include "ApplyExternalSettings.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "ActionInterface.hpp"
#include "Device/MultipleDevices.hpp"
#include "Operation/MessageOperationEnvironment.hpp"

static bool
BallastProcessTimer()
{
  bool modified = false;

  static Validity last_fraction, last_overload;
  const ExternalSettings &settings = CommonInterface::Basic().settings;
  const Plane &plane = CommonInterface::GetComputerSettings().plane;
  if (settings.ballast_fraction_available.Modified(last_fraction)) {
    ActionInterface::SetBallast(settings.ballast_fraction, false);
    modified = true;
  }

  last_fraction = settings.ballast_fraction_available;

  if (settings.ballast_overload_available.Modified(last_overload) &&
      settings.ballast_overload >= 1 &&
      plane.max_ballast > 0) {
    auto fraction = ((settings.ballast_overload - 1) *
                     plane.dry_mass) / plane.max_ballast;
    ActionInterface::SetBallast(fraction, false);
    modified = true;
  }

  last_overload = settings.ballast_overload_available;

  return modified;
}

static bool
BugsProcessTimer()
{
  bool modified = false;

  static Validity last;
  const ExternalSettings &settings = CommonInterface::Basic().settings;

  if (settings.bugs_available.Modified(last)) {
    ActionInterface::SetBugs(settings.bugs, false);
    modified = true;
  }

  last = settings.bugs_available;

  return modified;
}

static bool
QNHProcessTimer()
{
  bool modified = false;

  ComputerSettings &settings_computer =
    CommonInterface::SetComputerSettings();
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (basic.settings.qnh_available.Modified(settings_computer.pressure_available)) {
    settings_computer.pressure = basic.settings.qnh;
    settings_computer.pressure_available = basic.settings.qnh_available;
    modified = true;
  }

  if (calculated.pressure_available.Modified(settings_computer.pressure_available)) {
    settings_computer.pressure = calculated.pressure;
    settings_computer.pressure_available = calculated.pressure_available;

    MessageOperationEnvironment env;
    if (devices != nullptr)
      devices->PutQNH(settings_computer.pressure, env);

    modified = true;
  }

  return modified;
}

static bool
MacCreadyProcessTimer()
{
  bool modified = false;

  static ExternalSettings last_external_settings;
  static Validity last_auto_mac_cready;

  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (last_auto_mac_cready.Modified(calculated.auto_mac_cready_available)) {
    /* time warp, reset */
    last_auto_mac_cready.Clear();
    modified = true;
  }

  if (basic.settings.mac_cready_available.Modified(last_external_settings.mac_cready_available)) {
    ActionInterface::SetMacCready(basic.settings.mac_cready, false);
    modified = true;
  } else if (calculated.auto_mac_cready_available.Modified(last_auto_mac_cready)) {
    last_auto_mac_cready = calculated.auto_mac_cready_available;
    ActionInterface::SetMacCready(calculated.auto_mac_cready);
    modified = true;
  }

  last_external_settings = basic.settings;

  return modified;
}

bool
ApplyExternalSettings()
{
  bool modified = false;
  modified |= BallastProcessTimer();
  modified |= BugsProcessTimer();
  modified |= QNHProcessTimer();
  modified |= MacCreadyProcessTimer();
  return modified;
}
