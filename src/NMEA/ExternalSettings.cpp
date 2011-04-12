/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
  mac_cready_available.clear();
  ballast_available.clear();
  bugs_available.clear();
  qnh_available.clear();
}

void
ExternalSettings::Expire(fixed time)
{
  /* the settings do not expire, they are only updated with a new
     value */
}

void
ExternalSettings::Complement(const ExternalSettings &add)
{
  if (add.mac_cready_available.modified(mac_cready_available)) {
    mac_cready = add.mac_cready;
    mac_cready_available = add.mac_cready_available;
  }

  if (add.ballast_available.modified(ballast_available)) {
    ballast = add.ballast;
    ballast_available = add.ballast_available;
  }

  if (add.bugs_available.modified(bugs_available)) {
    bugs = add.bugs;
    bugs_available = add.bugs_available;
  }

  if (add.qnh_available.modified(qnh_available)) {
    qnh = add.qnh;
    qnh_available = add.qnh_available;
  }
}

bool
ExternalSettings::ProvideMacCready(fixed value, fixed time)
{
  if (mac_cready_available && fabs(mac_cready - value) <= fixed(0.01))
    return false;

  mac_cready = value;
  mac_cready_available.update(time);
  return true;
}

bool
ExternalSettings::ProvideBallast(fixed value, fixed time)
{
  if (ballast_available && fabs(ballast - value) <= fixed(0.01))
    return false;

  ballast = value;
  ballast_available.update(time);
  return true;
}

bool
ExternalSettings::ProvideBugs(fixed value, fixed time)
{
  if (bugs_available && fabs(bugs - value) <= fixed(0.01))
    return false;

  bugs = value;
  bugs_available.update(time);
  return true;
}

bool
ExternalSettings::ProvideQNH(fixed value, fixed time)
{
  if (qnh_available && fabs(qnh.get_QNH() - value) < fixed(0.1))
    return false;

  qnh = value;
  qnh_available.update(time);
  return true;
}
