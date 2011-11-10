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

#include <assert.h>
#include <cstdio>

#ifdef _UNICODE
#include <windows.h>
#endif

void
FlarmDevice::Send(const char *sentence)
{
  assert(sentence != NULL);

  port.Write('$');
  port.Write(sentence);
  port.Write("\r\n");
}

bool
FlarmDevice::SetGet(char *buffer)
{
  assert(buffer != NULL);

  Send(buffer);

  buffer[6] = _T('A');
  return port.ExpectString(buffer);
}

#ifdef _UNICODE
bool
FlarmDevice::SetGet(TCHAR *s)
{
  assert(s != NULL);

  char buffer[_tcslen(s) * 4 + 1];
  return ::WideCharToMultiByte(CP_ACP, 0, s, -1, buffer, sizeof(buffer), NULL,
                               NULL) > 0 && SetGet(buffer);
}
#endif

bool
FlarmDevice::SetStealthMode(bool enabled)
{
  return SetConfig(_T("PRIV"), enabled ? _T("1") : _T("0"));
}

bool
FlarmDevice::SetRange(unsigned range)
{
  TCHAR buffer[32];
  _stprintf(buffer, _T("%d"), range);
  return SetConfig(_T("RANGE"), buffer);
}

bool
FlarmDevice::SetPilot(const TCHAR *pilot_name)
{
  return SetConfig(_T("PILOT"), pilot_name);
}

bool
FlarmDevice::SetCoPilot(const TCHAR *copilot_name)
{
  return SetConfig(_T("COPIL"), copilot_name);
}

bool
FlarmDevice::SetPlaneType(const TCHAR *plane_type)
{
  return SetConfig(_T("GLIDERTYPE"), plane_type);
}

bool
FlarmDevice::SetPlaneRegistration(const TCHAR *registration)
{
  return SetConfig(_T("GLIDERID"), registration);
}

bool
FlarmDevice::SetCompetitionId(const TCHAR *competition_id)
{
  return SetConfig(_T("COMPID"), competition_id);
}

bool
FlarmDevice::SetCompetitionClass(const TCHAR *competition_class)
{
  return SetConfig(_T("COMPCLASS"), competition_class);
}

bool
FlarmDevice::SetConfig(const TCHAR *setting, const TCHAR *value)
{
  TCHAR buffer[256];
  _stprintf(buffer, _T("PFLAC,S,%s,%s"), setting, value);
  return SetGet(buffer);
}

void
FlarmDevice::Restart()
{
  Send("PFLAR,0");
}
