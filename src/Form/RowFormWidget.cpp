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

#include "Form/RowFormWidget.hpp"
#include "Form/Edit.hpp"
#include "Form/Panel.hpp"
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

RowFormWidget::RowFormWidget(const DialogLook &_look,
                             UPixelScalar _caption_width)
  :look(_look), caption_width(_caption_width)
{
}

RowFormWidget::~RowFormWidget()
{
  PanelControl *panel = (PanelControl *)GetWindow();
  delete panel;
}

void
RowFormWidget::Add(WndProperty *edit)
{
  assert(IsDefined());
#ifndef USE_GDI
  assert(edit->GetParent() == GetWindow());
#endif
  assert(edit->is_visible());

  controls.push_back(edit);
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

  Add(edit);
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
RowFormWidget::Initialise(ContainerWindow &parent, const PixelRect &rc)
{
  assert(!IsDefined());
  assert(controls.empty());

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

  PixelRect current_rect = panel.get_client_rect();
  current_rect.bottom = current_rect.top;

  for (auto i = controls.begin(), end = controls.end(); i != end; ++i) {
    NextControlRect(current_rect, (*i)->get_height());
    (*i)->move(current_rect);
  }

  panel.show();
}

void
RowFormWidget::Hide()
{
  PanelControl &panel = *(PanelControl *)GetWindow();
  panel.fast_hide();
}

void
RowFormWidget::Move(const PixelRect &rc)
{
  PanelControl &panel = *(PanelControl *)GetWindow();
  panel.move(rc);

  PixelRect current_rect = panel.get_client_rect();
  current_rect.bottom = current_rect.top;

  for (auto i = controls.begin(), end = controls.end(); i != end; ++i) {
    NextControlRect(current_rect, (*i)->get_height());
    (*i)->move(current_rect);
  }
}

bool
RowFormWidget::SetFocus()
{
  if (controls.empty())
    return false;

  GetControl(0).set_focus();
  return true;
}
