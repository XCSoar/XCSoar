// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SystemdServiceList.hpp"
#include "Language/Language.hpp"
#include "lib/dbus/Connection.hxx"
#include "lib/dbus/Systemd.hxx"

#include <initializer_list>

namespace {

static void
AddFirstInstalled(std::vector<SystemdService> &services,
                  ODBus::Connection &connection,
                  const char *id, const char *display_name,
                  const char *description,
                  std::initializer_list<const char *> alternatives)
{
  const char *best_unit = nullptr;
  unsigned best_score = 0;

  for (const auto *unit_name : alternatives) {
    if (!Systemd::UnitExists(connection, unit_name))
      continue;

    unsigned score = 1;
    try {
      if (Systemd::IsUnitActive(connection, unit_name))
        score = 3;
      else if (Systemd::IsUnitEnabled(connection, unit_name))
        score = 2;
    } catch (...) {
    }

    if (score > best_score) {
      best_unit = unit_name;
      best_score = score;
    }
  }

  if (best_unit != nullptr)
    services.push_back({id, display_name, description, best_unit});
}

} // namespace

std::vector<SystemdService>
BuildSystemdServiceList() noexcept
{
  std::vector<SystemdService> services;

  try {
    auto connection = ODBus::Connection::GetSystem();

    AddFirstInstalled(services, connection, "ssh", _("SSH server"),
                      _("Remote terminal access"),
                      {"ssh.socket", "sshd.socket", "dropbear.socket",
                       "ssh.service", "sshd.service", "dropbear.service"});
    AddFirstInstalled(services, connection, "sensord", _("Sensors"),
                      _("OpenVario sensor daemon"),
                      {"sensord.socket", "sensord.service"});
    AddFirstInstalled(services, connection, "variod", _("Audio vario"),
                      _("OpenVario audio vario daemon"),
                      {"variod.socket", "variod.service"});
  } catch (...) {
    /* The settings page remains empty when systemd is unavailable. */
  }

  return services;
}
