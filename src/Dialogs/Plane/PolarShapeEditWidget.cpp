// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PolarShapeEditWidget.hpp"
#include "Form/Frame.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Float.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Font.hpp"
#include "UIGlobals.hpp"
#include "util/Macros.hpp"
#include "Language/Language.hpp"
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"

#include <algorithm>

#include <cassert>

PolarShapeEditWidget::PolarShapeEditWidget(const PolarShape &_shape,
                                           DataFieldListener *_listener) noexcept
  :shape(_shape), listener(_listener) {}

PolarShapeEditWidget::~PolarShapeEditWidget() noexcept = default;

[[gnu::pure]]
static unsigned
GetLabelWidth(const DialogLook &look) noexcept
{
  const char *v_text = C_("Glider polar coefficient", "Polar V");
  const char *w_text = C_("Glider polar coefficient", "Polar W");

  return 2 * Layout::GetTextPadding() +
    std::max(look.text_font.TextSize(v_text).width,
             look.text_font.TextSize(w_text).width);
}

[[gnu::pure]]
static unsigned
GetMinimumColumnWidth(const DialogLook &look) noexcept
{
  const unsigned padding = Layout::GetTextPadding();
  const Font &font = look.text_font;
  unsigned width = font.TextSize("-2000").width + 2 * padding;
  const unsigned min_control = Layout::GetMinimumControlHeight();
  if (width < min_control)
    width = min_control;

  return width;
}

[[gnu::pure]]
static unsigned
GetMaximumColumnWidth(const DialogLook &look) noexcept
{
  const unsigned padding = Layout::GetTextPadding();
  const Font &font = look.text_font;
  unsigned width = font.TextSize("300.00").width + 2 * padding;
  const unsigned max_control = Layout::GetMaximumControlHeight();
  if (width < max_control)
    width = max_control;

  return width;
}

static void
LoadValue(WndProperty &e, double value, UnitGroup unit_group) noexcept
{
  const Unit unit = Units::GetUserUnitByGroup(unit_group);

  DataFieldFloat &df = *(DataFieldFloat *)e.GetDataField();
  assert(df.GetType() == DataField::Type::REAL);
  df.SetUnits(Units::GetUnitName(unit));
  df.SetValue(Units::ToUserUnit(value, unit));

  e.RefreshDisplay();
}

static void
LoadPoint(PolarShapeEditWidget::PointEditor &pe,
          const PolarPoint &point) noexcept
{
  LoadValue(*pe.v, point.v, UnitGroup::HORIZONTAL_SPEED);
  LoadValue(*pe.w, point.w, UnitGroup::VERTICAL_SPEED);
}

static double
GetValue(WndProperty &e) noexcept
{
  return ((DataFieldFloat *)e.GetDataField())->GetValue();
}

static bool
SaveValue(WndProperty &e, double &value_r, UnitGroup unit_group) noexcept
{
  const Unit unit = Units::GetUserUnitByGroup(unit_group);

  auto new_value = Units::ToSysUnit(GetValue(e), unit);
  if (new_value == value_r)
    return false;

  value_r = new_value;
  return true;
}

static bool
SavePoint(const PolarShapeEditWidget::PointEditor &pe,
          PolarPoint &point) noexcept
{
  bool changed = false;
  changed |= SaveValue(*pe.v, point.v, UnitGroup::HORIZONTAL_SPEED);
  changed |= SaveValue(*pe.w, point.w, UnitGroup::VERTICAL_SPEED);
  return changed;
}

void
PolarShapeEditWidget::SetPolarShape(const PolarShape &_shape) noexcept
{
  shape = _shape;

  for (unsigned i = 0; i < ARRAY_SIZE(points); ++i)
    if (points[i].v != nullptr)
      LoadPoint(points[i], shape[i]);
}

PixelSize
PolarShapeEditWidget::GetMinimumSize() const noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  const unsigned width = GetLabelWidth(look) +
    ARRAY_SIZE(points) * GetMinimumColumnWidth(look);

  return { width, 2 * Layout::GetMinimumControlHeight() };
}

PixelSize
PolarShapeEditWidget::GetMaximumSize() const noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  const unsigned width = GetLabelWidth(look) +
    ARRAY_SIZE(points) * GetMaximumColumnWidth(look);

  return { width, 2 * Layout::GetMaximumControlHeight() };
}

