/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "ActionInterface.hpp"
#include "Device/All.hpp"
#include "Operation/MessageOperationEnvironment.hpp"

static void
BallastProcessTimer()
{
  static Validity last_fraction, last_overload;
  const ExternalSettings &settings = CommonInterface::Basic().settings;
  const Plane &plane = CommonInterface::GetComputerSettings().plane;

  if (settings.ballast_fraction_available.Modified(last_fraction))
    ActionInterface::SetBallast(settings.ballast_fraction, false);

  last_fraction = settings.ballast_fraction_available;

  if (settings.ballast_overload_available.Modified(last_overload) &&
      settings.ballast_overload >= fixed(1) &&
      positive(plane.max_ballast)) {
    fixed fraction = ((settings.ballast_overload - fixed(1)) *
                      plane.dry_mass) / plane.max_ballast;
    ActionInterface::SetBallast(fraction, false);
  }

  last_overload = settings.ballast_overload_available;
}

static void
BugsProcessTimer()
{
  static Validity last;
  const ExternalSettings &settings = CommonInterface::Basic().settings;

  if (settings.bugs_available.Modified(last))
    ActionInterface::SetBugs(settings.bugs, false);

  last = settings.bugs_available;
}

static void
QNHProcessTimer()
{
  ComputerSettings &settings_computer =
    CommonInterface::SetComputerSettings();
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (basic.settings.qnh_available.Modified(settings_computer.pressure_available)) {
    settings_computer.pressure = basic.settings.qnh;
    settings_computer.pressure_available = basic.settings.qnh_available;
  }

  if (calculated.pressure_available.Modified(settings_computer.pressure_available)) {
    settings_computer.pressure = calculated.pressure;
    settings_computer.pressure_available = calculated.pressure_available;

    MessageOperationEnvironment env;
    AllDevicesPutQNH(settings_computer.pressure, env);
  }
}

static void
MacCreadyProcessTimer()
{
  static ExternalSettings last_external_settings;
  static Validity last_auto_mac_cready;

  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (last_auto_mac_cready.Modified(calculated.auto_mac_cready_available))
    /* time warp, reset */
    last_auto_mac_cready.Clear();

  if (basic.settings.mac_cready_available.Modified(last_external_settings.mac_cready_available)) {
    ActionInterface::SetMacCready(basic.settings.mac_cready, false);
  } else if (calculated.auto_mac_cready_available.Modified(last_auto_mac_cready)) {
    last_auto_mac_cready = calculated.auto_mac_cready_available;
    ActionInterface::SetMacCready(calculated.auto_mac_cready);
  }

  last_external_settings = basic.settings;
}

void
ApplyExternalSettings()
{
  BallastProcessTimer();
  BugsProcessTimer();
  QNHProcessTimer();
  MacCreadyProcessTimer();
}
