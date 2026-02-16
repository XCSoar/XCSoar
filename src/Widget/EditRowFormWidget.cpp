// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RowFormWidget.hpp"
#include "Form/Edit.hpp"
#include "Screen/Layout.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Integer.hpp"
#include "Form/DataField/Float.hpp"
#include "Form/DataField/Angle.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/String.hpp"
#include "Form/DataField/Password.hpp"
#include "Form/DataField/Date.hpp"
#include "Form/DataField/Time.hpp"
#include "Form/DataField/RoughTime.hpp"
#include "time/RoughTime.hpp"
#include "time/BrokenDate.hpp"
#include "Language/Language.hpp"
#include "Math/Angle.hpp"
#include "util/StringAPI.hxx"
#include "util/TruncateString.hpp"

#include <cassert>

std::unique_ptr<WndProperty>
RowFormWidget::CreateEdit(const char *label, const char *help,
                          bool read_only) noexcept
{
  assert(IsDefined());

  const PixelRect edit_rc =
    InitialControlRect(Layout::GetMinimumControlHeight());

  WindowStyle style;
  if (!read_only)
    style.TabStop();

  ContainerWindow &panel = (ContainerWindow &)GetWindow();
  auto edit =
    std::make_unique<WndProperty>(panel, look, label,
                                  edit_rc, (*label == '\0') ? 0 : 100,
                                  style);
  edit->SetReadOnly(read_only);

  if (help != nullptr)
    edit->SetHelpText(help);

  return edit;
}

WndProperty *
RowFormWidget::Add(const char *label, const char *help,
                   bool read_only) noexcept
{
  return (WndProperty *)&Add(Row::Type::EDIT,
                             CreateEdit(label, help, read_only));
}

void
RowFormWidget::AddReadOnly(const char *label, const char *help,
                           const char *text) noexcept
{
  WndProperty *control = Add(label, help, true);
  if (text != nullptr)
    control->SetText(text);
}

void
RowFormWidget::AddReadOnly(const char *label, const char *help,
                           const char *display_format,
                           double value) noexcept
{
  WndProperty *edit = Add(label, help, true);
  DataFieldFloat *df = new DataFieldFloat(display_format, display_format,
                                          0, 0, value, 1, false);
  edit->SetDataField(df);
}

void
RowFormWidget::AddReadOnly(const char *label, const char *help,
                           bool value) noexcept
{
  WndProperty *edit = Add(label, help, true);
  DataFieldBoolean *df = new DataFieldBoolean(value, _("On"), _("Off"));
  edit->SetDataField(df);
}

