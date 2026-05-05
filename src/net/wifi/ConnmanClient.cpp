// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConnmanClient.hpp"
#include "LinuxNetWifiDbus.hpp"
#include "WifiTypes.hpp"
#include "WifiError.hpp"
#include "Language/Language.hpp"
#include "lib/dbus/AppendIter.hxx"
#include "lib/dbus/CallMethodSync.hxx"
#include "lib/dbus/Connection.hxx"
#include "lib/dbus/Message.hxx"
#include "lib/dbus/PendingCall.hxx"
#include "lib/dbus/Properties.hxx"
#include "lib/dbus/ReadIter.hxx"
#include "lib/dbus/Values.hxx"
#include "util/StringAPI.hxx"

#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

static constexpr const char *kCM = "net.connman";
static constexpr const char *kCMPath = "/";
static constexpr const char *kCMAgentInterface = "net.connman.Agent";
static constexpr const char *kCMAgentPath = "/org/xcsoar/connman/agent";

struct TechnologyEntry {
  std::string path;
  bool powered{false};
};

struct AgentInputContext {
  std::string ssid;
  std::string passphrase;
};

struct Agent {
  AgentInputContext input_context;
  std::recursive_mutex mutex;
  bool object_registered{false};
};

struct RequestedInput {
  bool name{false};
  bool passphrase{false};
  bool unsupported_required{false};
};

static std::mutex agent_state_mutex;
static std::once_flag agent_data_slot_once;
static int agent_data_slot = -1;

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

static bool
ReadVBool(ODBus::ReadMessageIter &i)
{
  return i.GetArgType() == DBUS_TYPE_BOOLEAN && i.GetBool();
}

