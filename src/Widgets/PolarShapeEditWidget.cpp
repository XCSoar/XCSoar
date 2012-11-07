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

#include "PolarShapeEditWidget.hpp"
#include "Form/Frame.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Float.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"
#include "Util/Macros.hpp"
#include "Language/Language.hpp"
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"

#include <algorithm>

PolarShapeEditWidget::PolarShapeEditWidget(const PolarShape &_shape)
  :shape(_shape) {}

static void
LoadValue(WndProperty &e, fixed value, UnitGroup unit_group)
{
  const Unit unit = Units::GetUserUnitByGroup(unit_group);

  DataFieldFloat &df = *(DataFieldFloat *)e.GetDataField();
  assert(df.GetType() == DataField::Type::REAL);
  df.SetUnits(Units::GetUnitName(unit));
  df.Set(Units::ToUserUnit(value, unit));

  e.RefreshDisplay();
}

static void
LoadPoint(PolarShapeEditWidget::PointEditor &pe, const PolarPoint &point)
{
  LoadValue(*pe.v, point.v, UnitGroup::HORIZONTAL_SPEED);
  LoadValue(*pe.w, point.w, UnitGroup::VERTICAL_SPEED);
}

static fixed
GetValue(WndProperty &e)
{
  return ((DataFieldFloat *)e.GetDataField())->GetAsFixed();
}

static bool
SaveValue(WndProperty &e, fixed &value_r, UnitGroup unit_group)
{
  const Unit unit = Units::GetUserUnitByGroup(unit_group);

  fixed new_value = Units::ToSysUnit(GetValue(e), unit);
  if (new_value == value_r)
    return false;

  value_r = new_value;
  return true;
}

static bool
SavePoint(const PolarShapeEditWidget::PointEditor &pe, PolarPoint &point)
{
  bool changed = false;
  changed |= SaveValue(*pe.v, point.v, UnitGroup::HORIZONTAL_SPEED);
  changed |= SaveValue(*pe.w, point.w, UnitGroup::VERTICAL_SPEED);
  return changed;
}

void
PolarShapeEditWidget::SetPolarShape(const PolarShape &_shape)
{
  shape = _shape;

  for (unsigned i = 0; i < ARRAY_SIZE(points); ++i)
    LoadPoint(points[i], shape[i]);
}

static void
SetDataAccessCallback(WndProperty &e, DataField::DataAccessCallback callback)
{
  DataField &df = *(DataField *)e.GetDataField();
  df.SetDataAccessCallback(callback);
}

void
PolarShapeEditWidget::SetDataAccessCallback(DataField::DataAccessCallback callback)
{
  for (unsigned i = 0; i < ARRAY_SIZE(points); ++i) {
    ::SetDataAccessCallback(*points[i].v, callback);
    ::SetDataAccessCallback(*points[i].w, callback);
  }
}

void
PolarShapeEditWidget::Prepare(ContainerWindow &parent, const PixelRect &_rc)
{
  PanelWidget::Prepare(parent, _rc);
  const DialogLook &look = UIGlobals::GetDialogLook();
  ContainerWindow &panel = *(ContainerWindow *)GetWindow();

  const UPixelScalar width = _rc.right - _rc.left;
  const UPixelScalar height = _rc.bottom - _rc.top;

  const TCHAR *v_text = _("Polar V");
  const TCHAR *w_text = _("Polar W");

  const UPixelScalar row_height = height / 2;
  const UPixelScalar label_width = Layout::Scale(8) +
    std::max(look.text_font->TextSize(v_text).cx,
             look.text_font->TextSize(v_text).cx);
  const UPixelScalar edit_width = (width - label_width) / ARRAY_SIZE(points);

  WindowStyle style;
  style.ControlParent();

  EditWindowStyle edit_style;
  edit_style.SetVerticalCenter();
  edit_style.TabStop();

  PixelRect label_rc { 0, 0, PixelScalar(label_width),
      PixelScalar(row_height) };
  v_label = new WndFrame(panel, look, label_rc);
  v_label->SetText(v_text);

  PixelRect rc;
  rc.left = label_width;
  rc.top = 0;
  rc.right = rc.left + edit_width;
  rc.bottom = row_height;
  for (unsigned i = 0; i < ARRAY_SIZE(points);
       ++i, rc.left += edit_width, rc.right += edit_width) {
    points[i].v = new WndProperty(panel, look, _T(""),
                                  rc, 0, style, edit_style);
    DataFieldFloat *df = new DataFieldFloat(_T("%.0f"), _T("%.0f %s"),
                                            fixed_zero, fixed(300), fixed_zero,
                                            fixed_one, false, NULL);
    points[i].v->SetDataField(df);
  }

  label_rc.top += row_height;
  label_rc.bottom += row_height;
  w_label = new WndFrame(panel, look, label_rc);
  w_label->SetText(w_text);

  rc.left = label_width;
  rc.top = row_height;
  rc.right = rc.left + edit_width;
  rc.bottom = height;

  fixed step = fixed(0.05), min = fixed(-10);
  switch (Units::current.vertical_speed_unit) {
  case Unit::FEET_PER_MINUTE:
    step = fixed_ten;
    min = fixed(-2000);
    break;

  case Unit::KNOTS:
    step = fixed(0.1);
    min = fixed(-20);
    break;

  default:
    break;
  }

  for (unsigned i = 0; i < ARRAY_SIZE(points);
       ++i, rc.left += edit_width, rc.right += edit_width) {
    points[i].w = new WndProperty(panel, look, _T(""),
                                  rc, 0, style, edit_style);
    DataFieldFloat *df = new DataFieldFloat(_T("%.2f"), _T("%.2f %s"),
                                            min, fixed_zero, fixed_zero,
                                            step, false, NULL);

    points[i].w->SetDataField(df);
  }

  for (unsigned i = 0; i < ARRAY_SIZE(points); ++i)
    LoadPoint(points[i], shape[i]);
}

void
PolarShapeEditWidget::Unprepare()
{
  for (unsigned i = 0; i < ARRAY_SIZE(points); ++i) {
    delete points[i].v;
    delete points[i].w;
  }

  PanelWidget::Unprepare();
}

bool
PolarShapeEditWidget::Save(bool &_changed, bool &require_restart)
{
  bool changed = false;

  for (unsigned i = 0; i < ARRAY_SIZE(points); ++i)
    changed |= SavePoint(points[i], shape[i]);

  _changed |= changed;
  return true;
}
