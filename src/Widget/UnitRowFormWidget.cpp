// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RowFormWidget.hpp"
#include "Form/DataField/Float.hpp"
#include "Profile/Profile.hpp"
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"
#include "Math/Util.hpp"

#include <cassert>

void
RowFormWidget::AddReadOnly(const char *label, const char *help,
                           const char *display_format,
                           UnitGroup unit_group, double value) noexcept
{
  WndProperty *edit = Add(label, help, true);
  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  value = Units::ToUserUnit(value, unit);
  DataFieldFloat *df = new DataFieldFloat(display_format, display_format,
                                          0, 0,
                                          value, 1, false);
  df->SetUnits(Units::GetUnitName(unit));
  edit->SetDataField(df);
}

WndProperty *
RowFormWidget::AddFloat(const char *label, const char *help,
                        const char *display_format,
                        const char *edit_format,
                        double min_value, double max_value,
                        double step, bool fine,
                        UnitGroup unit_group, double value,
                        DataFieldListener *listener) noexcept
{
  WndProperty *edit = Add(label, help);
  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  value = Units::ToUserUnit(value, unit);
  DataFieldFloat *df = new DataFieldFloat(edit_format, display_format,
                                          min_value, max_value,
                                          value, step, fine, listener);
  df->SetUnits(Units::GetUnitName(unit));
  edit->SetDataField(df);
  return edit;
}

void
RowFormWidget::LoadValue(unsigned i, double value,
                         UnitGroup unit_group) noexcept
{
  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  WndProperty &control = GetControl(i);
  DataFieldFloat &df = *(DataFieldFloat *)control.GetDataField();
  assert(df.GetType() == DataField::Type::REAL);
  df.SetValue(Units::ToUserUnit(value, unit));
  df.SetUnits(Units::GetUnitName(unit));
  control.RefreshDisplay();
}

bool
RowFormWidget::SaveValue(unsigned i, UnitGroup unit_group,
                         double &value) const noexcept
{
  const DataFieldFloat &df =
    (const DataFieldFloat &)GetDataField(i);
  assert(df.GetType() == DataField::Type::REAL);

  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  auto new_value = df.GetValue();
  auto old_value = Units::ToUserUnit(value, unit);

  if (fabs(new_value - old_value) < df.GetStep() / 100)
    return false;

  value = Units::ToSysUnit(new_value, unit);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, UnitGroup unit_group,
                         std::string_view profile_key,
                         double &value) const noexcept
{
  const DataFieldFloat &df =
    (const DataFieldFloat &)GetDataField(i);
  assert(df.GetType() == DataField::Type::REAL);

  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  auto new_value = df.GetValue();
  auto old_value = Units::ToUserUnit(value, unit);

  if (fabs(new_value - old_value) < df.GetStep() / 100)
    return false;

  value = Units::ToSysUnit(new_value, unit);
  Profile::Set(profile_key, value);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, UnitGroup unit_group,
                         std::string_view profile_key,
                         unsigned int &value) const noexcept
{
  const DataFieldFloat &df =
    (const DataFieldFloat &)GetDataField(i);
  assert(df.GetType() == DataField::Type::INTEGER ||
         df.GetType() == DataField::Type::REAL);

  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  auto new_value = df.GetValue();
  auto old_value = Units::ToUserUnit(value, unit);

  if (fabs(new_value - old_value) < df.GetStep() / 100)
    return false;

  value = iround(Units::ToSysUnit(new_value, unit));
  Profile::Set(profile_key, value);
  return true;
}
