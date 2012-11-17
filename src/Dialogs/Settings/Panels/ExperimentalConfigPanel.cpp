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

#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/RowFormWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Asset.hpp"
#include "ExperimentalConfigPanel.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "SystemSettings.hpp"
#include "Interface.hpp"

enum ControlIndex {
  DeviceModelType
};


static constexpr StaticEnumChoice model_type_list[] = {
  { (unsigned)ModelType::GENERIC, N_("Generic") },
  { (unsigned)ModelType::HP31X, _T("HP31x") },
  { (unsigned)ModelType::MEDION_P5, _T("MedionP5") },
  { (unsigned)ModelType::MIO, _T("MIO") },
  { (unsigned)ModelType::NOKIA_500, _T("Nokia500") },
  { (unsigned)ModelType::PN6000, _T("PN6000") },
  { (unsigned)ModelType::LX_MINI_MAP, _T("LX MiniMap") },
  { 0 }
};


class ExperimentalConfigPanel : public RowFormWidget {
public:
  ExperimentalConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
};

void
ExperimentalConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

#ifdef HAVE_MODEL_TYPE
  const SystemSettings &system_settings = CommonInterface::GetSystemSettings();

  AddEnum(_("Device model"),
          _("Select your PNA model to make full use of its hardware capabilities. If it is not included, use Generic type."),
          model_type_list, (unsigned)system_settings.model_type);
#endif
}

bool
ExperimentalConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

#ifdef HAVE_MODEL_TYPE
  SystemSettings &system_settings = CommonInterface::SetSystemSettings();
  changed |= SaveValueEnum(DeviceModelType, ProfileKeys::AppInfoBoxModel,
                           system_settings.model_type);
  if (changed) {
    global_model_type = system_settings.model_type;
    require_restart = true;
  }
#endif

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateExperimentalConfigPanel()
{
  return new ExperimentalConfigPanel();
}
