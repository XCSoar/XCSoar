// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device.hpp"
#include "Device/Port/Port.hpp"
#include "util/StaticString.hxx"
#include "util/TruncateString.hpp"
#include "util/Macros.hpp"
#include "util/NumberParser.hpp"
#include "util/StringCompare.hxx"
#include "NMEA/Checksum.hpp"

#include <fmt/format.h>

void
FlarmDevice::LinkTimeout()
{
  mode = Mode::UNKNOWN;
}

bool
FlarmDevice::PutPilotEvent(OperationEnvironment &env)
{
  Send("PFLAI,PILOTEVENT", env);
  return true;
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
  return SetConfig("RANGE", fmt::format_int{range}.c_str(), env);
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
  return SetConfig("BAUD", fmt::format_int{baud_id}.c_str(), env);
}

bool
FlarmDevice::GetPilot(char *buffer, size_t length, OperationEnvironment &env)
{
  return GetConfig("PILOT", buffer, length, env);
}

bool
FlarmDevice::SetPilot(const char *pilot_name, OperationEnvironment &env)
{
  return SetConfig("PILOT", pilot_name, env);
}

bool
FlarmDevice::GetCoPilot(char *buffer, size_t length,
                        OperationEnvironment &env)
{
  return GetConfig("COPIL", buffer, length, env);
}

bool
FlarmDevice::SetCoPilot(const char *copilot_name, OperationEnvironment &env)
{
  return SetConfig("COPIL", copilot_name, env);
}

bool
FlarmDevice::GetPlaneType(char *buffer, size_t length,
                          OperationEnvironment &env)
{
  return GetConfig("GLIDERTYPE", buffer, length, env);
}

bool
FlarmDevice::SetPlaneType(const char *plane_type, OperationEnvironment &env)
{
  return SetConfig("GLIDERTYPE", plane_type, env);
}

bool
FlarmDevice::GetPlaneRegistration(char *buffer, size_t length,
                                  OperationEnvironment &env)
{
  return GetConfig("GLIDERID", buffer, length, env);
}

bool
FlarmDevice::SetPlaneRegistration(const char *registration,
                                  OperationEnvironment &env)
{
  return SetConfig("GLIDERID", registration, env);
}

bool
FlarmDevice::GetCompetitionId(char *buffer, size_t length,
                              OperationEnvironment &env)
{
  return GetConfig("COMPID", buffer, length, env);
}

bool
FlarmDevice::SetCompetitionId(const char *competition_id,
                              OperationEnvironment &env)
{
  return SetConfig("COMPID", competition_id, env);
}

bool
FlarmDevice::GetCompetitionClass(char *buffer, size_t length,
                                 OperationEnvironment &env)
{
  return GetConfig("COMPCLASS", buffer, length, env);
}

bool
FlarmDevice::SetCompetitionClass(const char *competition_class,
                                 OperationEnvironment &env)
{
  return SetConfig("COMPCLASS", competition_class, env);
}

bool
FlarmDevice::GetConfig(const char *setting, char *buffer, size_t length,
                       OperationEnvironment &env)
{
  StaticString<90> request;
  request.Format("PFLAC,R,%s", setting);

  StaticString<90> expected_answer(request);
  expected_answer[6u] = 'A';
  expected_answer.push_back(',');

  Send(request, env);
  return Receive(expected_answer, buffer, length,
                 env, std::chrono::seconds(2));
}

/**
 * Read three bytes from the port: an asterisk and two hexadecimals
 * comprising the given checksum.
 */
static bool
ExpectChecksum(Port &port, uint8_t checksum, OperationEnvironment &env)
{
  char data[4];
  port.FullRead(std::as_writable_bytes(std::span{data, 3}),
                env, std::chrono::milliseconds(500));
  if (data[0] != '*')
    return false;

  data[3] = '\0';
  return strtoul(data + 1, nullptr, 16) == checksum;
}

bool
FlarmDevice::SetConfig(const char *setting, const char *value,
                       OperationEnvironment &env)
{
  StaticString<90> buffer;
  buffer.Format("PFLAC,S,%s,%s", setting, value);

  StaticString<90> expected_answer(buffer);
  expected_answer[6u] = 'A';

  Send(buffer, env);
  port.ExpectString(expected_answer, env, std::chrono::seconds(2));
  return ExpectChecksum(port, NMEAChecksum(expected_answer), env);
}

void
FlarmDevice::Restart(OperationEnvironment &env)
{
  Send("PFLAR,0", env);
}

void
FlarmDevice::RunSimulation(unsigned scenario, OperationEnvironment &env)
{
  StaticString<32> buffer;
  buffer.Format("PFLAF,S,%u", scenario);
  Send(buffer, env);
}
