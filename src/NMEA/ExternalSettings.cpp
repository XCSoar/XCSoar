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

#include "ExternalSettings.hpp"

void
ExternalSettings::Clear()
{
  mac_cready_available.Clear();
  ballast_fraction_available.Clear();
  ballast_overload_available.Clear();
  wing_loading_available.Clear();
  bugs_available.Clear();
  qnh_available.Clear();
  volume_available.Clear();
}

void
ExternalSettings::Expire(double time)
{
  /* the settings do not expire, they are only updated with a new
     value */
}

void
ExternalSettings::Complement(const ExternalSettings &add)
{
  if (add.mac_cready_available.Modified(mac_cready_available)) {
    mac_cready = add.mac_cready;
    mac_cready_available = add.mac_cready_available;
  }

  if (add.ballast_fraction_available.Modified(ballast_fraction_available)) {
    ballast_fraction = add.ballast_fraction;
    ballast_fraction_available = add.ballast_fraction_available;
  }

  if (add.wing_loading_available.Modified(wing_loading_available)) {
    wing_loading = add.wing_loading;
    wing_loading_available = add.wing_loading_available;
  }

  if (add.ballast_overload_available.Modified(ballast_overload_available)) {
    ballast_overload = add.ballast_overload;
    ballast_overload_available = add.ballast_overload_available;
  }

  if (add.bugs_available.Modified(bugs_available)) {
    bugs = add.bugs;
    bugs_available = add.bugs_available;
  }

  if (add.qnh_available.Modified(qnh_available)) {
    qnh = add.qnh;
    qnh_available = add.qnh_available;
  }

  if (add.volume_available.Modified(volume_available)) {
    volume = add.volume;
    volume_available = add.volume_available;
  }
}

void
ExternalSettings::EliminateRedundant(const ExternalSettings &other,
                                     const ExternalSettings &last)
{
  if (mac_cready_available && other.CompareMacCready(mac_cready) &&
      !last.CompareMacCready(mac_cready))
    mac_cready_available.Clear();

  if (ballast_fraction_available &&
      other.CompareBallastFraction(ballast_fraction) &&
      !last.CompareBallastFraction(ballast_fraction))
    ballast_fraction_available.Clear();

  if (ballast_overload_available &&
      other.CompareBallastOverload(ballast_overload) &&
      !last.CompareBallastOverload(ballast_overload))
    ballast_overload_available.Clear();

  if (wing_loading_available && other.CompareWingLoading(wing_loading) &&
      !last.CompareWingLoading(wing_loading))
    wing_loading_available.Clear();

  if (bugs_available && other.CompareBugs(bugs) &&
      !last.CompareBugs(bugs))
    bugs_available.Clear();

  if (qnh_available && other.CompareQNH(qnh) && !last.CompareQNH(qnh))
    qnh_available.Clear();

  if (volume_available && other.CompareVolume(volume) &&
      !last.CompareVolume(volume))
    volume_available.Clear();
}

bool
ExternalSettings::ProvideMacCready(double value, double time)
{
  if (value < 0 || value > 20)
    /* failed sanity check */
    return false;

  if (CompareMacCready(value))
    return false;

  mac_cready = value;
  mac_cready_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvideBallastFraction(double value, double time)
{
  if (value < 0 || value > 1)
    /* failed sanity check */
    return false;

  if (CompareBallastFraction(value))
    return false;

  ballast_fraction = value;
  ballast_fraction_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvideBallastOverload(double value, double time)
{
  if (value < 1 || value > 5)
    /* failed sanity check */
    return false;

  if (CompareBallastOverload(value))
    return false;

  ballast_overload = value;
  ballast_overload_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvideWingLoading(double value, double time)
{
  if (value < 1 || value > 200)
    /* failed sanity check */
    return false;

  if (CompareWingLoading(value))
    return false;

  wing_loading = value;
  wing_loading_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvideBugs(double value, double time)
{
  if (value < 0.5 || value > 1)
    /* failed sanity check */
    return false;

  if (CompareBugs(value))
    return false;

  bugs = value;
  bugs_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvideQNH(AtmosphericPressure value, double time)
{
  if (value.GetHectoPascal() < 500 ||
      value.GetHectoPascal() > 1500)
    /* failed sanity check */
    return false;

  if (CompareQNH(value))
    return false;

  qnh = value;
  qnh_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvideVolume(unsigned value, double time)
{
  if (value > 100)
    /* failed sanity check */
    return false;

  if (CompareVolume(value))
    return false;

  volume = value;
  volume_available.Update(time);
  return true;
}
