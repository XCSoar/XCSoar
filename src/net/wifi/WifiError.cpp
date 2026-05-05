// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WifiError.hpp"
#include "Language/Language.hpp"
#include "util/Exception.hxx"

#include <string>
#include <string_view>

namespace {

[[gnu::const]]
static constexpr const char *
DebugMessage(WifiError::Code code) noexcept
{
  using Code = WifiError::Code;

  switch (code) {
  case Code::Gone:
    return "The selected network is not available";

  case Code::NeedKey:
    return "A passphrase is required";

  case Code::NoInterface:
    return "No Wi-Fi interface available";

  case Code::NoDbusConnection:
    return "No D-Bus connection";

  case Code::ConnmanUnavailable:
    return "ConnMan is not available";

  case Code::NoBackendAvailable:
    return "No Wi-Fi backend available";

  case Code::NetworkManagerUnavailable:
    return "NetworkManager is not available";
  }

  return "The operation failed";
}

static std::string
FormatCodeForUser(WifiError::Code code)
{
  using Code = WifiError::Code;

  switch (code) {
  case Code::Gone:
    return {_("The selected network is not available. Try scanning again.")};

  case Code::NeedKey:
    return {_("A passphrase is required, or a saved network for this "
              "name must exist in the system settings.")};

  case Code::NoInterface:
    return {_("No Wi-Fi interface available")};

  case Code::NoDbusConnection:
    return {_("No D-Bus connection")};

  case Code::ConnmanUnavailable:
    return {_("ConnMan is not available")};

  case Code::NoBackendAvailable:
    return {_("No Wi-Fi backend available")};

  case Code::NetworkManagerUnavailable:
    return {_("NetworkManager is not available")};
  }

  return {_("The operation failed.")};
}

} // namespace

WifiError::Exception::Exception(Code _code)
  : std::runtime_error(DebugMessage(_code)), code(_code)
{}

static std::string
FormatWhatForUser(const char *what)
{
  if (what == nullptr) {
    return {_("The operation failed.")};
  }
  const std::string_view w{what};

  if (w.find("Not authorized") != std::string_view::npos ||
      w.find("org.freedesktop.DBus.Error.AccessDenied") != std::string_view::npos) {
    return {_("Permission denied. You may not be allowed to connect to this "
              "network.")};
  }
  if (w.find("org.freedesktop.") != std::string_view::npos) {
    return {_("A D-Bus error occurred. Is NetworkManager or ConnMan running "
              "on the bus?")};
  }
  if (w.find("DBus.Error") != std::string_view::npos ||
      w.find("DBus:") != std::string_view::npos) {
    return {_("A D-Bus error occurred. Is NetworkManager or ConnMan running "
              "on the bus?")};
  }
  if (w.find("secret") != std::string_view::npos && w.length() < 200U) {
    return {_("A passphrase is required, or a saved network for this "
              "name must exist in the system settings.")};
  }
  if (w.length() < 200U) {
    return {what};
  }
  return {_("Could not connect. Check the passphrase and try again.")};
}

std::string
WifiError::Format(const std::exception &e)
{
  if (const auto *wifi_error = dynamic_cast<const WifiError::Exception *>(&e);
      wifi_error != nullptr)
    return FormatCodeForUser(wifi_error->GetCode());

  return FormatWhatForUser(e.what());
}

std::string
WifiError::Format(std::exception_ptr e)
{
  if (e == nullptr)
    return {_("The operation failed.")};

  if (const auto *wifi_error = FindNested<WifiError::Exception>(e);
      wifi_error != nullptr)
    return FormatCodeForUser(wifi_error->GetCode());

  try {
    std::rethrow_exception(std::move(e));
  } catch (const std::exception &ex) {
    return WifiError::Format(ex);
  } catch (...) {
    return {_("The operation failed.")};
  }
}
