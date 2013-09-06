/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Form/DataField/FileReader.hpp"
#include "Form/DataField/Time.hpp"
#include "Form/DataField/RoughTime.hpp"
#include "Time/RoughTime.hpp"
#include "Language/Language.hpp"
#include "Math/Angle.hpp"

#include <assert.h>

WndProperty *
RowFormWidget::CreateEdit(const TCHAR *label, const TCHAR *help,
                          bool read_only)
{
  assert(IsDefined());

  const PixelRect edit_rc =
    InitialControlRect(Layout::GetMinimumControlHeight());

  WindowStyle style;
  if (!read_only)
    style.TabStop();

  ContainerWindow &panel = (ContainerWindow &)GetWindow();
  WndProperty *edit =
    new WndProperty(panel, look, label,
                    edit_rc, (*label == '\0') ? 0 : 100,
                    style);
  edit->SetReadOnly(read_only);

  if (help != NULL)
    edit->SetHelpText(help);

  return edit;
}

WndProperty *
RowFormWidget::Add(const TCHAR *label, const TCHAR *help, bool read_only)
{
  WndProperty *edit = CreateEdit(label, help, read_only);
  Add(Row::Type::EDIT, edit);
  return edit;
}

void
RowFormWidget::AddReadOnly(const TCHAR *label, const TCHAR *help,
                           const TCHAR *text)
{
  WndProperty *control = Add(label, help, true);
  if (text != NULL)
    control->SetText(text);
}

void
RowFormWidget::AddReadOnly(const TCHAR *label, const TCHAR *help,
                           const TCHAR *display_format,
                           fixed value)
{
  WndProperty *edit = Add(label, help, true);
  DataFieldFloat *df = new DataFieldFloat(display_format, display_format,
                                          fixed(0), fixed(0),
                                          value, fixed(1), false, NULL);
  edit->SetDataField(df);
}

void
RowFormWidget::AddReadOnly(const TCHAR *label, const TCHAR *help,
                           bool value)
{
  WndProperty *edit = Add(label, help, true);
  DataFieldBoolean *df = new DataFieldBoolean(value, _("On"), _("Off"),
                                              nullptr);
  edit->SetDataField(df);
}

