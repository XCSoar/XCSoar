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
#include "Form/Edit.hpp"

void
SystemStatusPanel::Refresh()
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const GPSState &gps = basic.gps;

  TCHAR Temp[80];

  WndProperty* wp;

  wp = (WndProperty*)form.FindByName(_T("prpGPS"));
  assert(wp != NULL);
  if (!basic.connected)
    wp->SetText(_("Disconnected"));
  else if (!basic.location_available)
    wp->SetText(_("Fix invalid"));
  else if (!basic.gps_altitude_available)
    wp->SetText(_("2D fix"));
  else
    wp->SetText(_("3D fix"));

  wp->RefreshDisplay();


  wp = (WndProperty*)form.FindByName(_T("prpNumSat"));
  assert(wp != NULL);
  if (!basic.connected)
    wp->SetText(_T(""));
  else if (gps.satellites_used >= 0) {
    // known number of sats
    _stprintf(Temp,_T("%d"), gps.satellites_used);
    wp->SetText(Temp);
  } else
    // valid but unknown number of sats
    wp->SetText(_("Unknown"));

  wp->RefreshDisplay();


  wp = (WndProperty*)form.FindByName(_T("prpVario"));
  assert(wp != NULL);
  if (basic.total_energy_vario_available)
    wp->SetText(_("Connected"));
  else
    wp->SetText(_("Disconnected"));

  wp->RefreshDisplay();


  wp = (WndProperty*)form.FindByName(_T("prpFLARM"));
  assert(wp != NULL);
  if (basic.flarm.available)
    wp->SetText(_("Connected"));
  else
    wp->SetText(_("Disconnected"));

  wp->RefreshDisplay();


  wp = (WndProperty*)form.FindByName(_T("prpLogger"));
  assert(wp != NULL);
  if (logger.IsLoggerActive())
    wp->SetText(_("On"));
  else
    wp->SetText(_("Off"));

  wp->RefreshDisplay();


  wp = (WndProperty*)form.FindByName(_T("prpDeclared"));
  assert(wp != NULL);
  if (logger.IsTaskDeclared())
    wp->SetText(_("Yes"));
  else
    wp->SetText(_("No"));

  wp->RefreshDisplay();


  wp = (WndProperty*)form.FindByName(_T("prpBattery"));
  assert(wp != NULL);
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

  wp->SetText(Temp);
  wp->RefreshDisplay();
}

void
SystemStatusPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent, _T("IDR_XML_STATUS_SYSTEM"));
}
