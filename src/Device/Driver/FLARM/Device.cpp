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

#include "Device.hpp"
#include "Device/Port/Port.hpp"
#include "Util/ConvertString.hpp"
#include "Util/StaticString.hxx"
#include "Util/TruncateString.hpp"
#include "Util/Macros.hpp"
#include "Util/NumberParser.hpp"
#include "Util/StringCompare.hxx"
#include "NMEA/Checksum.hpp"

void
FlarmDevice::LinkTimeout()
{
  mode = Mode::UNKNOWN;
}

bool
FlarmDevice::GetStealthMode(bool &enabled, OperationEnvironment &env)
{
  char buffer[2];
  if (!GetConfig("PRIV", buffer, ARRAY_SIZE(buffer), env))
    return false;

  if (buffer[0] == '1')
    enabled = true;
  else if (buffer[0] == '0')
    enabled = false;
  else
    return false;

  return true;
}

bool
FlarmDevice::SetStealthMode(bool enabled, OperationEnvironment &env)
{
  return SetConfig("PRIV", enabled ? "1" : "0", env);
}

bool
FlarmDevice::GetRange(unsigned &range, OperationEnvironment &env)
{
  char buffer[12];
  if (!GetConfig("RANGE", buffer, ARRAY_SIZE(buffer), env))
    return false;

  char *end_ptr;
  unsigned value = ParseUnsigned(buffer, &end_ptr, 10);
  if (end_ptr == buffer)
    return false;

  range = value;
  return true;
}

bool
FlarmDevice::SetRange(unsigned range, OperationEnvironment &env)
{
  NarrowString<32> buffer;
  buffer.Format("%d", range);
  return SetConfig("RANGE", buffer, env);
}

bool
FlarmDevice::GetBaudRate(unsigned &baud_id, OperationEnvironment &env)
{
  char buffer[12];
  if (!GetConfig("BAUD", buffer, ARRAY_SIZE(buffer), env))
    return false;

  char *end_ptr;
  unsigned value = ParseUnsigned(buffer, &end_ptr, 10);
  if (end_ptr == buffer)
    return false;

  baud_id = value;
  return true;
}

bool
FlarmDevice::SetBaudRate(unsigned baud_id, OperationEnvironment &env)
{
  NarrowString<32> buffer;
  buffer.Format("%u", baud_id);
  return SetConfig("BAUD", buffer, env);
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
FlarmDevice::GetConfig(const char *setting, char *buffer, size_t length,
                       OperationEnvironment &env)
{
  NarrowString<90> request;
  request.Format("PFLAC,R,%s", setting);

  NarrowString<90> expected_answer(request);
  expected_answer[6u] = 'A';
  expected_answer.push_back(',');

  Send(request, env);
  return Receive(expected_answer, buffer, length, env, 2000);
}

/**
 * Read three bytes from the port: an asterisk and two hexadecimals
 * comprising the given checksum.
 */
static bool
ExpectChecksum(Port &port, uint8_t checksum, OperationEnvironment &env)
{
  char data[4];
  if (!port.FullRead(data, 3, env, 500) || data[0] != '*')
    return false;

  data[3] = '\0';
  return strtoul(data + 1, nullptr, 16) == checksum;
}

bool
FlarmDevice::SetConfig(const char *setting, const char *value,
                       OperationEnvironment &env)
{
  NarrowString<90> buffer;
  buffer.Format("PFLAC,S,%s,%s", setting, value);

  NarrowString<90> expected_answer(buffer);
  expected_answer[6u] = 'A';

  Send(buffer, env);
  return port.ExpectString(expected_answer, env, 2000) &&
    ExpectChecksum(port, NMEAChecksum(expected_answer), env);
}

#ifdef _UNICODE

bool
FlarmDevice::GetConfig(const char *setting, TCHAR *buffer, size_t length,
                       OperationEnvironment &env)
{
  char narrow_buffer[90];
  if (!GetConfig(setting, narrow_buffer, ARRAY_SIZE(narrow_buffer), env))
    return false;

  if (StringIsEmpty(narrow_buffer)) {
    *buffer = _T('\0');
    return true;
  }

  UTF8ToWideConverter wide(narrow_buffer);
  if (!wide.IsValid())
    return false;

  CopyTruncateString(buffer, length, wide);
  return true;
}

bool
FlarmDevice::SetConfig(const char *setting, const TCHAR *value,
                       OperationEnvironment &env)
{
  WideToUTF8Converter narrow_value(value);
  if (!narrow_value.IsValid())
    return false;

  return SetConfig(setting, narrow_value, env);
}

#endif

void
FlarmDevice::Restart(OperationEnvironment &env)
{
  Send("PFLAR,0", env);
}
