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

#include "SystemStatusPanel.hpp"
#include "Logger/Logger.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Form/Util.hpp"

gcc_pure
static const TCHAR *
GetGPSStatus(const NMEAInfo &basic)
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

void
SystemStatusPanel::Refresh()
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const GPSState &gps = basic.gps;

  StaticString<80> Temp;

  SetFormValue(form, _T("prpGPS"), gettext(GetGPSStatus(basic)));

  if (!basic.alive)
    SetFormValue(form, _T("prpNumSat"), _T(""));
  else if (gps.satellites_used_available) {
    // known number of sats
    Temp.Format(_T("%d"), gps.satellites_used);
    SetFormValue(form, _T("prpNumSat"), Temp);
  } else
    // valid but unknown number of sats
    SetFormValue(form, _T("prpNumSat"), _("Unknown"));

  SetFormValue(form, _T("prpVario"),
               basic.total_energy_vario_available
               ? _("Connected")
               : _("Disconnected"));

  SetFormValue(form, _T("prpFLARM"),
               basic.flarm.available
               ? _("Connected")
               : _("Disconnected"));

  SetFormValue(form, _T("prpLogger"),
               logger.IsLoggerActive()
               ? _("On")
               : _("Off"));

  Temp.clear();
#ifdef HAVE_BATTERY
  if (Power::Battery::RemainingPercentValid) {
    Temp.Format(_T("%d %% "), Power::Battery::RemainingPercent);
  }
#endif
  if (basic.voltage_available)
    Temp.AppendFormat(_T("%.1f V"), (double)basic.voltage);
  else if (basic.battery_level_available)
    Temp.AppendFormat(_T("%.0f%%"), (double)basic.battery_level);

  SetFormValue(form, _T("prpBattery"), Temp);
}

void
SystemStatusPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent, _T("IDR_XML_STATUS_SYSTEM"));
}

void
SystemStatusPanel::Show(const PixelRect &rc)
{
  Refresh();
  CommonInterface::GetLiveBlackboard().AddListener(rate_limiter);
  WindowWidget::Show(rc);
}

void
SystemStatusPanel::Hide()
{
  WindowWidget::Hide();
  CommonInterface::GetLiveBlackboard().RemoveListener(rate_limiter);
  rate_limiter.Cancel();
}

void
SystemStatusPanel::OnGPSUpdate(const MoreData &basic)
{
  Refresh();
}
