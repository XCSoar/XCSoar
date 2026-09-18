// SPDX-License-Identifier: BSD-2-Clause
// Copyright The XCSoar Project

#include "Login1.hxx"
#include "AppendIter.hxx"
#include "CallMethodSync.hxx"
#include "Connection.hxx"
#include "Error.hxx"
#include "Message.hxx"
#include "util/StringCompare.hxx"

namespace Login1 {

static constexpr const char *service = "org.freedesktop.login1";
static constexpr const char *path = "/org/freedesktop/login1";
static constexpr const char *interface = "org.freedesktop.login1.Manager";

static bool
CanAction(ODBus::Connection &connection, const char *method)
{
  using namespace ODBus;

  auto msg = Message::NewMethodCall(service, path, interface, method);
  Message reply = CallMethodSync(connection, msg);

  Error error;
  const char *result;
  if (!reply.GetArgs(error, DBUS_TYPE_STRING, &result))
    error.Throw("login1 capability reply failed");

  return StringIsEqual(result, "yes");
}

bool
CanReboot(ODBus::Connection &connection)
{
  return CanAction(connection, "CanReboot");
}

bool
CanPowerOff(ODBus::Connection &connection)
{
  return CanAction(connection, "CanPowerOff");
}

static void
RequestAction(ODBus::Connection &connection, const char *method)
{
  using namespace ODBus;

  auto msg = Message::NewMethodCall(service, path, interface, method);
  AppendMessageIter{*msg.Get()}.Append(Boolean{false});
  (void)CallMethodSync(connection, msg);
}

void
Reboot(ODBus::Connection &connection)
{
  RequestAction(connection, "Reboot");
}

void
PowerOff(ODBus::Connection &connection)
{
  RequestAction(connection, "PowerOff");
}

} // namespace Login1
