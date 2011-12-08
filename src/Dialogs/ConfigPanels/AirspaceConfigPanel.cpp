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
#include "DataField/Base.hpp"
#include "DataField/Enum.hpp"
#include "DataField/Boolean.hpp"
#include "DataField/ComboList.hpp"
#include "Form/Button.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "Form/Frame.hpp"
#include "Form/Form.hpp"
#include "Dialogs/Airspace.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Language/Language.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "Form/XMLWidget.hpp"
#include "Screen/Layout.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Interface.hpp"

class AirspaceConfigPanel : public XMLWidget {
public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
  void ShowDisplayControls(AirspaceDisplayMode_t mode);
  void ShowWarningControls(bool visible);
};


/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static AirspaceConfigPanel *instance;

void
AirspaceConfigPanel::Show(const PixelRect &rc)
{
  XMLWidget::Show(rc);
}

void
AirspaceConfigPanel::Hide()
{
  XMLWidget::Hide();
}

static void
OnAirspaceColoursClicked(gcc_unused WndButton &button)
{
  dlgAirspaceShowModal(true);
}

static void
OnAirspaceModeClicked(gcc_unused WndButton &button)
{
  dlgAirspaceShowModal(false);
}

void
AirspaceConfigPanel::ShowDisplayControls(AirspaceDisplayMode_t mode)
{
  ShowFormControl(form, _T("prpClipAltitude"), mode == CLIP);
  ShowFormControl(form, _T("prpAltWarningMargin"),
                            mode == AUTO || mode == ALLBELOW);
}

void
AirspaceConfigPanel::ShowWarningControls(bool visible)
{
  ShowFormControl(form, _T("prpWarningTime"), visible);
  ShowFormControl(form, _T("prpAcknowledgementTime"), visible);
}

static void
OnAirspaceDisplay(DataField *Sender,
                  DataField::DataAccessKind_t Mode)
{
  const DataFieldEnum &df = *(const DataFieldEnum *)Sender;
  AirspaceDisplayMode_t mode = (AirspaceDisplayMode_t)df.GetAsInteger();
  instance->ShowDisplayControls(mode);
}

static void
OnAirspaceWarning(DataField *Sender,
                  DataField::DataAccessKind_t Mode)
{
  const DataFieldBoolean &df = *(const DataFieldBoolean *)Sender;
  instance->ShowWarningControls(df.GetAsBoolean());
}

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnAirspaceColoursClicked),
  DeclareCallBackEntry(OnAirspaceModeClicked),
  DeclareCallBackEntry(OnAirspaceDisplay),
  DeclareCallBackEntry(OnAirspaceWarning),
  DeclareCallBackEntry(NULL)
};

void
AirspaceConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const AirspaceComputerSettings &computer =
    CommonInterface::SettingsComputer().airspace;
  const AirspaceRendererSettings &renderer =
    CommonInterface::SettingsMap().airspace;

  instance = this;

  LoadWindow(CallBackTable, parent,
             Layout::landscape ? _T("IDR_XML_AIRSPACECONFIGPANEL") :
                               _T("IDR_XML_AIRSPACECONFIGPANEL_L"));

  WndProperty *wp;

  wp = (WndProperty*)form.FindByName(_T("prpAirspaceDisplay"));
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

  LoadFormProperty(form, _T("prpClipAltitude"), ugAltitude,
                   renderer.clip_altitude);

  LoadFormProperty(form, _T("prpAltWarningMargin"), ugAltitude,
                   computer.warnings.AltWarningMargin);

  LoadFormProperty(form, _T("prpAirspaceWarnings"), computer.enable_warnings);
  LoadFormProperty(form, _T("prpWarningTime"), computer.warnings.WarningTime);
  LoadFormProperty(form, _T("prpAcknowledgementTime"),
                   computer.warnings.AcknowledgementTime);

  LoadFormProperty(form, _T("prpAirspaceOutline"), renderer.black_outline);

  wp = (WndProperty *)form.FindByName(_T("prpAirspaceFillMode"));
  {
    DataFieldEnum &dfe = *(DataFieldEnum *)wp->GetDataField();
    dfe.addEnumText(_("Default"), AirspaceRendererSettings::AS_FILL_DEFAULT);
    dfe.addEnumText(_("Fill all"), AirspaceRendererSettings::AS_FILL_ALL);
    dfe.addEnumText(_("Fill padding"),
                    AirspaceRendererSettings::AS_FILL_PADDING);
    dfe.Set(renderer.fill_mode);
    wp->RefreshDisplay();
  }

#if !defined(ENABLE_OPENGL) && defined(HAVE_ALPHA_BLEND)
  if (AlphaBlendAvailable())
    LoadFormProperty(form, _T("prpAirspaceTransparency"),
                     renderer.transparency);
  else
#endif
  {
    wp = (WndProperty *)form.FindByName(_T("prpAirspaceTransparency"));
    wp->hide();
    form.RemoveExpert(wp);  // prevent unhiding with expert-switch
  }

  ShowDisplayControls(renderer.altitude_mode);
  ShowWarningControls(computer.enable_warnings);
}


bool
AirspaceConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  AirspaceComputerSettings &computer =
    CommonInterface::SetSettingsComputer().airspace;
  AirspaceRendererSettings &renderer =
    CommonInterface::SetSettingsMap().airspace;

  changed |= SaveFormPropertyEnum(form, _T("prpAirspaceDisplay"),
                                  szProfileAltMode, renderer.altitude_mode);

  changed |= SaveFormProperty(form, _T("prpClipAltitude"), ugAltitude,
                              renderer.clip_altitude,
                              szProfileClipAlt);

  changed |= SaveFormProperty(form, _T("prpAltWarningMargin"),
                              ugAltitude, computer.warnings.AltWarningMargin,
                              szProfileAltMargin);

  changed |= SaveFormProperty(form, _T("prpAirspaceWarnings"),
                              szProfileAirspaceWarning,
                              computer.enable_warnings);

  if (SaveFormProperty(form, _T("prpWarningTime"),
                       szProfileWarningTime,
                       computer.warnings.WarningTime)) {
    changed = true;
    require_restart = true;
  }

  if (SaveFormProperty(form, _T("prpAcknowledgementTime"),
                       szProfileAcknowledgementTime,
                       computer.warnings.AcknowledgementTime)) {
    changed = true;
    require_restart = true;
  }

  changed |= SaveFormProperty(form, _T("prpAirspaceOutline"),
                              szProfileAirspaceBlackOutline,
                              renderer.black_outline);

  changed |= SaveFormPropertyEnum(form, _T("prpAirspaceFillMode"),
                                  szProfileAirspaceFillMode,
                                  renderer.fill_mode);

#ifndef ENABLE_OPENGL
#ifdef HAVE_ALPHA_BLEND
  if (AlphaBlendAvailable())
    changed |= SaveFormProperty(form, _T("prpAirspaceTransparency"),
                                szProfileAirspaceTransparency,
                                renderer.transparency);
#endif
#endif /* !OpenGL */

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}


Widget *
CreateAirspaceConfigPanel()
{
  return new AirspaceConfigPanel();
}
