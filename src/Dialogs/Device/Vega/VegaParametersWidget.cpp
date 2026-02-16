// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VegaParametersWidget.hpp"
#include "Device/Driver/Vega/Internal.hpp"
#include "Language/Language.hpp"
#include "system/Sleep.h"
#include "Operation/PopupOperationEnvironment.hpp"

#include <cassert>

void
VegaParametersWidget::AddBoolean(const char *name, const char *label,
                                 const char *help)
{
  AddParameter(name);
  RowFormWidget::AddBoolean(label, help);
}

void
VegaParametersWidget::AddInteger(const char *name, const char *label,
                                 const char *help,
                                 int min_value, int max_value,
                                 const char *format)
{
  AddParameter(name);
  RowFormWidget::AddInteger(label, help, format, format,
                            min_value, max_value, 1, 0);
}

void
VegaParametersWidget::AddEnum(const char *name, const char *label,
                              const char *help, const StaticEnumChoice *list)
{
  AddParameter(name);
  RowFormWidget::AddEnum(label, help, list);
}

void
VegaParametersWidget::AddParameter(const StaticParameter &p)
{
  assert(p.label != NULL);

  const char *label = gettext(p.label);
  const char *help = p.help != NULL ? gettext(p.help) : NULL;

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

[[gnu::pure]]
static bool
SettingExists(VegaDevice &device, const char *name)
{
  return (bool)device.GetSetting(name);
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

    if (!SettingExists(device, i->name))
      return false;

    try {
      device.RequestSetting(i->name, env);
    } catch (OperationCancelled) {
      return false;
    } catch (...) {
      env.SetError(std::current_exception());
      return false;
    }
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
      int ui_value;

      switch (GetDataField(i).GetType()) {
      case DataField::Type::BOOLEAN:
        ui_value = GetValueBoolean(i);
        break;

      case DataField::Type::INTEGER:
        ui_value = GetValueInteger(i);
        break;

      case DataField::Type::ENUM:
        ui_value = GetValueEnum(i);
        break;

      default:
        gcc_unreachable();
        assert(false);
      }

      if (ui_value != parameter.value)
        /* don't update parameters that were changed by the user */
        continue;
    }

    if (const auto x = device.GetSetting(parameter.name)) {
      parameter.value = *x;

      switch (GetDataField(i).GetType()) {
      case DataField::Type::BOOLEAN:
        LoadValue(i, (bool)parameter.value);
        break;

      case DataField::Type::INTEGER:
        LoadValue(i, parameter.value);
        break;

      case DataField::Type::ENUM:
        LoadValueEnum(i, parameter.value);
        break;

      default:
        gcc_unreachable();
        assert(false);
      }
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
VegaParametersWidget::Prepare(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  if (static_parameters != NULL)
    for (auto i = static_parameters; i->name != NULL; ++i)
      AddParameter(*i);
}

void
VegaParametersWidget::Show(const PixelRect &rc) noexcept
{
  RequestAll();
  UpdateUI();

  RowFormWidget::Show(rc);
}

bool
VegaParametersWidget::Save(bool &changed_r) noexcept
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
    try {
      device.SendSetting(parameter.name, ui_value, env);
    } catch (OperationCancelled) {
      return false;
    } catch (...) {
      env.SetError(std::current_exception());
      return false;
    }

    parameter.value = ui_value;
    changed = true;
  }

  changed_r = changed;
  return true;
}