static void
ReadVSecurityProperty(ODBus::ReadMessageIter &i, CmClient::ServiceEntry &service)
{
  if (i.GetArgType() != DBUS_TYPE_ARRAY ||
      i.GetArrayElementType() != DBUS_TYPE_STRING)
    return;

  for (auto security = i.Recurse();
       security.GetArgType() == DBUS_TYPE_STRING; security.Next()) {
    const char *value = security.GetString();
    if (value != nullptr && !StringIsEqual(value, "none")) {
      service.needs_key = true;
      return;
    }
  }
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

static void
DestroyAgent(void *data) noexcept
{
  delete static_cast<Agent *>(data);
}

static int
GetAgentDataSlot()
{
  std::call_once(agent_data_slot_once, [] {
    if (!dbus_connection_allocate_data_slot(&agent_data_slot))
      throw std::runtime_error{"Failed to allocate ConnMan agent data slot"};
  });

  return agent_data_slot;
}

static Agent &
GetAgent(ODBus::Connection &c)
{
  const int slot = GetAgentDataSlot();
  if (auto *agent = static_cast<Agent *>(dbus_connection_get_data(c, slot));
      agent != nullptr)
    return *agent;

  std::lock_guard<std::mutex> lock(agent_state_mutex);
  if (auto *agent = static_cast<Agent *>(dbus_connection_get_data(c, slot));
      agent != nullptr)
    return *agent;

  auto *agent = new Agent;
  if (!dbus_connection_set_data(c, slot, agent, DestroyAgent)) {
    delete agent;
    throw std::runtime_error{"Failed to store ConnMan agent state"};
  }

  return *agent;
}

static void
SetAgentInputContext(Agent &agent, const char *ssid, const char *passphrase)
{
  agent.input_context.ssid = ssid != nullptr ? ssid : "";
  agent.input_context.passphrase = passphrase != nullptr ? passphrase : "";
}

static void
AppendDictStringVariant(ODBus::AppendMessageIter &dict,
             const char *name, const char *value)
{
  using namespace ODBus;

  auto entry = AppendMessageIter(dict, DBUS_TYPE_DICT_ENTRY, nullptr);
  entry.Append(name).Append(Variant(String{value})).CloseContainer(dict);
}

static DBusHandlerResult
SendAgentReply(DBusConnection *connection, DBusMessage *reply) noexcept
{
  if (reply == nullptr)
    return DBUS_HANDLER_RESULT_NEED_MEMORY;

  const bool success = dbus_connection_send(connection, reply, nullptr);
  dbus_message_unref(reply);
  return success ? DBUS_HANDLER_RESULT_HANDLED
    : DBUS_HANDLER_RESULT_NEED_MEMORY;
}

static DBusHandlerResult
SendEmptyAgentReply(DBusConnection *connection, DBusMessage *message) noexcept
{
  return SendAgentReply(connection, dbus_message_new_method_return(message));
}

static DBusHandlerResult
SendAgentError(DBusConnection *connection, DBusMessage *message,
               const char *name, const char *text) noexcept
{
  return SendAgentReply(connection, dbus_message_new_error(message, name, text));
}

static std::string
ReadFieldRequirement(ODBus::ReadMessageIter &value)
{
  std::string requirement;
  if (value.GetArgType() != DBUS_TYPE_ARRAY)
    return requirement;

  auto dict = value.Recurse();
  dict.ForEachProperty(
    [&](const char *key, ODBus::ReadMessageIter entry_value) {
      if (StringIsEqual(key, "Requirement"))
        ReadVString(entry_value, requirement);
    });

  return requirement;
}

static RequestedInput
ParseRequestedInput(DBusMessage &message)
{
  RequestedInput requested;
  ODBus::ReadMessageIter iter{message};
  if (iter.GetArgType() != DBUS_TYPE_OBJECT_PATH || !iter.Next() ||
      iter.GetArgType() != DBUS_TYPE_ARRAY)
    return requested;

  auto dict = iter.Recurse();
  dict.ForEachProperty(
    [&](const char *field, auto &&value) {
      const auto requirement = ReadFieldRequirement(value);

      if (StringIsEqual(field, "Name")) {
        requested.name = true;
        return;
      }

      if (StringIsEqual(field, "Passphrase")) {
        requested.passphrase = true;
        return;
      }

      if (StringIsEqual(field, "SSID") || StringIsEqual(field, "WPS") ||
          StringIsEqual(field, "PreviousPassphrase"))
        return;

      if (requirement == "mandatory")
        requested.unsupported_required = true;
    });

  return requested;
}

static DBusHandlerResult
HandleRequestInput(DBusConnection *connection, DBusMessage *message,
                   Agent &agent) noexcept
{
  std::lock_guard<std::recursive_mutex> lock(agent.mutex);
  const auto requested = ParseRequestedInput(*message);
  if (requested.unsupported_required ||
      (requested.name && agent.input_context.ssid.empty()) ||
      (requested.passphrase && agent.input_context.passphrase.empty()))
    return SendAgentError(connection, message,
      "net.connman.Agent.Error.Canceled",
      "Unsupported ConnMan agent request");

  auto *reply = dbus_message_new_method_return(message);
  if (reply == nullptr)
    return DBUS_HANDLER_RESULT_NEED_MEMORY;

  using namespace ODBus;
  AppendMessageIter root{*reply};
  auto dict = AppendMessageIter(root, DBUS_TYPE_ARRAY,
             DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
             DBUS_TYPE_STRING_AS_STRING
             DBUS_TYPE_VARIANT_AS_STRING
             DBUS_DICT_ENTRY_END_CHAR_AS_STRING);

  if (requested.name)
    AppendDictStringVariant(dict, "Name", agent.input_context.ssid.c_str());

  if (requested.passphrase)
    AppendDictStringVariant(dict, "Passphrase",
          agent.input_context.passphrase.c_str());

  dict.CloseContainer(root);
  return SendAgentReply(connection, reply);
}

static void
HandleAgentUnregistered(DBusConnection *, void *user_data) noexcept
{
  auto *agent = static_cast<Agent *>(user_data);
  if (agent == nullptr)
    return;

  std::lock_guard<std::recursive_mutex> lock(agent->mutex);
  agent->object_registered = false;
}

static DBusHandlerResult
HandleAgentMessage(DBusConnection *connection, DBusMessage *message,
                   void *user_data) noexcept
{
  auto *agent = static_cast<Agent *>(user_data);

  if (dbus_message_is_method_call(message, kCMAgentInterface, "Release") ||
      dbus_message_is_method_call(message, kCMAgentInterface, "Cancel") ||
      dbus_message_is_method_call(message, kCMAgentInterface, "ReportError"))
    return SendEmptyAgentReply(connection, message);

  if (dbus_message_is_method_call(message, kCMAgentInterface,
               "RequestBrowser"))
    return SendAgentError(connection, message,
      "net.connman.Agent.Error.Canceled",
      "Browser authentication is not supported");

  if (dbus_message_is_method_call(message, kCMAgentInterface, "RequestInput"))
    return agent != nullptr
      ? HandleRequestInput(connection, message, *agent)
      : SendAgentError(connection, message,
          "net.connman.Agent.Error.Canceled",
          "ConnMan agent is not initialized");

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
EnsureAgentObjectRegistered(ODBus::Connection &c, Agent &agent)
{
  if (agent.object_registered)
    return;

  static constexpr DBusObjectPathVTable agent_vtable = {
    HandleAgentUnregistered,
    HandleAgentMessage,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
  };

  if (!dbus_connection_register_object_path(c, kCMAgentPath,
             &agent_vtable, &agent))
    throw std::runtime_error{"Failed to register ConnMan agent object"};

  agent.object_registered = true;
}

static void
CallAgentMethodIgnoreErrors(ODBus::Connection &c, const char *method)
{
  using namespace ODBus;

  auto message = Message::NewMethodCall(kCM, kCMPath,
               "net.connman.Manager", method);
  const char *path = kCMAgentPath;
  AppendMessageIter{*message.Get()}.AppendBasic(DBUS_TYPE_OBJECT_PATH, &path);
  (void)CallMethodSync(c, message, false);
}

static void
EnsureAgentRegistered(ODBus::Connection &c, Agent &agent)
{
  using namespace ODBus;

  EnsureAgentObjectRegistered(c, agent);
  CallAgentMethodIgnoreErrors(c, "UnregisterAgent");

  auto message = Message::NewMethodCall(kCM, kCMPath,
               "net.connman.Manager",
               "RegisterAgent");
  const char *path = kCMAgentPath;
  AppendMessageIter{*message.Get()}.AppendBasic(DBUS_TYPE_OBJECT_PATH, &path);
  (void)CallMethodSync(c, message);
}

static ODBus::Message
CallMethodSyncDispatching(ODBus::Connection &c, ODBus::Message &request)
{
  auto pending = ODBus::PendingCall::SendWithReply(c, request.Get());
  dbus_connection_flush(c);

  while (!dbus_pending_call_get_completed(pending.Get()))
    if (!dbus_connection_read_write_dispatch(c, -1))
      throw std::runtime_error{"ConnMan D-Bus dispatch failed"};

  auto reply = ODBus::Message::StealReply(*pending.Get());
  reply.CheckThrowError();
  return reply;
}

static TechnologyEntry
FindTechnology(ODBus::Connection &c, const char *type)
{
  using namespace ODBus;

  auto m = Message::NewMethodCall(kCM, kCMPath, "net.connman.Manager",
               "GetTechnologies");
  Message r = CallMethodSync(c, m);
  ReadMessageIter top{*r.Get()};
  if (top.GetArgType() != DBUS_TYPE_ARRAY)
    return {};

  for (ReadMessageIter a = top.Recurse();
       a.GetArgType() == DBUS_TYPE_STRUCT; a.Next()) {
    ReadMessageIter d = a.Recurse();
    if (d.GetArgType() != DBUS_TYPE_OBJECT_PATH)
      continue;

    TechnologyEntry technology;
    const char *path = d.GetString();
    if (path == nullptr)
      continue;

    technology.path = path;
    if (!d.Next() || d.GetArgType() != DBUS_TYPE_ARRAY)
      continue;

    std::string technology_type;
    ReadMessageIter properties = d.Recurse();
    properties.ForEachProperty(
      [&](const char *key, ODBus::ReadMessageIter value) {
        if (StringIsEqual(key, "Type"))
          ReadVString(value, technology_type);
        else if (StringIsEqual(key, "Powered"))
          technology.powered = ReadVBool(value);
      });

    if (technology_type == type)
      return technology;
  }

  return {};
}

template<typename T>
static void
SetProperty(ODBus::Connection &c, const char *path,
            const char *object_interface, const char *name, T value)
{
  using namespace ODBus;

  auto message = Message::NewMethodCall(kCM, path, object_interface,
                                        "SetProperty");
  AppendMessageIter{*message.Get()}
    .Append(name)
    .Append(Variant(value));
  (void)CallMethodSync(c, message);
}

static void
SetTechnologyPowered(ODBus::Connection &c, const char *path, bool powered)
{
  SetProperty(c, path, "net.connman.Technology", "Powered",
              ODBus::Boolean{powered});
}

static const char *
RequireServicePath(const char *path, const char *method)
{
  if (path == nullptr || *path == '\0')
    throw std::invalid_argument{std::string{"ConnMan service path is invalid for "} + method};

  return path;
}

bool
CmClient::IsActiveServiceState(const std::string &state) noexcept
{
  return state == "online" || state == "ready" || state == "configuration";
}

bool
CmClient::HasWifiTechnology(ODBus::Connection &c)
{
  return !FindTechnology(c, "wifi").path.empty();
}

void
CmClient::Disconnect(ODBus::Connection &c, const char *path)
{
  using namespace ODBus;
  auto m = Message::NewMethodCall(kCM, RequireServicePath(path, "Disconnect"), "net.connman.Service",
                                  "Disconnect");
  (void)CallMethodSync(c, m);
}

void
CmClient::Remove(ODBus::Connection &c, const char *path)
{
  using namespace ODBus;
  auto m = Message::NewMethodCall(kCM, RequireServicePath(path, "Remove"), "net.connman.Service", "Remove");
  (void)CallMethodSync(c, m);
}

bool
CmClient::IsWifiTechnologyPowered(ODBus::Connection &c)
{
  const auto technology = FindTechnology(c, "wifi");
  if (technology.path.empty())
    throw std::runtime_error{"No Wi-Fi technology available"};

  return technology.powered;
}

void
CmClient::SetWifiTechnologyPowered(ODBus::Connection &c, bool enabled)
{
  const auto technology = FindTechnology(c, "wifi");
  if (technology.path.empty())
    throw std::runtime_error{"No Wi-Fi technology available"};

  SetTechnologyPowered(c, technology.path.c_str(), enabled);
}

static void
ScanTechnology(ODBus::Connection &c, const char *path)
{
  using namespace ODBus;
  auto m = Message::NewMethodCall(kCM, path, "net.connman.Technology", "Scan");
  (void)CallMethodSync(c, m);
}

void
CmClient::EnableWifiTechnology(ODBus::Connection &c)
{
  if (!IsWifiTechnologyPowered(c))
    SetWifiTechnologyPowered(c, true);
}

void
CmClient::ScanWifiTechnology(ODBus::Connection &c)
{
  const auto technology = FindTechnology(c, "wifi");
  if (technology.path.empty())
    throw std::runtime_error{"No Wi-Fi technology available"};

  ScanTechnology(c, technology.path.c_str());
}

void
CmClient::Connect(ODBus::Connection &c, const char *path,
                  const char *ssid, const char *passphrase)
{
  using namespace ODBus;

  auto &agent = GetAgent(c);
  std::lock_guard<std::recursive_mutex> lock(agent.mutex);
  SetAgentInputContext(agent, ssid, passphrase);
  EnsureAgentRegistered(c, agent);

  auto message = Message::NewMethodCall(kCM, RequireServicePath(path, "Connect"), "net.connman.Service",
                "Connect");
  (void)CallMethodSyncDispatching(c, message);
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
       a.GetArgType() == DBUS_TYPE_STRUCT; a.Next()) {
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
    std::string name, type;
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
        else if (StringIsEqual(k, "Favorite"))
          se.favorite = ReadVBool(v);
        else if (StringIsEqual(k, "Immutable"))
          se.immutable = ReadVBool(v);
        else if (StringIsEqual(k, "Security"))
          ReadVSecurityProperty(v, se);
        else if (StringIsEqual(k, "Strength")) {
          se.strength = ReadVInt(v);
          se.has_strength = true;
        }
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
