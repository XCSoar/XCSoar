/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Declaration.hpp"

#include "Task/Tasks/OrderedTask.hpp"
#include "Profile/Profile.hpp"

Declaration::Declaration(const OrderedTask* task)
{
  Profile::Get(szProfilePilotName, PilotName, 64);
  Profile::Get(szProfileAircraftType, AircraftType, 32);
  Profile::Get(szProfileAircraftRego, AircraftRego, 32);

  if (task)
    for (unsigned i = 0; i < task->task_size(); i++)
      waypoints.push_back(task->get_tp(i)->get_waypoint());
}

const TCHAR* 
Declaration::get_name(const unsigned i) const
{
  return waypoints[i].Name.c_str();
}

const GeoPoint& 
Declaration::get_location(const unsigned i) const
{
  return waypoints[i].Location;
}

size_t 
Declaration::size() const
{
  return waypoints.size();
}
