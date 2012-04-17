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

#include "Device.hpp"
#include "Device/Port/Port.hpp"
#include "Util/StaticString.hpp"
#include "OS/PathName.hpp"

void
FlarmDevice::LinkTimeout()
{
  mode = Mode::UNKNOWN;
}

bool
FlarmDevice::EnableNMEA(OperationEnvironment &env)
{
  switch (mode) {
  case Mode::UNKNOWN:
    /* device could be in binary mode, we don't know, but this is the
       best we can do: */
    if (!BinaryReset(env, 500))
      return false;

    mode = Mode::NMEA;
    return true;

  case Mode::NMEA:
    return true;

  case Mode::TEXT:
    /* no real difference between NMEA and TEXT; in mode==TEXT, the
       Port thread is stopped, but the caller is responsible for
       restarting it, which means there's nothing to do for us */
    mode = Mode::NMEA;
    return true;

  case Mode::BINARY:
    if (!BinaryReset(env, 500)) {
      mode = Mode::UNKNOWN;
      return false;
    }

    mode = Mode::NMEA;
    return true;
  }

  /* unreachable */
  assert(false);
  return false;
}

bool
FlarmDevice::GetStealthMode(bool &enabled, OperationEnvironment &env)
{
  TCHAR buffer[2];
  if (!GetConfig("PRIV", buffer, ARRAY_SIZE(buffer), env))
    return false;

  if (buffer[0] == _T('1'))
    enabled = true;
  else if (buffer[0] == _T('0'))
    enabled = false;
  else
    return false;

  return true;
}

bool
FlarmDevice::SetStealthMode(bool enabled, OperationEnvironment &env)
{
  return SetConfig("PRIV", enabled ? _T("1") : _T("0"), env);
}

bool
FlarmDevice::GetRange(unsigned &range, OperationEnvironment &env)
{
  TCHAR buffer[12];
  if (!GetConfig("RANGE", buffer, ARRAY_SIZE(buffer), env))
    return false;

  TCHAR *end_ptr;
  unsigned value = _tcstoul(buffer, &end_ptr, 10);
  if (end_ptr == buffer)
    return false;

  range = value;
  return true;
}

bool
FlarmDevice::SetRange(unsigned range, OperationEnvironment &env)
{
  StaticString<32> buffer;
  buffer.Format(_T("%d"), range);
  return SetConfig("RANGE", buffer, env);
}

bool
FlarmDevice::GetPilot(TCHAR *buffer, size_t length, OperationEnvironment &env)
{
  return GetConfig("PILOT", buffer, length, env);
}

bool
FlarmDevice::SetPilot(const TCHAR *pilot_name, OperationEnvironment &env)
{
  return SetConfig("PILOT", pilot_name, env);
}

bool
FlarmDevice::GetCoPilot(TCHAR *buffer, size_t length,
                        OperationEnvironment &env)
{
  return GetConfig("COPIL", buffer, length, env);
}

bool
FlarmDevice::SetCoPilot(const TCHAR *copilot_name, OperationEnvironment &env)
{
  return SetConfig("COPIL", copilot_name, env);
}

bool
FlarmDevice::GetPlaneType(TCHAR *buffer, size_t length,
                          OperationEnvironment &env)
{
  return GetConfig("GLIDERTYPE", buffer, length, env);
}

bool
FlarmDevice::SetPlaneType(const TCHAR *plane_type, OperationEnvironment &env)
{
  return SetConfig("GLIDERTYPE", plane_type, env);
}

bool
FlarmDevice::GetPlaneRegistration(TCHAR *buffer, size_t length,
                                  OperationEnvironment &env)
{
  return GetConfig("GLIDERID", buffer, length, env);
}

bool
FlarmDevice::SetPlaneRegistration(const TCHAR *registration,
                                  OperationEnvironment &env)
{
  return SetConfig("GLIDERID", registration, env);
}

bool
FlarmDevice::GetCompetitionId(TCHAR *buffer, size_t length,
                              OperationEnvironment &env)
{
  return GetConfig("COMPID", buffer, length, env);
}

bool
FlarmDevice::SetCompetitionId(const TCHAR *competition_id,
                              OperationEnvironment &env)
{
  return SetConfig("COMPID", competition_id, env);
}

bool
FlarmDevice::GetCompetitionClass(TCHAR *buffer, size_t length,
                                 OperationEnvironment &env)
{
  return GetConfig("COMPCLASS", buffer, length, env);
}

bool
FlarmDevice::SetCompetitionClass(const TCHAR *competition_class,
                                 OperationEnvironment &env)
{
  return SetConfig("COMPCLASS", competition_class, env);
}

bool
FlarmDevice::GetConfig(const char *setting, TCHAR *buffer, size_t length,
                       OperationEnvironment &env)
{
  NarrowString<256> request;
  request.Format("PFLAC,R,%s", setting);

  NarrowString<256> expected_answer(request);
  expected_answer[6u] = 'A';
  expected_answer += ',';

  char narrow_buffer[length];

  Send(request);
  if (!Receive(expected_answer, narrow_buffer, length, env, 2000))
    return false;

  _tcscpy(buffer, PathName(narrow_buffer));
  return true;
}

bool
FlarmDevice::SetConfig(const char *setting, const TCHAR *value,
                       OperationEnvironment &env)
{
  NarrowPathName narrow_value(value);

  NarrowString<256> buffer;
  buffer.Format("PFLAC,S,%s,", setting);
  buffer.append(narrow_value);

  NarrowString<256> expected_answer(buffer);
  expected_answer[6u] = 'A';

  Send(buffer);
  return port.ExpectString(expected_answer, env, 2000);
}

void
FlarmDevice::Restart()
{
  Send("PFLAR,0");
}
