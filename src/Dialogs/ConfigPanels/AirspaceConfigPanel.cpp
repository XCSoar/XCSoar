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

#include "DataField/Enum.hpp"
#include "DataField/ComboList.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "Form/Frame.hpp"
#include "Dialogs/Dialogs.h"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "SettingsAirspace.hpp"
#include "Interface.hpp"
#include "AirspaceConfigPanel.hpp"
#include "Language.hpp"

static WndForm* wf = NULL;


void
AirspaceConfigPanel::OnAirspaceColoursClicked(gcc_unused WndButton &button)
{
  dlgAirspaceShowModal(true);
}


void
AirspaceConfigPanel::OnAirspaceModeClicked(gcc_unused WndButton &button)
{
  dlgAirspaceShowModal(false);
}


void
AirspaceConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;
  WndProperty *wp;

  const SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SettingsComputer();
  const SETTINGS_MAP &settings_map = XCSoarInterface::SettingsMap();

  wp = (WndProperty*)wf->FindByName(_T("prpAirspaceDisplay"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("All on"));
    dfe->addEnumText(_("Clip"));
    dfe->addEnumText(_("Auto"));
    dfe->addEnumText(_("All below"));
    dfe->Set(settings_computer.AltitudeMode);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpClipAltitude"), ugAltitude,
                   settings_computer.ClipAltitude);

  LoadFormProperty(*wf, _T("prpAltWarningMargin"), ugAltitude,
                   settings_computer.airspace_warnings.AltWarningMargin);

  LoadFormProperty(*wf, _T("prpAirspaceWarnings"),
                   settings_computer.EnableAirspaceWarnings);

  LoadFormProperty(*wf, _T("prpWarningTime"),
                   settings_computer.airspace_warnings.WarningTime);

  LoadFormProperty(*wf, _T("prpAcknowledgementTime"),
                   settings_computer.airspace_warnings.AcknowledgementTime);

  LoadFormProperty(*wf, _T("prpAirspaceOutline"),
                   settings_map.bAirspaceBlackOutline);

  wp = (WndProperty *)wf->FindByName(_T("prpAirspaceFillMode"));
  {
#ifdef ENABLE_OPENGL
    wp->hide();
#else
    DataFieldEnum &dfe = *(DataFieldEnum *)wp->GetDataField();
    dfe.addEnumText(_("Default"), SETTINGS_MAP::AS_FILL_DEFAULT);
    dfe.addEnumText(_("Fill all"), SETTINGS_MAP::AS_FILL_ALL);
    dfe.addEnumText(_("Fill padding"), SETTINGS_MAP::AS_FILL_PADDING);
    dfe.Set(settings_map.AirspaceFillMode);
    wp->RefreshDisplay();
#endif
  }

#if !defined(ENABLE_OPENGL) && defined(HAVE_ALPHA_BLEND)
  if (AlphaBlendAvailable())
    LoadFormProperty(*wf, _T("prpAirspaceTransparency"),
                     settings_map.airspace_transparency);
  else
#endif
    wf->FindByName(_T("prpAirspaceTransparency"))->hide();
}


bool
AirspaceConfigPanel::Save(bool &requirerestart)
{
  bool changed = false;
  SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SetSettingsComputer();

  short tmp = settings_computer.AltitudeMode;
  changed |= SaveFormProperty(*wf, _T("prpAirspaceDisplay"),
                              szProfileAltMode, tmp);
  settings_computer.AltitudeMode = (AirspaceDisplayMode_t)tmp;

  changed |= SaveFormProperty(*wf, _T("prpClipAltitude"), ugAltitude,
                              settings_computer.ClipAltitude,
                              szProfileClipAlt);

  changed |= SaveFormProperty(*wf, _T("prpAltWarningMargin"),
                              ugAltitude, settings_computer.airspace_warnings.AltWarningMargin,
                              szProfileAltMargin);

  changed |= SaveFormProperty(*wf, _T("prpAirspaceWarnings"),
                              szProfileAirspaceWarning,
                              settings_computer.EnableAirspaceWarnings);

  if (SaveFormProperty(*wf, _T("prpWarningTime"),
                       szProfileWarningTime,
                       settings_computer.airspace_warnings.WarningTime)) {
    changed = true;
    requirerestart = true;
  }

  if (SaveFormProperty(*wf, _T("prpAcknowledgementTime"),
                       szProfileAcknowledgementTime,
                       settings_computer.airspace_warnings.AcknowledgementTime)) {
    changed = true;
    requirerestart = true;
  }

  changed |= SaveFormProperty(*wf, _T("prpAirspaceOutline"),
                              szProfileAirspaceBlackOutline,
                              XCSoarInterface::SetSettingsMap().bAirspaceBlackOutline);

#ifndef ENABLE_OPENGL
  SETTINGS_MAP &settings_map = XCSoarInterface::SetSettingsMap();
  tmp = settings_map.AirspaceFillMode;
  changed |= SaveFormProperty(*wf, _T("prpAirspaceFillMode"),
                              szProfileAirspaceFillMode, tmp);
  settings_map.AirspaceFillMode = (enum SETTINGS_MAP::AirspaceFillMode)tmp;

#ifdef HAVE_ALPHA_BLEND
  if (AlphaBlendAvailable())
    changed |= SaveFormProperty(*wf, _T("prpAirspaceTransparency"),
                                szProfileAirspaceTransparency,
                                settings_map.airspace_transparency);
#endif
#endif /* !OpenGL */

  return changed;
}
