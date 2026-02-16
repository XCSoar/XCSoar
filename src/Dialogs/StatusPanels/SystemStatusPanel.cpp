// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SystemStatusPanel.hpp"
#include "Logger/Logger.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Hardware/PowerGlobal.hpp"
#include "net/State.hpp"

#ifdef HAVE_BATTERY
#include "Hardware/PowerInfo.hpp"
#endif

enum Controls {
  GPS,
  NumSat,
  Vario,
  FLARM,
  Logger,
  Battery,
  Network,
};

[[gnu::pure]]
static const char *
GetGPSStatus(const NMEAInfo &basic) noexcept
{
  if (!basic.alive)
    return N_("Disconnected");
  else if (!basic.location_available)
    return N_("Fix invalid");
  else if (!basic.gps_altitude_available)
    return N_("2D fix");
  else
    return N_("3D fix");
}

static const char *const net_state_strings[] = {
  N_("Unknown"),
  N_("Disconnected"),
  N_("Connected"),
  N_("Roaming"),
};

[[gnu::pure]]
static const char *
ToString(NetState state) noexcept
{
  return gettext(net_state_strings[unsigned(state)]);
}

void
SystemStatusPanel::Refresh() noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const GPSState &gps = basic.gps;

  StaticString<80> Temp;

  SetText(GPS, gettext(GetGPSStatus(basic)));

  if (!basic.alive)
    ClearText(NumSat);
  else if (gps.satellites_used_available) {
    // known number of sats
    Temp.Format("%u", gps.satellites_used);
    SetText(NumSat, Temp);
  } else
    // valid but unknown number of sats
    SetText(NumSat, _("Unknown"));

  SetText(Vario, basic.total_energy_vario_available
          ? _("Connected") : _("Disconnected"));

  Temp = basic.flarm.status.available
    ? _("Connected")
    : _("Disconnected");

  if (basic.flarm.version.available &&
      !basic.flarm.version.software_version.empty()) {
    /* append FLARM firmware version */
    Temp.append(" (fw ");
    Temp.UnsafeAppendASCII(basic.flarm.version.software_version.c_str());
    Temp.push_back(')');
  }

  SetText(FLARM, Temp);

  SetText(Logger, backend_components->igc_logger != nullptr &&
          backend_components->igc_logger->IsLoggerActive()
          ? _("On")
          : _("Off"));

  Temp.clear();
#ifdef HAVE_BATTERY
  const auto &battery = Power::global_info.battery;
  if (battery.remaining_percent) {
    Temp.Format("%u %% ", *battery.remaining_percent);
  }
#endif
  if (basic.voltage_available)
    Temp.AppendFormat("%.1f V", (double)basic.voltage);
  else if (basic.battery_level_available)
    Temp.AppendFormat("%.0f%%", (double)basic.battery_level);

  SetText(Battery, Temp);

  SetText(Network, ToString(GetNetState()));
}

void
SystemStatusPanel::Prepare([[maybe_unused]] ContainerWindow &parent,
                           [[maybe_unused]] const PixelRect &rc) noexcept
{
  AddReadOnly(_("GPS lock"));
  AddReadOnly(_("Satellites in view"));
  AddReadOnly(_("Variometer"));
  AddReadOnly("FLARM");
  AddReadOnly(_("Logger"));
  AddReadOnly(_("Supply voltage"));
  AddReadOnly(_("Network"));
}

void
SystemStatusPanel::Show(const PixelRect &rc) noexcept
{
  Refresh();
  CommonInterface::GetLiveBlackboard().AddListener(rate_limiter);
  StatusPanel::Show(rc);
}

void
SystemStatusPanel::Hide() noexcept
{
  StatusPanel::Hide();
  CommonInterface::GetLiveBlackboard().RemoveListener(rate_limiter);
  rate_limiter.Cancel();
}

void
SystemStatusPanel::OnGPSUpdate([[maybe_unused]] const MoreData &basic)
{
  Refresh();
}
