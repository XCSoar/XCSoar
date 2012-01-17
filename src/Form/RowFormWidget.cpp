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

#include "Form/RowFormWidget.hpp"
#include "Form/Edit.hpp"
#include "Form/Panel.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "DataField/Boolean.hpp"
#include "DataField/Integer.hpp"
#include "DataField/Float.hpp"
#include "DataField/Enum.hpp"
#include "DataField/String.hpp"
#include "DataField/FileReader.hpp"
#include "Language/Language.hpp"
#include "Profile/Profile.hpp"
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"
#include "LocalPath.hpp"

#include <windef.h>
#include <assert.h>

/**
 * Returns the minimum height of an edit control.
 */
gcc_pure
static UPixelScalar
GetMinimumControlHeight()
{
  return Layout::Scale(22);
}

/**
 * Returns the maximum height of an edit control.
 */
gcc_pure
static UPixelScalar
GetMaximumControlHeight()
{
  if (!HasTouchScreen())
    return GetMinimumControlHeight();

  /* larger rows for touch screens */
  // XXX make this dpi-aware and use physical sizes
  return Layout::Scale(44);
}

UPixelScalar
RowFormWidget::Row::GetMinimumHeight() const
{
  switch (type) {
  case Type::GENERIC:
    break;

  case Type::EDIT:
    return GetMinimumControlHeight();
  }

  return window->get_height();
}

UPixelScalar
RowFormWidget::Row::GetMaximumHeight() const
{
  switch (type) {
  case Type::GENERIC:
    break;

  case Type::EDIT:
    return GetMaximumControlHeight();
  }

  return window->get_height();
}

RowFormWidget::RowFormWidget(const DialogLook &_look,
                             UPixelScalar _caption_width)
  :look(_look), caption_width(_caption_width)
{
}

RowFormWidget::~RowFormWidget()
{
  if (IsDefined()) {
    PanelControl *panel = (PanelControl *)GetWindow();
    delete panel;
  }

  /* destroy all rows */
  for (auto i = rows.begin(), end = rows.end(); i != end; ++i)
    i->Delete();
}

void
RowFormWidget::Add(Row::Type type, Window *window)
{
  assert(IsDefined());
#ifndef USE_GDI
  assert(window->GetParent() == GetWindow());
#endif
  assert(window->is_visible());

  rows.push_back(Row(type, window));
}

