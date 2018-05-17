/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "VegaParametersWidget.hpp"
#include "Device/Driver/Vega/Internal.hpp"
#include "Language/Language.hpp"
#include "OS/Sleep.h"
#include "Operation/PopupOperationEnvironment.hpp"

#include <assert.h>

void
VegaParametersWidget::AddBoolean(const char *name, const TCHAR *label,
                                 const TCHAR *help)
{
  AddParameter(name);
  RowFormWidget::AddBoolean(label, help);
}

void
VegaParametersWidget::AddInteger(const char *name, const TCHAR *label,
                                 const TCHAR *help,
                                 int min_value, int max_value,
                                 const TCHAR *format)
{
  AddParameter(name);
  RowFormWidget::AddInteger(label, help, format, format,
                            min_value, max_value, 1, 0);
}

void
VegaParametersWidget::AddEnum(const char *name, const TCHAR *label,
                              const TCHAR *help, const StaticEnumChoice *list)
{
  AddParameter(name);
  RowFormWidget::AddEnum(label, help, list);
}

void
VegaParametersWidget::AddParameter(const StaticParameter &p)
{
  assert(p.label != NULL);

  const TCHAR *label = gettext(p.label);
  const TCHAR *help = p.help != NULL ? gettext(p.help) : NULL;

  switch (p.type) {
  case DataField::Type::BOOLEAN:
    assert(p.choices == NULL);
    assert(p.format == NULL);

    AddBoolean(p.name, label, help);
    break;

  case DataField::Type::INTEGER:
    assert(p.choices == NULL);
    assert(p.format != NULL);

    AddInteger(p.name, label, help, p.min_value, p.max_value, p.format);
    break;

  case DataField::Type::ENUM:
    assert(p.choices != NULL);
    assert(p.format == NULL);

    AddEnum(p.name, label, help, p.choices);
    break;

  default:
    gcc_unreachable();
    assert(false);
  }
}

gcc_pure
static bool
SettingExists(VegaDevice &device, const char *name)
{
  return device.GetSetting(name).first;
}

/**
 * Wait for a setting to be received from the Vega.
 */
static bool
WaitForSetting(VegaDevice &device, const char *name, unsigned timeout_ms)
{
  for (unsigned i = 0; i < timeout_ms / 100; ++i) {
    if (SettingExists(device, name))
      return true;
    Sleep(100);
  }

  return false;
}

bool
VegaParametersWidget::RequestAll()
{
  PopupOperationEnvironment env;

  /* long timeout for first response */
  unsigned timeout_ms = 3000;

  /* the first response that we're still waiting for */
  const auto end = parameters.end();
  auto start = parameters.begin();

  for (auto i = parameters.begin(); i != end; ++i) {
    /* send up to 4 requests at a time */
    if (std::distance(start, i) >= 4) {
      /* queue is long enough: wait for one response */
      WaitForSetting(device, start->name, timeout_ms);

      /* reduce timeout for follow-up responses */
      timeout_ms = 1000;

      ++start;
    }

    if (!SettingExists(device, i->name) &&
        !device.RequestSetting(i->name, env))
      return false;
  }

  /* wait for the remaining responses */
  for (auto i = start; i != end; ++i)
    WaitForSetting(device, i->name, 500);

  return true;
}

void
VegaParametersWidget::UpdateUI()
{
  for (unsigned i = 0, end = parameters.size(); i != end; ++i) {
    Parameter &parameter = parameters[i];
    if (loaded) {
      const int ui_value = GetValueInteger(i);
      if (ui_value != parameter.value)
        /* don't update parameters that were changed by the user */
        continue;
    }

    auto x = device.GetSetting(parameter.name);
    if (x.first) {
      parameter.value = x.second;
      GetDataField(i).SetAsInteger(x.second);
      GetControl(i).RefreshDisplay();
    }
  }

  loaded = true;
}

void
VegaParametersWidget::Revert()
{
  if (loaded) {
    loaded = false;
    UpdateUI();
  }
}

void
VegaParametersWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  if (static_parameters != NULL)
    for (auto i = static_parameters; i->name != NULL; ++i)
      AddParameter(*i);
}

void
VegaParametersWidget::Show(const PixelRect &rc)
{
  RequestAll();
  UpdateUI();

  RowFormWidget::Show(rc);
}

bool
VegaParametersWidget::Save(bool &changed_r)
{
  bool changed = false;

  /* see which parameters have been edited by the user */
  PopupOperationEnvironment env;
  for (unsigned i = 0, end = parameters.size(); i != end; ++i) {
    Parameter &parameter = parameters[i];
    const int ui_value = GetValueInteger(i);
    if (ui_value == parameter.value)
      /* not modified */
      continue;

    /* value has been changed by the user */
    if (!device.SendSetting(parameter.name, ui_value, env))
      /* error; should this be told to the user? */
      return false;

    parameter.value = ui_value;
    changed = true;
  }

  changed_r = changed;
  return true;
}