WndProperty *
RowFormWidget::Add(const TCHAR *label, const TCHAR *help,
                   DataField *df)
{
  WndProperty *edit = Add(label, help);
  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddBoolean(const TCHAR *label, const TCHAR *help,
                          bool value,
                          DataField::DataAccessCallback callback)
{
  WndProperty *edit = Add(label, help);
  DataFieldBoolean *df = new DataFieldBoolean(value, _("On"), _("Off"),
                                              callback);
  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddInteger(const TCHAR *label, const TCHAR *help,
                          const TCHAR *display_format,
                          const TCHAR *edit_format,
                          int min_value, int max_value, int step, int value,
                          DataField::DataAccessCallback callback)
{
  WndProperty *edit = Add(label, help);
  DataFieldInteger *df = new DataFieldInteger(edit_format, display_format,
                                              min_value, max_value,
                                              value, step, callback);
  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddFloat(const TCHAR *label, const TCHAR *help,
                        const TCHAR *display_format,
                        const TCHAR *edit_format,
                        fixed min_value, fixed max_value,
                        fixed step, bool fine,
                        fixed value,
                        DataField::DataAccessCallback callback)
{
  WndProperty *edit = Add(label, help);
  DataFieldFloat *df = new DataFieldFloat(edit_format, display_format,
                                          min_value, max_value,
                                          value, step, fine, callback);
  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddAngle(const TCHAR *label, const TCHAR *help,
                        Angle value, unsigned step, bool fine,
                        DataFieldListener *listener)
{
  WndProperty *edit = Add(label, help);
  AngleDataField *df = new AngleDataField(value, step, fine, listener);
  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddEnum(const TCHAR *label, const TCHAR *help,
                       const StaticEnumChoice *list, unsigned value,
                       DataField::DataAccessCallback callback)
{
  assert(list != NULL);

  WndProperty *edit = Add(label, help);
  DataFieldEnum *df = new DataFieldEnum(callback);

  if (list[0].help != NULL)
    df->EnableItemHelp(true);

  df->AddChoices(list);
  df->Set(value);

  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddEnum(const TCHAR *label, const TCHAR *help,
                       DataField::DataAccessCallback callback)
{
  WndProperty *edit = Add(label, help);
  DataFieldEnum *df = new DataFieldEnum(callback);

  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddText(const TCHAR *label, const TCHAR *help,
                       const TCHAR *content)
{
  WndProperty *edit = Add(label, help);
  DataFieldString *df = new DataFieldString(content, NULL);

  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddPassword(const TCHAR *label, const TCHAR *help,
                           const TCHAR *content)
{
  WndProperty *edit = Add(label, help);
  PasswordDataField *df = new PasswordDataField(content);

  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddTime(const TCHAR *label, const TCHAR *help,
                       int min_value, int max_value, unsigned step,
                       int value, unsigned max_tokens,
                       DataField::DataAccessCallback callback)
{
  WndProperty *edit = Add(label, help);
  DataFieldTime *df = new DataFieldTime(min_value, max_value, value,
                                        step, callback);
  df->SetMaxTokenNumber(max_tokens);
  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddRoughTime(const TCHAR *label, const TCHAR *help,
                            RoughTime value, RoughTimeDelta time_zone,
                            DataFieldListener *listener)
{
  WndProperty *edit = Add(label, help);
  RoughTimeDataField *df = new RoughTimeDataField(value, time_zone, listener);
  edit->SetDataField(df);
  return edit;
}

void
RowFormWidget::LoadValue(unsigned i, int value)
{
  WndProperty &control = GetControl(i);
  DataFieldInteger &df = *(DataFieldInteger *)control.GetDataField();
  assert(df.GetType() == DataField::Type::INTEGER);
  df.Set(value);
  control.RefreshDisplay();
}

void
RowFormWidget::LoadValue(unsigned i, bool value)
{
  WndProperty &control = GetControl(i);
  DataFieldBoolean &df = *(DataFieldBoolean *)control.GetDataField();
  assert(df.GetType() == DataField::Type::BOOLEAN);
  df.Set(value);
  control.RefreshDisplay();
}

void
RowFormWidget::LoadValueEnum(unsigned i, unsigned value)
{
  WndProperty &control = GetControl(i);
  DataFieldEnum &df = *(DataFieldEnum *)control.GetDataField();
  assert(df.GetType() == DataField::Type::ENUM);
  df.Set(value);
  control.RefreshDisplay();
}

void
RowFormWidget::LoadValue(unsigned i, fixed value)
{
  WndProperty &control = GetControl(i);
  DataFieldFloat &df = *(DataFieldFloat *)control.GetDataField();
  assert(df.GetType() == DataField::Type::REAL);
  df.Set(value);
  control.RefreshDisplay();
}

void
RowFormWidget::LoadValue(unsigned i, Angle value)
{
  WndProperty &control = GetControl(i);
  AngleDataField &df = *(AngleDataField *)control.GetDataField();
  assert(df.GetType() == DataField::Type::ANGLE);
  df.SetValue(value);
  control.RefreshDisplay();
}

void
RowFormWidget::LoadValue(unsigned i, RoughTime value)
{
  WndProperty &control = GetControl(i);
  RoughTimeDataField &df = *(RoughTimeDataField *)control.GetDataField();
  df.SetValue(value);
  control.RefreshDisplay();
}

void
RowFormWidget::LoadValueTime(unsigned i, int value)
{
  WndProperty &control = GetControl(i);
  DataFieldTime &df = *(DataFieldTime *)control.GetDataField();
  assert(df.GetType() == DataField::Type::TIME);
  df.Set(value);
  control.RefreshDisplay();
}

bool
RowFormWidget::GetValueBoolean(unsigned i) const
{
  const DataFieldBoolean &df =
    (const DataFieldBoolean &)GetDataField(i);
  assert(df.GetType() == DataField::Type::BOOLEAN);
  return df.GetAsBoolean();
}

int
RowFormWidget::GetValueInteger(unsigned i) const
{
  return GetDataField(i).GetAsInteger();
}

fixed
RowFormWidget::GetValueFloat(unsigned i) const
{
  const DataFieldFloat &df =
    (const DataFieldFloat &)GetDataField(i);
  assert(df.GetType() == DataField::Type::REAL);
  return df.GetAsFixed();
}

Angle
RowFormWidget::GetValueAngle(unsigned i) const
{
  const AngleDataField &df =
    (const AngleDataField &)GetDataField(i);
  assert(df.GetType() == DataField::Type::ANGLE);
  return df.GetValue();
}

unsigned
RowFormWidget::GetValueIntegerAngle(unsigned i) const
{
  const AngleDataField &df =
    (const AngleDataField &)GetDataField(i);
  assert(df.GetType() == DataField::Type::ANGLE);
  return df.GetIntegerValue();
}

RoughTime
RowFormWidget::GetValueRoughTime(unsigned i) const
{
  const RoughTimeDataField &df =
    (const RoughTimeDataField &)GetDataField(i);
  assert(df.GetType() == DataField::Type::ROUGH_TIME);
  return df.GetValue();
}

bool
RowFormWidget::SaveValue(unsigned i, bool &value, bool negated) const
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
RowFormWidget::SaveValue(unsigned i, int &value) const
{
  int new_value = GetValueInteger(i);
  if (new_value == value)
    return false;

  value = new_value;
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, uint8_t &value) const
{
  int new_value = GetValueInteger(i);
  if (new_value == value || new_value < 0)
    return false;

  value = (uint8_t)new_value;
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, uint16_t &value) const
{
  int new_value = GetValueInteger(i);
  if (new_value == value || new_value < 0)
    return false;

  value = (uint16_t)new_value;
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, fixed &value) const
{
  fixed new_value = GetValueFloat(i);
  if (new_value == value)
    return false;

  value = new_value;
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, Angle &value_r) const
{
  unsigned old_value = AngleDataField::Import(value_r);
  unsigned new_value = GetValueIntegerAngle(i);
  if (new_value == old_value)
    return false;

  value_r = GetValueAngle(i);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, RoughTime &value_r) const
{
  const auto new_value = GetValueRoughTime(i);
  if (new_value == value_r)
    return false;

  value_r = new_value;
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, TCHAR *string, size_t max_size) const
{
  const TCHAR *new_value = GetDataField(i).GetAsString();
  assert(new_value != NULL);

  if (_tcscmp(string, new_value) == 0)
    return false;

  CopyString(string, new_value, max_size);
  return true;
}
