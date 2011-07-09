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

#include "AirspaceConfigPanel.hpp"
#include "DataField/Enum.hpp"
#include "DataField/Boolean.hpp"
#include "DataField/ComboList.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "Form/Frame.hpp"
#include "Form/Form.hpp"
#include "Dialogs/Airspace.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Language/Language.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Airspace/AirspaceRendererSettings.hpp"

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

static void
ShowDisplayControls(AirspaceDisplayMode_t mode)
{
  ShowFormControl(*wf, _T("prpClipAltitude"), mode == CLIP);
  ShowFormControl(*wf, _T("prpAltWarningMargin"), mode == AUTO);
}

static void
ShowWarningControls(bool visible)
{
  ShowFormControl(*wf, _T("prpWarningTime"), visible);
  ShowFormControl(*wf, _T("prpAcknowledgementTime"), visible);
}

void
AirspaceConfigPanel::OnAirspaceDisplay(DataField *Sender,
                                       DataField::DataAccessKind_t Mode)
{
  const DataFieldEnum &df = *(const DataFieldEnum *)Sender;
  AirspaceDisplayMode_t mode = (AirspaceDisplayMode_t)df.GetAsInteger();
  ShowDisplayControls(mode);
}

void
AirspaceConfigPanel::OnAirspaceWarning(DataField *Sender,
                                       DataField::DataAccessKind_t Mode)
{
  const DataFieldBoolean &df = *(const DataFieldBoolean *)Sender;
  ShowWarningControls(df.GetAsBoolean());
}

void
AirspaceConfigPanel::Init(WndForm *_wf,
                          const AirspaceComputerSettings &computer,
                          const AirspaceRendererSettings &renderer)
{
  assert(_wf != NULL);
  wf = _wf;
  WndProperty *wp;

  wp = (WndProperty*)wf->FindByName(_T("prpAirspaceDisplay"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("All on"));
    dfe->addEnumText(_("Clip"));
    dfe->addEnumText(_("Auto"));
    dfe->addEnumText(_("All below"));
    dfe->Set(renderer.altitude_mode);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpClipAltitude"), ugAltitude,
                   renderer.clip_altitude);

  LoadFormProperty(*wf, _T("prpAltWarningMargin"), ugAltitude,
                   computer.warnings.AltWarningMargin);

  LoadFormProperty(*wf, _T("prpAirspaceWarnings"), computer.enable_warnings);
  LoadFormProperty(*wf, _T("prpWarningTime"), computer.warnings.WarningTime);
  LoadFormProperty(*wf, _T("prpAcknowledgementTime"),
                   computer.warnings.AcknowledgementTime);

  LoadFormProperty(*wf, _T("prpAirspaceOutline"), renderer.black_outline);

  wp = (WndProperty *)wf->FindByName(_T("prpAirspaceFillMode"));
  {
#ifdef ENABLE_OPENGL
    wp->hide();
    wf->RemoveExpert(wp);  // prevent unhiding with expert-switch
#else
    DataFieldEnum &dfe = *(DataFieldEnum *)wp->GetDataField();
    dfe.addEnumText(_("Default"), AirspaceRendererSettings::AS_FILL_DEFAULT);
    dfe.addEnumText(_("Fill all"), AirspaceRendererSettings::AS_FILL_ALL);
    dfe.addEnumText(_("Fill padding"),
                    AirspaceRendererSettings::AS_FILL_PADDING);
    dfe.Set(renderer.fill_mode);
    wp->RefreshDisplay();
#endif
  }

#if !defined(ENABLE_OPENGL) && defined(HAVE_ALPHA_BLEND)
  if (AlphaBlendAvailable())
    LoadFormProperty(*wf, _T("prpAirspaceTransparency"),
                     renderer.transparency);
  else
#endif
  {
    wp = (WndProperty *)wf->FindByName(_T("prpAirspaceTransparency"));
    wp->hide();
    wf->RemoveExpert(wp);  // prevent unhiding with expert-switch
  }

  ShowDisplayControls(renderer.altitude_mode);
  ShowWarningControls(computer.enable_warnings);
}


bool
AirspaceConfigPanel::Save(bool &requirerestart,
                          AirspaceComputerSettings &computer,
                          AirspaceRendererSettings &renderer)
{
  bool changed = false;

  short tmp = renderer.altitude_mode;
  changed |= SaveFormProperty(*wf, _T("prpAirspaceDisplay"),
                              szProfileAltMode, tmp);
  renderer.altitude_mode = (AirspaceDisplayMode_t)tmp;

  changed |= SaveFormProperty(*wf, _T("prpClipAltitude"), ugAltitude,
                              renderer.clip_altitude,
                              szProfileClipAlt);

  changed |= SaveFormProperty(*wf, _T("prpAltWarningMargin"),
                              ugAltitude, computer.warnings.AltWarningMargin,
                              szProfileAltMargin);

  changed |= SaveFormProperty(*wf, _T("prpAirspaceWarnings"),
                              szProfileAirspaceWarning,
                              computer.enable_warnings);

  if (SaveFormProperty(*wf, _T("prpWarningTime"),
                       szProfileWarningTime,
                       computer.warnings.WarningTime)) {
    changed = true;
    requirerestart = true;
  }

  if (SaveFormProperty(*wf, _T("prpAcknowledgementTime"),
                       szProfileAcknowledgementTime,
                       computer.warnings.AcknowledgementTime)) {
    changed = true;
    requirerestart = true;
  }

  changed |= SaveFormProperty(*wf, _T("prpAirspaceOutline"),
                              szProfileAirspaceBlackOutline,
                              renderer.black_outline);

#ifndef ENABLE_OPENGL
  tmp = renderer.fill_mode;
  changed |= SaveFormProperty(*wf, _T("prpAirspaceFillMode"),
                              szProfileAirspaceFillMode, tmp);
  renderer.fill_mode = (enum AirspaceRendererSettings::AirspaceFillMode)tmp;

#ifdef HAVE_ALPHA_BLEND
  if (AlphaBlendAvailable())
    changed |= SaveFormProperty(*wf, _T("prpAirspaceTransparency"),
                                szProfileAirspaceTransparency,
                                renderer.transparency);
#endif
#endif /* !OpenGL */

  return changed;
}
