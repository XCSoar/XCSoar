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

#include "Device.hpp"
#include "Device/Port/Port.hpp"
#include "Util/StaticString.hpp"
#include "OS/PathName.hpp"

bool
FlarmDevice::SetStealthMode(bool enabled)
{
  return SetConfig("PRIV", enabled ? _T("1") : _T("0"));
}

bool
FlarmDevice::SetRange(unsigned range)
{
  StaticString<32> buffer;
  buffer.Format(_T("%d"), range);
  return SetConfig("RANGE", buffer);
}

bool
FlarmDevice::SetPilot(const TCHAR *pilot_name)
{
  return SetConfig("PILOT", pilot_name);
}

bool
FlarmDevice::SetCoPilot(const TCHAR *copilot_name)
{
  return SetConfig("COPIL", copilot_name);
}

bool
FlarmDevice::SetPlaneType(const TCHAR *plane_type)
{
  return SetConfig("GLIDERTYPE", plane_type);
}

bool
FlarmDevice::SetPlaneRegistration(const TCHAR *registration)
{
  return SetConfig("GLIDERID", registration);
}

bool
FlarmDevice::SetCompetitionId(const TCHAR *competition_id)
{
  return SetConfig("COMPID", competition_id);
}

bool
FlarmDevice::SetCompetitionClass(const TCHAR *competition_class)
{
  return SetConfig("COMPCLASS", competition_class);
}

bool
FlarmDevice::SetConfig(const char *setting, const TCHAR *value)
{
  NarrowPathName narrow_value(value);

  NarrowString<256> buffer;
  buffer.Format("PFLAC,S,%s,", setting);
  buffer.append(narrow_value);

  NarrowString<256> expected_answer(buffer);
  expected_answer[6u] = 'A';

  Send(buffer);
  return port.ExpectString(expected_answer);
}

void
FlarmDevice::Restart()
{
  Send("PFLAR,0");
}
