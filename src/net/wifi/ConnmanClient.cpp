// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConnmanClient.hpp"
#include "LinuxNetWifiDbus.hpp"
#include "WifiTypes.hpp"
#include "WifiError.hpp"
#include "Language/Language.hpp"
#include "lib/dbus/CallMethodSync.hxx"
#include "lib/dbus/Connection.hxx"
#include "lib/dbus/Message.hxx"
#include "lib/dbus/Properties.hxx"
#include "lib/dbus/ReadIter.hxx"
#include "lib/dbus/Values.hxx"
#include "util/StringAPI.hxx"

#include <stdexcept>
#include <string>
#include <vector>

static constexpr const char *kCM = "net.connman";
static constexpr const char *kCMPath = "/";

bool
CmClient::IsActiveServiceState(const std::string &state) noexcept
{
  return state == "online" || state == "ready" || state == "configuration";
}

void
CmClient::SetPassphrase(ODBus::Connection &c, const char *path, const char *passphrase)
{
  if (passphrase == nullptr)
    return;
  using namespace ODBus;
  PropertiesSet(c, kCM, path, "net.connman.Service", "Passphrase",
                String(passphrase));
}

void
CmClient::Connect(ODBus::Connection &c, const char *path)
{
  using namespace ODBus;
  auto m = Message::NewMethodCall(kCM, path, "net.connman.Service", "Connect");
  (void)CallMethodSync(c, m);
}

void
CmClient::Disconnect(ODBus::Connection &c, const char *path)
{
  using namespace ODBus;
  auto m = Message::NewMethodCall(kCM, path, "net.connman.Service", "Disconnect");
  (void)CallMethodSync(c, m);
}

static void
ReadVString(ODBus::ReadMessageIter &i, std::string &s)
{
  s.clear();
  if (i.GetArgType() == DBUS_TYPE_STRING) {
    const char *p = i.GetString();
    if (p != nullptr)
      s = p;
  }
}

static int
ReadVInt(ODBus::ReadMessageIter &i)
{
  if (i.GetArgType() == DBUS_TYPE_INT32) {
    dbus_int32_t v = 0;
    i.GetBasic(&v);
    return (int)v;
  }
  if (i.GetArgType() == DBUS_TYPE_BYTE) {
    unsigned char b = 0;
    i.GetBasic(&b);
    return b;
  }
  return 0;
}

static void
ReadVDictStringProperty(ODBus::ReadMessageIter &i, const char *name,
                        std::string &value)
{
  if (i.GetArgType() != DBUS_TYPE_ARRAY)
    return;

  auto properties = i.Recurse();
  properties.ForEachProperty(
    [&](const char *key, ODBus::ReadMessageIter v) {
      if (StringIsEqual(key, name))
        ReadVString(v, value);
    });
}

std::vector<CmClient::ServiceEntry>
CmClient::ListServices(ODBus::Connection &c)
{
  std::vector<ServiceEntry> out;
  using namespace ODBus;
  auto m = Message::NewMethodCall(kCM, kCMPath, "net.connman.Manager", "GetServices");
  Message r = CallMethodSync(c, m);
  ReadMessageIter top{*r.Get()};
  if (top.GetArgType() != DBUS_TYPE_ARRAY)
    return out;
  for (ReadMessageIter a = top.Recurse();
       a.GetArgType() == DBUS_TYPE_DICT_ENTRY; a.Next()) {
    ReadMessageIter d = a.Recurse();
    if (d.GetArgType() != DBUS_TYPE_OBJECT_PATH) {
      continue;
    }
    ServiceEntry se;
    const char *opath = d.GetString();
    if (opath == nullptr)
      continue;
    se.path = opath;
    if (!d.Next())
      continue;
    if (d.GetArgType() != DBUS_TYPE_ARRAY)
      continue;
    ReadMessageIter props = d.Recurse();
    std::string name, type, security;
    props.ForEachProperty(
      [&](const char *k, ODBus::ReadMessageIter v) {
        if (StringIsEqual(k, "Name"))
          ReadVString(v, name);
        else if (StringIsEqual(k, "Type"))
          ReadVString(v, type);
        else if (StringIsEqual(k, "Ethernet"))
          ReadVDictStringProperty(v, "Interface", se.interface_name);
        else if (StringIsEqual(k, "State"))
          ReadVString(v, se.state);
        else if (StringIsEqual(k, "Security")) {
          ReadVString(v, security);
          if (!security.empty() && !StringIsEqual(security.c_str(), "none")) {
            se.needs_key = true;
          }
        } else if (StringIsEqual(k, "Strength"))
          se.strength = ReadVInt(v);
      });
    if (type != "wifi")
      continue;
    se.ssid_text = name;
    if (se.ssid_text.empty())
      se.ssid_text = se.path;
    out.push_back(std::move(se));
  }
  return out;
}

std::string
CmClient::FormatStatus(ODBus::Connection &c)
{
  for (const auto &s : ListServices(c)) {
    if (IsActiveServiceState(s.state))
      return s.ssid_text.empty() ? _("Connected") : s.ssid_text;
  }

  return _("Not connected");
}