WndProperty *
RowFormWidget::Add(const char *label, const char *help,
                   DataField *df) noexcept
{
  WndProperty *edit = Add(label, help);
  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddBoolean(const char *label, const char *help,
                          bool value,
                          DataFieldListener *listener) noexcept
{
  WndProperty *edit = Add(label, help);
  DataFieldBoolean *df = new DataFieldBoolean(value, _("On"), _("Off"),
                                              listener);
  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddInteger(const char *label, const char *help,
                          const char *display_format,
                          const char *edit_format,
                          int min_value, int max_value, int step, int value,
                          DataFieldListener *listener) noexcept
{
  WndProperty *edit = Add(label, help);
  DataFieldInteger *df = new DataFieldInteger(edit_format, display_format,
                                              min_value, max_value,
                                              value, step, listener);
  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddFloat(const char *label, const char *help,
                        const char *display_format,
                        const char *edit_format,
                        double min_value, double max_value,
                        double step, bool fine,
                        double value,
                        DataFieldListener *listener) noexcept
{
  WndProperty *edit = Add(label, help);
  DataFieldFloat *df = new DataFieldFloat(edit_format, display_format,
                                          min_value, max_value,
                                          value, step, fine, listener);
  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddAngle(const char *label, const char *help,
                        Angle value, unsigned step, bool fine,
                        DataFieldListener *listener) noexcept
{
  WndProperty *edit = Add(label, help);
  AngleDataField *df = new AngleDataField(value, step, fine, listener);
  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddEnum(const char *label, const char *help,
                       const StaticEnumChoice *list, unsigned value,
                       DataFieldListener *listener) noexcept
{
  assert(list != nullptr);

  WndProperty *edit = Add(label, help);
  DataFieldEnum *df = new DataFieldEnum(listener);

  if (list[0].help != nullptr)
    df->EnableItemHelp(true);

  df->AddChoices(list);
  df->SetValue(value);

  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddEnum(const char *label, const char *help,
                       DataFieldListener *listener) noexcept
{
  WndProperty *edit = Add(label, help);
  DataFieldEnum *df = new DataFieldEnum(listener);

  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddText(const char *label, const char *help,
                       const char *content,
                       DataFieldListener *listener) noexcept
{
  WndProperty *edit = Add(label, help);
  DataFieldString *df = new DataFieldString(content, listener);

  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddPassword(const char *label, const char *help,
                           const char *content) noexcept
{
  WndProperty *edit = Add(label, help);
  PasswordDataField *df = new PasswordDataField(content);

  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddDuration(const char *label, const char *help,
                           std::chrono::seconds min_value,
                           std::chrono::seconds max_value,
                           std::chrono::seconds step,
                           std::chrono::seconds value,
                           unsigned max_tokens,
                           DataFieldListener *listener) noexcept
{
  WndProperty *edit = Add(label, help);
  DataFieldTime *df = new DataFieldTime(min_value, max_value, value,
                                        step, listener);
  df->SetMaxTokenNumber(max_tokens);
  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddDate(const char *label, const char *help,
                       BrokenDate date,
                       DataFieldListener *listener) noexcept
{
  WndProperty *edit = Add(label, help);
  DataFieldDate *df = new DataFieldDate(date, listener);
  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddRoughTime(const char *label, const char *help,
                            RoughTime value, RoughTimeDelta time_zone,
                            DataFieldListener *listener) noexcept
{
  WndProperty *edit = Add(label, help);
  RoughTimeDataField *df = new RoughTimeDataField(value, time_zone, listener);
  edit->SetDataField(df);
  return edit;
}

void
RowFormWidget::LoadValue(unsigned i, int value) noexcept
{
  WndProperty &control = GetControl(i);
  DataFieldInteger &df = *(DataFieldInteger *)control.GetDataField();
  assert(df.GetType() == DataField::Type::INTEGER);
  df.SetValue(value);
  control.RefreshDisplay();
}

void
RowFormWidget::LoadValue(unsigned i, bool value) noexcept
{
  WndProperty &control = GetControl(i);
  DataFieldBoolean &df = *(DataFieldBoolean *)control.GetDataField();
  assert(df.GetType() == DataField::Type::BOOLEAN);
  df.SetValue(value);
  control.RefreshDisplay();
}

void
RowFormWidget::LoadValueEnum(unsigned i, const char *text) noexcept
{
  WndProperty &control = GetControl(i);
  DataFieldEnum &df = *(DataFieldEnum *)control.GetDataField();
  assert(df.GetType() == DataField::Type::ENUM);
  if (df.SetValue(text))
    control.RefreshDisplay();
}

void
RowFormWidget::LoadValueEnum(unsigned i, unsigned value) noexcept
{
  WndProperty &control = GetControl(i);
  DataFieldEnum &df = *(DataFieldEnum *)control.GetDataField();
  assert(df.GetType() == DataField::Type::ENUM);
  df.SetValue(value);
  control.RefreshDisplay();
}

void
RowFormWidget::LoadValue(unsigned i, double value) noexcept
{
  WndProperty &control = GetControl(i);
  DataFieldFloat &df = *(DataFieldFloat *)control.GetDataField();
  assert(df.GetType() == DataField::Type::REAL);
  df.SetValue(value);
  control.RefreshDisplay();
}

void
RowFormWidget::LoadValue(unsigned i, const char *value) noexcept
{
  WndProperty &control = GetControl(i);
  DataFieldString &df = *(DataFieldString *)control.GetDataField();
  assert(df.GetType() == DataField::Type::STRING ||
         df.GetType() == DataField::Type::PREFIX);
  df.SetValue(value);
  control.RefreshDisplay();
}

void
RowFormWidget::LoadValue(unsigned i, Angle value) noexcept
{
  WndProperty &control = GetControl(i);
  AngleDataField &df = *(AngleDataField *)control.GetDataField();
  assert(df.GetType() == DataField::Type::ANGLE);
  df.SetValue(value);
  control.RefreshDisplay();
}

void
RowFormWidget::LoadValue(unsigned i, RoughTime value) noexcept
{
  WndProperty &control = GetControl(i);
  RoughTimeDataField &df = *(RoughTimeDataField *)control.GetDataField();
  df.SetValue(value);
  control.RefreshDisplay();
}

void
RowFormWidget::LoadValueDuration(unsigned i, std::chrono::seconds value) noexcept
{
  WndProperty &control = GetControl(i);
  DataFieldTime &df = *(DataFieldTime *)control.GetDataField();
  assert(df.GetType() == DataField::Type::TIME);
  df.SetValue(value);
  control.RefreshDisplay();
}

bool
RowFormWidget::GetValueBoolean(unsigned i) const noexcept
{
  const DataFieldBoolean &df =
    (const DataFieldBoolean &)GetDataField(i);
  assert(df.GetType() == DataField::Type::BOOLEAN);
  return df.GetValue();
}

int
RowFormWidget::GetValueInteger(unsigned i) const noexcept
{
  auto &df = static_cast<const DataFieldInteger &>(GetDataField(i));
  assert(df.GetType() == DataField::Type::INTEGER);
  return df.GetValue();
}

double
RowFormWidget::GetValueFloat(unsigned i) const noexcept
{
  const DataFieldFloat &df =
    (const DataFieldFloat &)GetDataField(i);
  assert(df.GetType() == DataField::Type::REAL);
  return df.GetValue();
}

Angle
RowFormWidget::GetValueAngle(unsigned i) const noexcept
{
  const AngleDataField &df =
    (const AngleDataField &)GetDataField(i);
  assert(df.GetType() == DataField::Type::ANGLE);
  return df.GetValue();
}

unsigned
RowFormWidget::GetValueIntegerAngle(unsigned i) const noexcept
{
  const AngleDataField &df =
    (const AngleDataField &)GetDataField(i);
  assert(df.GetType() == DataField::Type::ANGLE);
  return df.GetIntegerValue();
}

unsigned
RowFormWidget::GetValueEnum(unsigned i) const noexcept
{
  auto &df = static_cast<const DataFieldEnum &>(GetDataField(i));
  assert(df.GetType() == DataField::Type::ENUM);
  return df.GetValue();
}

std::chrono::seconds
RowFormWidget::GetValueTime(unsigned i) const noexcept
{
  const auto &df = (const DataFieldTime &)GetDataField(i);
  assert(df.GetType() == DataField::Type::TIME);
  return df.GetValue();
}

RoughTime
RowFormWidget::GetValueRoughTime(unsigned i) const noexcept
{
  const RoughTimeDataField &df =
    (const RoughTimeDataField &)GetDataField(i);
  assert(df.GetType() == DataField::Type::ROUGH_TIME);
  return df.GetValue();
}

bool
RowFormWidget::SaveValue(unsigned i, bool &value, bool negated) const noexcept
{
  bool new_value = GetValueBoolean(i);
  if (negated)
    new_value = !new_value;
  if (new_value == value)
    return false;

  value = new_value;
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, double &value) const noexcept
{
  auto new_value = GetValueFloat(i);
  if (new_value == value)
    return false;

  value = new_value;
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, Angle &value_r) const noexcept
{
  unsigned old_value = AngleDataField::Import(value_r);
  unsigned new_value = GetValueIntegerAngle(i);
  if (new_value == old_value)
    return false;

  value_r = GetValueAngle(i);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i,
                         std::chrono::seconds &value_r) const noexcept
{
  const auto new_value = GetValueTime(i);
  if (new_value == value_r)
    return false;

  value_r = new_value;
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, RoughTime &value_r) const noexcept
{
  const auto new_value = GetValueRoughTime(i);
  if (new_value == value_r)
    return false;

  value_r = new_value;
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i,
                         char *string, size_t max_size) const noexcept
{
  const char *new_value = GetDataField(i).GetAsString();
  assert(new_value != nullptr);

  if (StringIsEqual(string, new_value))
    return false;

  CopyTruncateString(string, max_size, new_value);
  return true;
}