void
PolarShapeEditWidget::CreateControls(ContainerWindow &panel,
                                     const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  const unsigned width = rc.GetWidth(), height = rc.GetHeight();
  const unsigned row_height = std::max(1U, height / 2);
  const unsigned label_width = std::min(GetLabelWidth(look), width);
  const unsigned columns_width = width > label_width ? width - label_width : 1U;
  const unsigned edit_width =
    std::max(1U, columns_width / unsigned(ARRAY_SIZE(points)));
  const unsigned edit_remainder = columns_width - edit_width * ARRAY_SIZE(points);

  const char *v_text = C_("Glider polar coefficient", "Polar V");
  const char *w_text = C_("Glider polar coefficient", "Polar W");

  WindowStyle style;
  style.TabStop();

  PixelRect label_rc(0, 0, label_width, row_height);
  v_label = std::make_unique<WndFrame>(panel, look, label_rc);
  v_label->SetText(v_text);

  PixelRect field_rc;
  field_rc.left = label_width;
  field_rc.top = 0;
  field_rc.right = field_rc.left + edit_width;
  field_rc.bottom = row_height;
  for (unsigned i = 0; i < ARRAY_SIZE(points); ++i) {
    if (i + 1 == ARRAY_SIZE(points))
      field_rc.right = width;

    points[i].v = std::make_unique<WndProperty>(panel, look, "",
                                                field_rc, 0, style);
    DataFieldFloat *df = new DataFieldFloat("%.0f", "%.0f %s",
                                            0, 300, 0, 1, false,
                                            listener);
    points[i].v->SetDataField(df);

    field_rc.left = field_rc.right;
    field_rc.right = field_rc.left + edit_width +
      (i + 1 == ARRAY_SIZE(points) ? edit_remainder : 0u);
  }

  label_rc.top += row_height;
  label_rc.bottom += row_height;
  w_label = std::make_unique<WndFrame>(panel, look, label_rc);
  w_label->SetText(w_text);

  field_rc.left = label_width;
  field_rc.top = row_height;
  field_rc.right = field_rc.left + edit_width;
  field_rc.bottom = height;

  double step = 0.01, min = -10;
  switch (Units::current.vertical_speed_unit) {
  case Unit::FEET_PER_MINUTE:
    step = 2;
    min = -2000;
    break;

  case Unit::KNOTS:
    step = 0.02;
    min = -20;
    break;

  default:
    break;
  }

  for (unsigned i = 0; i < ARRAY_SIZE(points); ++i) {
    if (i + 1 == ARRAY_SIZE(points))
      field_rc.right = width;

    points[i].w = std::make_unique<WndProperty>(panel, look, "",
                                                field_rc, 0, style);
    DataFieldFloat *df = new DataFieldFloat("%.2f", "%.2f %s",
                                            min, 0, 0, step, false,
                                            listener);

    points[i].w->SetDataField(df);

    field_rc.left = field_rc.right;
    field_rc.right = field_rc.left + edit_width +
      (i + 1 == ARRAY_SIZE(points) ? edit_remainder : 0u);
  }

  for (unsigned i = 0; i < ARRAY_SIZE(points); ++i)
    LoadPoint(points[i], shape[i]);
}

void
PolarShapeEditWidget::UpdateGeometry(const PixelRect &rc) noexcept
{
  assert(IsDefined());

  const DialogLook &look = UIGlobals::GetDialogLook();

  const unsigned width = rc.GetWidth(), height = rc.GetHeight();
  const unsigned row_height = std::max(1U, height / 2);
  const unsigned label_width = std::min(GetLabelWidth(look), width);
  const unsigned columns_width = width > label_width ? width - label_width : 1U;
  const unsigned edit_width =
    std::max(1U, columns_width / unsigned(ARRAY_SIZE(points)));
  const unsigned edit_remainder = columns_width - edit_width * ARRAY_SIZE(points);

  PixelRect label_rc(0, 0, label_width, row_height);
  v_label->Move(label_rc);

  PixelRect field_rc;
  field_rc.left = label_width;
  field_rc.top = 0;
  field_rc.right = field_rc.left + edit_width;
  field_rc.bottom = row_height;
  for (unsigned i = 0; i < ARRAY_SIZE(points); ++i) {
    if (i + 1 == ARRAY_SIZE(points))
      field_rc.right = width;

    points[i].v->Move(field_rc);

    field_rc.left = field_rc.right;
    field_rc.right = field_rc.left + edit_width +
      (i + 1 == ARRAY_SIZE(points) ? edit_remainder : 0u);
  }

  label_rc.top += row_height;
  label_rc.bottom += row_height;
  w_label->Move(label_rc);

  field_rc.left = label_width;
  field_rc.top = row_height;
  field_rc.right = field_rc.left + edit_width;
  field_rc.bottom = height;

  for (unsigned i = 0; i < ARRAY_SIZE(points); ++i) {
    if (i + 1 == ARRAY_SIZE(points))
      field_rc.right = width;

    points[i].w->Move(field_rc);

    field_rc.left = field_rc.right;
    field_rc.right = field_rc.left + edit_width +
      (i + 1 == ARRAY_SIZE(points) ? edit_remainder : 0u);
  }
}

void
PolarShapeEditWidget::Prepare(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  PanelWidget::Prepare(parent, rc);
  ContainerWindow &panel = (ContainerWindow &)GetWindow();
  CreateControls(panel, rc);
}

void
PolarShapeEditWidget::Move(const PixelRect &rc) noexcept
{
  WindowWidget::Move(rc);

  if (v_label != nullptr)
    UpdateGeometry(rc);
}

bool
PolarShapeEditWidget::Save(bool &_changed) noexcept
{
  bool changed = false;

  for (unsigned i = 0; i < ARRAY_SIZE(points); ++i)
    changed |= SavePoint(points[i], shape[i]);

  _changed |= changed;
  return true;
}
