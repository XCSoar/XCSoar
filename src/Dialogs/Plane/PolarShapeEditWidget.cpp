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

#include "PolarShapeEditWidget.hpp"
#include "Form/Frame.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Float.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Font.hpp"
#include "UIGlobals.hpp"
#include "Util/Macros.hpp"
#include "Language/Language.hpp"
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"

#include <algorithm>

#include <assert.h>

PolarShapeEditWidget::PolarShapeEditWidget(const PolarShape &_shape,
                                           DataFieldListener *_listener)
  :shape(_shape), listener(_listener) {}

static void
LoadValue(WndProperty &e, double value, UnitGroup unit_group)
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

static double
GetValue(WndProperty &e)
{
  return ((DataFieldFloat *)e.GetDataField())->GetAsFixed();
}

static bool
SaveValue(WndProperty &e, double &value_r, UnitGroup unit_group)
{
  const Unit unit = Units::GetUserUnitByGroup(unit_group);

  auto new_value = Units::ToSysUnit(GetValue(e), unit);
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

PixelSize
PolarShapeEditWidget::GetMinimumSize() const
{
  return { Layout::Scale(200u),
      2 * Layout::GetMinimumControlHeight() };
}

PixelSize
PolarShapeEditWidget::GetMaximumSize() const
{
  return { Layout::Scale(400u),
      2 * Layout::GetMaximumControlHeight() };
}

void
PolarShapeEditWidget::Prepare(ContainerWindow &parent, const PixelRect &_rc)
{
  PanelWidget::Prepare(parent, _rc);
  const DialogLook &look = UIGlobals::GetDialogLook();
  ContainerWindow &panel = (ContainerWindow &)GetWindow();

  const unsigned width = _rc.GetWidth(), height = _rc.GetHeight();

  const TCHAR *v_text = _("Polar V");
  const TCHAR *w_text = _("Polar W");

  const unsigned row_height = height / 2;
  const unsigned label_width = 2 * Layout::GetTextPadding() +
    std::max(look.text_font.TextSize(v_text).cx,
             look.text_font.TextSize(w_text).cx);
  const unsigned edit_width = (width - label_width) / ARRAY_SIZE(points);

  WindowStyle style;
  style.TabStop();

  PixelRect label_rc(0, 0, label_width, row_height);
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
                                  rc, 0, style);
    DataFieldFloat *df = new DataFieldFloat(_T("%.0f"), _T("%.0f %s"),
                                            0, 300, 0, 1, false,
                                            listener);
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

  double step = 0.05, min = -10;
  switch (Units::current.vertical_speed_unit) {
  case Unit::FEET_PER_MINUTE:
    step = 10;
    min = -2000;
    break;

  case Unit::KNOTS:
    step = 0.1;
    min = -20;
    break;

  default:
    break;
  }

  for (unsigned i = 0; i < ARRAY_SIZE(points);
       ++i, rc.left += edit_width, rc.right += edit_width) {
    points[i].w = new WndProperty(panel, look, _T(""),
                                  rc, 0, style);
    DataFieldFloat *df = new DataFieldFloat(_T("%.2f"), _T("%.2f %s"),
                                            min, 0, 0, step, false,
                                            listener);

    points[i].w->SetDataField(df);
  }

  for (unsigned i = 0; i < ARRAY_SIZE(points); ++i)
    LoadPoint(points[i], shape[i]);
}

void
PolarShapeEditWidget::Unprepare()
{
  for (auto &i : points) {
    delete i.v;
    delete i.w;
  }

  PanelWidget::Unprepare();
}

bool
PolarShapeEditWidget::Save(bool &_changed)
{
  bool changed = false;

  for (unsigned i = 0; i < ARRAY_SIZE(points); ++i)
    changed |= SavePoint(points[i], shape[i]);

  _changed |= changed;
  return true;
}