WndProperty *
RowFormWidget::Add(const TCHAR *label, const TCHAR *help, bool read_only)
{
  assert(IsDefined());

  PixelRect edit_rc = NextControlRect(GetWindow()->get_client_rect(),
                                      Layout::Scale(22));

  WindowStyle style;
  if (!read_only)
    style.ControlParent();

  EditWindowStyle edit_style;
  edit_style.vertical_center();

  if (read_only)
    edit_style.read_only();
  else
    edit_style.TabStop();

  if (IsEmbedded() || Layout::scale_1024 < 2048)
    /* sunken edge doesn't fit well on the tiny screen of an embedded
       device */
    edit_style.Border();
  else
    edit_style.SunkenEdge();

  PanelControl &panel = *(PanelControl *)GetWindow();
  WndProperty *edit =
    new WndProperty(panel, look, label,
                    edit_rc, (*label == '\0') ? 0 : caption_width,
                    style, edit_style, NULL);
  if (help != NULL)
    edit->SetHelpText(help);

  Add(Row::Type::EDIT, edit);
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
                          DataField::DataAccessCallback_t callback)
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
                          DataField::DataAccessCallback_t callback)
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
                        DataField::DataAccessCallback_t callback)
{
  WndProperty *edit = Add(label, help);
  DataFieldFloat *df = new DataFieldFloat(edit_format, display_format,
                                          min_value, max_value,
                                          value, step, fine, callback);
  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddFloat(const TCHAR *label, const TCHAR *help,
                        const TCHAR *display_format,
                        const TCHAR *edit_format,
                        fixed min_value, fixed max_value,
                        fixed step, bool fine,
                        UnitGroup unit_group, fixed value,
                        DataField::DataAccessCallback_t callback)
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
RowFormWidget::AddEnum(const TCHAR *label, const TCHAR *help,
                       const StaticEnumChoice *list, unsigned value,
                       DataField::DataAccessCallback_t callback)
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
                       DataField::DataAccessCallback_t callback)
{
  WndProperty *edit = Add(label, help);
  DataFieldEnum *df = new DataFieldEnum(callback);

  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddText(const TCHAR *label, const TCHAR *help,
                       const TCHAR *content, bool editable,
                       DataField::DataAccessCallback_t callback)
{
  WndProperty *edit = Add(label, help);
  DataFieldString *df = new DataFieldString(content, callback);

  edit->SetDataField(df);
  return edit;
}

WndProperty *
RowFormWidget::AddSpacer(void)
{
  assert(IsDefined());

  PixelRect edit_rc = NextControlRect(GetWindow()->get_client_rect(), Layout::Scale(6));

  WindowStyle style;
  EditWindowStyle edit_style;
  edit_style.vertical_center();
  edit_style.read_only();

  PanelControl &panel = *(PanelControl *)GetWindow();
  WndProperty *edit = new WndProperty(panel, look, _T(""), edit_rc, 0, style, edit_style, NULL);
  Add(edit);
  return edit;
}

WndProperty *
RowFormWidget::AddFileReader(const TCHAR *label, const TCHAR *help,
                             const TCHAR *registry_key, const TCHAR *filters)
{
  WndProperty *edit = Add(label, help);
  DataFieldFileReader *df = new DataFieldFileReader(NULL);
  edit->SetDataField(df);

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
RowFormWidget::LoadValueEnum(unsigned i, int value)
{
  WndProperty &control = GetControl(i);
  DataFieldEnum &df = *(DataFieldEnum *)control.GetDataField();
  assert(df.GetType() == DataField::TYPE_ENUM);
  df.Set(value);
  control.RefreshDisplay();
}

void
RowFormWidget::LoadValue(unsigned i, fixed value)
{
  WndProperty &control = GetControl(i);
  DataFieldFloat &df = *(DataFieldFloat *)control.GetDataField();
  assert(df.GetType() == DataField::TYPE_REAL);
  df.Set(value);
  control.RefreshDisplay();
}

void
RowFormWidget::LoadValue(unsigned i, fixed value, UnitGroup unit_group)
{
  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  WndProperty &control = GetControl(i);
  DataFieldFloat &df = *(DataFieldFloat *)control.GetDataField();
  assert(df.GetType() == DataField::TYPE_REAL);
  df.Set(Units::ToUserUnit(value, unit));
  df.SetUnits(Units::GetUnitName(unit));
  control.RefreshDisplay();
}

bool
RowFormWidget::GetValueBoolean(unsigned i) const
{
  const DataFieldBoolean &df =
    (const DataFieldBoolean &)GetDataField(i);
  assert(df.GetType() == DataField::TYPE_BOOLEAN);
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
  assert(df.GetType() == DataField::TYPE_REAL);
  return df.GetAsFixed();
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
RowFormWidget::SaveValue(unsigned i, fixed &value) const
{
  fixed new_value = GetValueFloat(i);
  if (new_value == value)
    return false;

  value = new_value;
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
  assert(df.GetType() == DataField::TYPE_REAL);

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
  assert(df.GetType() == DataField::TYPE_REAL);

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
  assert(df.GetType() == DataField::TYPE_INTEGER || df.GetType() == DataField::TYPE_REAL);

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

  TCHAR old_value[MAX_PATH];
  Profile::Get(registry_key, old_value, MAX_PATH);
  if (_tcscmp(old_value, new_value) == 0)
    return false;

  Profile::Set(registry_key, new_value);
  return true;
}

void
RowFormWidget::UpdateLayout()
{
  PixelRect current_rect = GetWindow()->get_client_rect();
  const unsigned total_height = current_rect.bottom - current_rect.top;
  current_rect.bottom = current_rect.top;

  /* first row traversal: count the number of "elastic" rows and
     determine the minimum total height */
  unsigned min_height = 0;
  unsigned n_elastic = 0;
  for (auto i = rows.begin(), end = rows.end(); i != end; ++i) {
    min_height += i->GetMinimumHeight();
    if (i->IsElastic())
      ++n_elastic;
  }

  /* how much excess height in addition to the minimum height? */
  unsigned excess_height = min_height < total_height
    ? total_height - min_height
    : 0;

  /* second row traversal: now move and resize the rows */
  for (auto i = rows.begin(), end = rows.end(); i != end; ++i) {
    /* determine this row's height */
    UPixelScalar height;
    if (excess_height > 0 && i->IsElastic()) {
      assert(n_elastic > 0);

      /* distribute excess height among all elastic rows */
      unsigned grow_height = excess_height / n_elastic;
      if (grow_height > 0) {
        height = i->GetMinimumHeight() + grow_height;
        const UPixelScalar max_height = i->GetMaximumHeight();
        if (height > max_height) {
          /* never grow beyond declared maximum height */
          height = max_height;
          grow_height = max_height - height;
        }

        excess_height -= grow_height;
      }

      --n_elastic;
    } else
      height = i->GetMinimumHeight();

    /* finally move and resize */
    Window &window = i->GetWindow();
    NextControlRect(current_rect, height);
    window.move(current_rect);
  }

  assert(excess_height == 0 || n_elastic == 0);
}

PixelSize
RowFormWidget::GetMinimumSize() const
{
  const UPixelScalar value_width =
    look.text_font->TextSize(_T("Foo Bar Foo Bar")).cx;

  PixelSize size{ PixelScalar(caption_width + value_width), 0 };
  for (auto i = rows.begin(), end = rows.end(); i != end; ++i)
    size.cy += i->GetMinimumHeight();

  return size;
}

PixelSize
RowFormWidget::GetMaximumSize() const
{
  const UPixelScalar value_width =
    look.text_font->TextSize(_T("Foo Bar Foo Bar")).cx;

  PixelSize size{ PixelScalar(caption_width + value_width), 0 };
  for (auto i = rows.begin(), end = rows.end(); i != end; ++i)
    size.cy += i->GetMaximumHeight();

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
  panel.move(rc);

  UpdateLayout();

  panel.show();
}

void
RowFormWidget::Move(const PixelRect &rc)
{
  PanelControl &panel = *(PanelControl *)GetWindow();
  panel.move(rc);

  UpdateLayout();
}

bool
RowFormWidget::SetFocus()
{
  if (rows.empty())
    return false;

  GetRow(0).set_focus();
  return true;
}
