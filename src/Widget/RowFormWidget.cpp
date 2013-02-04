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
#include "Form/Panel.hpp"
#include "Form/Button.hpp"
#include "Form/HLine.hpp"
#include "Look/DialogLook.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "UIGlobals.hpp"
#include "Screen/Layout.hpp"
#include "Screen/LargeTextWindow.hpp"
#include "Screen/Font.hpp"
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
#include "Profile/Profile.hpp"
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"
#include "LocalPath.hpp"
#include "Math/Angle.hpp"

#include <windef.h>
#include <assert.h>
#include <limits.h>

unsigned
RowFormWidget::Row::GetMinimumHeight(bool vertical) const
{
  switch (type) {
  case Type::DUMMY:
    return 0;

  case Type::WIDGET:
    return widget->GetMinimumSize().cy;

  case Type::GENERIC:
    break;

  case Type::EDIT:
    if (vertical && GetControl().HasCaption())
      return 2 * Layout::GetMinimumControlHeight();

    /* fall through */

  case Type::BUTTON:
    return Layout::GetMinimumControlHeight();

  case Type::MULTI_LINE:
    return Layout::GetMinimumControlHeight();

  case Type::REMAINING:
    return Layout::GetMinimumControlHeight();
  }

  return window->GetHeight();
}

unsigned
RowFormWidget::Row::GetMaximumHeight(bool vertical) const
{
  switch (type) {
  case Type::DUMMY:
    return 0;

  case Type::WIDGET:
    return widget->GetMaximumSize().cy;

  case Type::GENERIC:
    break;

  case Type::EDIT:
    if (vertical && GetControl().HasCaption())
      return 2 * Layout::GetMinimumControlHeight();

    return GetControl().IsReadOnly()
      /* rows that are not clickable don't need to be extra-large */
      ? Layout::GetMinimumControlHeight()
      : Layout::GetMaximumControlHeight();

  case Type::BUTTON:
    return Layout::GetMaximumControlHeight();

  case Type::MULTI_LINE:
    return Layout::GetMinimumControlHeight() * 3;

  case Type::REMAINING:
    return 4096;
  }

  return window->GetHeight();
}

RowFormWidget::RowFormWidget(const DialogLook &_look, bool _vertical)
  :look(_look), vertical(_vertical)
{
}

RowFormWidget::~RowFormWidget()
{
  if (IsDefined()) {
    PanelControl *panel = (PanelControl *)GetWindow();
    delete panel;
  }

  /* destroy all rows */
  for (auto &i : rows)
    i.Delete();
}

void
RowFormWidget::SetRowAvailable(unsigned i, bool available)
{
  Row &row = rows[i];
  if (available == row.available)
    return;

  row.available = available;
  UpdateLayout();
}

void
RowFormWidget::SetRowVisible(unsigned i, bool visible)
{
  Row &row = rows[i];
  if (visible == row.visible)
    return;

  row.visible = visible;
  if (!visible)
    row.GetWindow().Hide();
  else if (row.IsAvailable(UIGlobals::GetDialogSettings().expert))
    row.GetWindow().Show();
}

void
RowFormWidget::SetExpertRow(unsigned i)
{
  Row &row = rows[i];
  assert(!row.expert);
  row.expert = true;
}

