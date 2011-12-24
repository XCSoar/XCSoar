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

#include "TrackingConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "Form/CheckBox.hpp"
#include "Form/Util.hpp"
#include "Form/Form.hpp"
#include "DataField/Boolean.hpp"
#include "Language/Language.hpp"
#include "Tracking/TrackingSettings.hpp"
#include "DataField/Base.hpp"
#include "Form/XMLWidget.hpp"
#include "Screen/Layout.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Interface.hpp"

class TrackingConfigPanel : public XMLWidget {

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
  void SetEnabled(bool enabled);
};

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static TrackingConfigPanel *instance;

void
TrackingConfigPanel::Show(const PixelRect &rc)
{
  XMLWidget::Show(rc);
}

void
TrackingConfigPanel::Hide()
{
  XMLWidget::Hide();
}

void
TrackingConfigPanel::SetEnabled(bool enabled)
{
  ((WndProperty *)form.FindByName(_T("LT24Username")))->set_enabled(enabled);
  ((WndProperty *)form.FindByName(_T("LT24Password")))->set_enabled(enabled);
}

static void
OnLT24Enabled(CheckBoxControl &control)
{
  instance->SetEnabled(control.get_checked());
}

gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnLT24Enabled),
  DeclareCallBackEntry(NULL)
};

void
TrackingConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  instance = this;

  const TrackingSettings &settings =
    CommonInterface::GetComputerSettings().tracking;

  LoadWindow(CallBackTable, parent,
             Layout::landscape ? _T("IDR_XML_TRACKINGCONFIGPANEL") :
                               _T("IDR_XML_TRACKINGCONFIGPANEL_L"));

  LoadFormProperty(form, _T("TrackingInterval"), settings.interval);

  CheckBox *cb = (CheckBox *)form.FindByName(_T("LT24Enabled"));
  cb->set_checked(settings.livetrack24.enabled);
  SetEnabled(settings.livetrack24.enabled);

  LoadFormProperty(form, _T("LT24Username"), settings.livetrack24.username);
  LoadFormProperty(form, _T("LT24Password"), settings.livetrack24.password);
}

bool
TrackingConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;

  CheckBox *cb = (CheckBox *)form.FindByName(_T("LT24Enabled"));

  changed |= (cb->get_checked() != settings.livetrack24.enabled);
  settings.livetrack24.enabled = cb->get_checked();

  changed |= SaveFormProperty(form, _T("LT24Username"),
                              settings.livetrack24.username);
  changed |= SaveFormProperty(form, _T("LT24Password"),
                              settings.livetrack24.password);

  changed |= SaveFormProperty(form, _T("TrackingInterval"), settings.interval);

  if (changed) {
    Profile::Set(ProfileTrackingInterval, settings.interval);

    Profile::Set(ProfileLiveTrack24Enabled, settings.livetrack24.enabled);
    Profile::Set(ProfileLiveTrack24Username, settings.livetrack24.username);
    Profile::Set(ProfileLiveTrack24Password, settings.livetrack24.password);
  }

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateTrackingConfigPanel()
{
  return new TrackingConfigPanel();
}
