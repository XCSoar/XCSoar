/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
  if (!basic.connected)
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

  TCHAR Temp[80];

  SetFormValue(form, _T("prpGPS"), gettext(GetGPSStatus(basic)));

  if (!basic.connected)
    SetFormValue(form, _T("prpNumSat"), _T(""));
  else if (gps.satellites_used >= 0) {
    // known number of sats
    _stprintf(Temp,_T("%d"), gps.satellites_used);
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

  SetFormValue(form, _T("prpDeclared"),
               logger.IsTaskDeclared()
               ? _("Yes")
               : _("No"));

  Temp[0] = 0;
#ifdef HAVE_BATTERY
  if (Power::Battery::RemainingPercentValid) {
    _stprintf(Temp + _tcslen(Temp), _T("%d %% "),
              Power::Battery::RemainingPercent);
  }
#endif
  if (basic.voltage_available) {
    _stprintf(Temp + _tcslen(Temp), _T("%.1f V"), (double)basic.voltage);
  }

  SetFormValue(form, _T("prpBattery"), Temp);
}

void
SystemStatusPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent, _T("IDR_XML_STATUS_SYSTEM"));
}