void
RowFormWidget::Add(Row::Type type, Window *window)
{
  assert(IsDefined());
#ifndef USE_GDI
  assert(window->GetParent() == GetWindow());
#endif
  assert(window->IsVisible());
  /* cannot append rows after a REMAINING row */
  assert(rows.empty() || rows.back().type != Row::Type::REMAINING);

  rows.push_back(Row(type, window));
}

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

  PanelControl &panel = *(PanelControl *)GetWindow();
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
                           const TCHAR *display_format,
                           UnitGroup unit_group, fixed value)
{
  WndProperty *edit = Add(label, help, true);
  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  value = Units::ToUserUnit(value, unit);
  DataFieldFloat *df = new DataFieldFloat(display_format, display_format,
                                          fixed(0), fixed(0),
                                          value, fixed(1), false, NULL);
  df->SetUnits(Units::GetUnitName(unit));
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
RowFormWidget::AddFloat(const TCHAR *label, const TCHAR *help,
                        const TCHAR *display_format,
                        const TCHAR *edit_format,
                        fixed min_value, fixed max_value,
                        fixed step, bool fine,
                        UnitGroup unit_group, fixed value,
                        DataField::DataAccessCallback callback)
{
  WndProperty *edit = Add(label, help);
  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  value = Units::ToUserUnit(value, unit);
  DataFieldFloat *df = new DataFieldFloat(edit_format, display_format,
                                          min_value, max_value,
                                          value, step, fine, callback);
  df->SetUnits(Units::GetUnitName(unit));
  edit->SetDataField(df);
  return edit;
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
                            RoughTime value, DataFieldListener *listener)
{
  WndProperty *edit = Add(label, help);
  RoughTimeDataField *df = new RoughTimeDataField(value, listener);
  edit->SetDataField(df);
  return edit;
}

void
RowFormWidget::AddSpacer()
{
  assert(IsDefined());

  HLine *window = new HLine(GetLook());
  ContainerWindow &panel = *(ContainerWindow *)GetWindow();
  const PixelRect rc = InitialControlRect(Layout::Scale(3));
  window->Create(panel, rc);
  Add(window);
}

WndProperty *
RowFormWidget::AddFileReader(const TCHAR *label, const TCHAR *help,
                             const TCHAR *registry_key, const TCHAR *filters,
                             bool nullable)
{
  WndProperty *edit = Add(label, help);
  DataFieldFileReader *df = new DataFieldFileReader(NULL);
  edit->SetDataField(df);

  if (nullable)
    df->AddNull();

  size_t length;
  while ((length = _tcslen(filters)) > 0) {
    df->ScanDirectoryTop(filters);
    filters += length + 1;
  }

  TCHAR path[MAX_PATH];
  if (Profile::GetPath(registry_key, path))
    df->Lookup(path);

  edit->RefreshDisplay();

  return edit;
}

void
RowFormWidget::AddMultiLine(const TCHAR *text)
{
  assert(IsDefined());

  const PixelRect rc =
    InitialControlRect(Layout::GetMinimumControlHeight());

  LargeTextWindowStyle style;
  if (IsEmbedded() || Layout::scale_1024 < 2048)
    /* sunken edge doesn't fit well on the tiny screen of an embedded
       device */
    style.Border();
  else
    style.SunkenEdge();

  PanelControl &panel = *(PanelControl *)GetWindow();
  LargeTextWindow *ltw = new LargeTextWindow();
  ltw->Create(panel, rc, style);
  ltw->SetFont(*look.text_font);

  if (text != nullptr)
    ltw->SetText(text);

  Add(Row::Type::MULTI_LINE, ltw);
}

void
RowFormWidget::AddButton(const TCHAR *label, ActionListener &listener, int id)
{
  assert(IsDefined());

  const PixelRect button_rc =
    InitialControlRect(Layout::GetMinimumControlHeight());

  ButtonWindowStyle button_style;
  button_style.TabStop();
  button_style.multiline();

  ContainerWindow &panel = *(ContainerWindow *)GetWindow();

  WndButton *button = new WndButton(panel, look, label, button_rc, button_style, listener, id);

  Add(Row::Type::BUTTON, button);
}

void
RowFormWidget::SetMultiLineText(unsigned i, const TCHAR *text)
{
  assert(text != nullptr);
  assert(rows[i].type == Row::Type::MULTI_LINE);

  LargeTextWindow &ltw = *(LargeTextWindow *)rows[i].window;
  ltw.SetText(text);
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
RowFormWidget::LoadValueEnum(unsigned i, int value)
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
  LoadValue(i, value.Degrees());
}

void
RowFormWidget::LoadValue(unsigned i, fixed value, UnitGroup unit_group)
{
  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  WndProperty &control = GetControl(i);
  DataFieldFloat &df = *(DataFieldFloat *)control.GetDataField();
  assert(df.GetType() == DataField::Type::REAL);
  df.Set(Units::ToUserUnit(value, unit));
  df.SetUnits(Units::GetUnitName(unit));
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

bool
RowFormWidget::SaveValue(unsigned i, const TCHAR *registry_key,
                         TCHAR *string, size_t max_size) const
{
  if (!SaveValue(i, string, max_size))
    return false;

  Profile::Set(registry_key, string);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, const TCHAR *registry_key,
                         bool &value, bool negated) const
{
  if (!SaveValue(i, value, negated))
    return false;

  Profile::Set(registry_key, value);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, const TCHAR *registry_key,
                         int &value) const
{
  if (!SaveValue(i, value))
    return false;

  Profile::Set(registry_key, value);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, const TCHAR *registry_key,
                         uint8_t &value) const
{
  if (!SaveValue(i, value))
    return false;

  Profile::Set(registry_key, value);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, const TCHAR *registry_key,
                         uint16_t &value) const
{
  if (!SaveValue(i, value))
    return false;

  Profile::Set(registry_key, value);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, const TCHAR *registry_key,
                         fixed &value) const
{
  if (!SaveValue(i, value))
    return false;

  Profile::Set(registry_key, value);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, UnitGroup unit_group, fixed &value) const
{
  const DataFieldFloat &df =
    (const DataFieldFloat &)GetDataField(i);
  assert(df.GetType() == DataField::Type::REAL);

  fixed new_value = df.GetAsFixed();
  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  new_value = Units::ToSysUnit(new_value, unit);
  if (new_value > value - df.GetStep() / 2 &&
      new_value < value + df.GetStep() / 2)
    return false;

  value = new_value;
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, UnitGroup unit_group,
                         const TCHAR *registry_key, fixed &value) const
{
  const DataFieldFloat &df =
    (const DataFieldFloat &)GetDataField(i);
  assert(df.GetType() == DataField::Type::REAL);

  fixed new_value = df.GetAsFixed();
  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  new_value = Units::ToSysUnit(new_value, unit);
  if (new_value > value - df.GetStep() / 2 &&
      new_value < value + df.GetStep() / 2)
    return false;

  value = new_value;
  Profile::Set(registry_key, new_value);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, UnitGroup unit_group,
                         const TCHAR *registry_key, unsigned int &value) const
{
  const DataFieldFloat &df =
    (const DataFieldFloat &)GetDataField(i);
  assert(df.GetType() == DataField::Type::INTEGER ||
         df.GetType() == DataField::Type::REAL);

  fixed new_value = df.GetAsFixed();
  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  new_value = Units::ToSysUnit(new_value, unit);
  if ((unsigned int)(new_value) > value - iround(df.GetStep()) / 2 &&
      (unsigned int)(new_value) < value + iround(df.GetStep()) / 2)
    return false;

  value = iround(new_value);
  Profile::Set(registry_key, value);
  return true;
}

bool
RowFormWidget::SaveValueFileReader(unsigned i, const TCHAR *registry_key)
{
  const DataFieldFileReader *dfe =
    (const DataFieldFileReader *)GetControl(i).GetDataField();
  TCHAR new_value[MAX_PATH];
  _tcscpy(new_value, dfe->GetPathFile());
  ContractLocalPath(new_value);

  const TCHAR *old_value = Profile::Get(registry_key, _T(""));
  if (_tcscmp(old_value, new_value) == 0)
    return false;

  Profile::Set(registry_key, new_value);
  return true;
}

unsigned
RowFormWidget::GetRecommendedCaptionWidth() const
{
  const bool expert = UIGlobals::GetDialogSettings().expert;

  unsigned w = 0;
  for (const auto &i : rows) {
    if (!i.IsAvailable(expert))
      continue;

    if (i.type == Row::Type::EDIT) {
      unsigned x = i.GetControl().GetRecommendedCaptionWidth();
      if (x > w)
        w = x;
    }
  }

  return w;
}

void
RowFormWidget::UpdateLayout()
{
  PixelRect current_rect = GetWindow()->GetClientRect();
  const unsigned total_width = current_rect.right - current_rect.left;
  const unsigned total_height = current_rect.bottom - current_rect.top;
  current_rect.bottom = current_rect.top;

  const bool expert = UIGlobals::GetDialogSettings().expert;

  /* first row traversal: count the number of "elastic" rows and
     determine the minimum total height */
  unsigned min_height = 0;
  unsigned n_elastic = 0;
  unsigned caption_width = 0;

  for (const auto &i : rows) {
    if (!i.IsAvailable(expert))
      continue;

    min_height += i.GetMinimumHeight(vertical);
    if (i.IsElastic(vertical))
      ++n_elastic;

    if (!vertical && i.type == Row::Type::EDIT) {
      unsigned cw = i.GetControl().GetRecommendedCaptionWidth();
      if (cw > caption_width)
        caption_width = cw;
    }
  }

  if (!vertical && caption_width * 3 > total_width * 2)
    caption_width = total_width * 2 / 3;

  /* how much excess height in addition to the minimum height? */
  unsigned excess_height = min_height < total_height
    ? total_height - min_height
    : 0;

  /* second row traversal: now move and resize the rows */
  for (auto &i : rows) {
    if (!i.IsAvailable(expert)) {
      if (i.type == Row::Type::WIDGET)
        i.GetWidget().Hide();
      else if (i.type != Row::Type::DUMMY)
        i.GetWindow().Hide();

      continue;
    }

    /* determine this row's height */
    UPixelScalar height = i.GetMinimumHeight(vertical);
    if (excess_height > 0 && i.IsElastic(vertical)) {
      assert(n_elastic > 0);

      /* distribute excess height among all elastic rows */
      unsigned grow_height = excess_height / n_elastic;
      if (grow_height > 0) {
        height += grow_height;
        const UPixelScalar max_height = i.GetMaximumHeight(vertical);
        if (height > max_height) {
          /* never grow beyond declared maximum height */
          height = max_height;
          grow_height = max_height - height;
        }

        excess_height -= grow_height;
      }

      --n_elastic;
    }

    if (i.type == Row::Type::WIDGET) {
      Widget &widget = i.GetWidget();

      /* TODO: visible check - hard to implement without remembering
         the control position, because Widget::Show() wants a
         PixelRect parameter */

      NextControlRect(current_rect, height);

      if (!i.initialised) {
        i.initialised = true;
        widget.Initialise(*(ContainerWindow *)GetWindow(), current_rect);
      }

      if (!i.prepared) {
        i.prepared = true;
        widget.Prepare(*(ContainerWindow *)GetWindow(), current_rect);
      }

      widget.Show(current_rect);
      continue;
    }

    Window &window = i.GetWindow();

    if (i.visible)
      window.Show();

    if (i.type == Row::Type::EDIT &&
        i.GetControl().HasCaption()) {
      if (vertical)
        i.GetControl().SetCaptionWidth(-1);
      else if (caption_width > 0)
        i.GetControl().SetCaptionWidth(caption_width);
    }

    /* finally move and resize */
    NextControlRect(current_rect, height);
    window.Move(current_rect);
  }

  assert(excess_height == 0 || n_elastic == 0);
}

PixelSize
RowFormWidget::GetMinimumSize() const
{
  const unsigned value_width =
    look.text_font->TextSize(_T("Foo Bar Foo Bar")).cx;

  const bool expert = UIGlobals::GetDialogSettings().expert;

  const unsigned edit_width = vertical
    ? std::max(GetRecommendedCaptionWidth(), value_width)
    : (GetRecommendedCaptionWidth() + value_width);

  PixelSize size(edit_width, 0u);
  for (const auto &i : rows)
    if (i.IsAvailable(expert))
      size.cy += i.GetMinimumHeight(vertical);

  return size;
}

PixelSize
RowFormWidget::GetMaximumSize() const
{
  const unsigned value_width =
    look.text_font->TextSize(_T("Foo Bar Foo Bar")).cx * 2;

  const unsigned edit_width = vertical
    ? std::max(GetRecommendedCaptionWidth(), value_width)
    : (GetRecommendedCaptionWidth() + value_width);

  PixelSize size(edit_width, 0u);
  for (const auto &i : rows)
    size.cy += i.GetMaximumHeight(vertical);

  return size;
}

void
RowFormWidget::Initialise(ContainerWindow &parent, const PixelRect &rc)
{
  assert(!IsDefined());
  assert(rows.empty());

  WindowStyle style;
  style.Hide();
  style.ControlParent();

  SetWindow(new PanelControl(parent, look, rc, style));
}

void
RowFormWidget::Show(const PixelRect &rc)
{
  PanelControl &panel = *(PanelControl *)GetWindow();
  panel.Move(rc);

  UpdateLayout();

  panel.Show();
}

void
RowFormWidget::Move(const PixelRect &rc)
{
  PanelControl &panel = *(PanelControl *)GetWindow();
  panel.Move(rc);

  UpdateLayout();
}

bool
RowFormWidget::SetFocus()
{
  if (rows.empty())
    return false;

  PanelControl &panel = *(PanelControl *)GetWindow();
  return panel.FocusFirstControl();
}
