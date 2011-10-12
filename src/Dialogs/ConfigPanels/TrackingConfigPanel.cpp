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

static WndForm* form = NULL;

static void
SetEnabled(bool enabled)
{
  ((WndProperty *)form->FindByName(_T("LT24Username")))->set_enabled(enabled);
  ((WndProperty *)form->FindByName(_T("LT24Password")))->set_enabled(enabled);
}

void
TrackingConfigPanel::OnLT24Enabled(CheckBoxControl &control)
{
  SetEnabled(control.get_checked());
}

void
TrackingConfigPanel::Init(WndForm *_form, const TrackingSettings &settings)
{
  assert(_form != NULL);
  form = _form;

  CheckBox *cb = (CheckBox *)form->FindByName(_T("LT24Enabled"));
  cb->set_checked(settings.livetrack24.IsDefined());
  SetEnabled(cb->get_checked());

  LoadFormProperty(*form, _T("LT24Username"), settings.livetrack24.username);
  LoadFormProperty(*form, _T("LT24Password"), settings.livetrack24.password);
}

bool
TrackingConfigPanel::Save(TrackingSettings &settings)
{
  bool changed = false;

  CheckBox *cb = (CheckBox *)form->FindByName(_T("LT24Enabled"));
  if (cb->get_checked()) {
    changed |= SaveFormProperty(*form, _T("LT24Username"),
                                settings.livetrack24.username);
    changed |= SaveFormProperty(*form, _T("LT24Password"),
                                settings.livetrack24.password);
  } else {
    changed = settings.livetrack24.IsDefined();
    settings.livetrack24.username.clear();
    settings.livetrack24.password.clear();
  }

  if (changed) {
    Profile::Set(ProfileLiveTrack24Username, settings.livetrack24.username);
    Profile::Set(ProfileLiveTrack24Password, settings.livetrack24.password);
  }

  return changed;
}
