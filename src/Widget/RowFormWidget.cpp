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
#include "Form/Panel.hpp"
#include "Form/Button.hpp"
#include "Form/HLine.hpp"
#include "Look/DialogLook.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "UIGlobals.hpp"
#include "Screen/Layout.hpp"
#include "Screen/LargeTextWindow.hpp"
#include "Screen/Font.hpp"

#include <windef.h> /* for MAX_PATH */
#include <assert.h>

gcc_pure
static unsigned
GetMinimumHeight(const WndProperty &control, const DialogLook &look,
                 bool vertical)
{
  if (vertical && control.HasCaption())
    return look.text_font->GetHeight() + Layout::GetMinimumControlHeight();

  return Layout::GetMinimumControlHeight();
}

gcc_pure
static unsigned
GetMaximumHeight(const WndProperty &control, const DialogLook &look,
                 bool vertical)
{
  if (vertical && control.HasCaption()) {
    const unsigned min_height = look.text_font->GetHeight()
      + Layout::GetMinimumControlHeight();
    return std::max(min_height,
                    Layout::GetMaximumControlHeight());
  }

  return control.IsReadOnly()
    /* rows that are not clickable don't need to be extra-large */
    ? Layout::GetMinimumControlHeight()
    : Layout::GetMaximumControlHeight();
}

unsigned
RowFormWidget::Row::GetMinimumHeight(const DialogLook &look,
                                     bool vertical) const
{
  switch (type) {
  case Type::DUMMY:
    return 0;

  case Type::WIDGET:
    return widget->GetMinimumSize().cy;

  case Type::GENERIC:
    break;

  case Type::EDIT:
    return ::GetMinimumHeight(GetControl(), look, vertical);

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
RowFormWidget::Row::GetMaximumHeight(const DialogLook &look,
                                     bool vertical) const
{
  switch (type) {
  case Type::DUMMY:
    return 0;

  case Type::WIDGET:
    return widget->GetMaximumSize().cy;

  case Type::GENERIC:
    break;

  case Type::EDIT:
    return ::GetMaximumHeight(GetControl(), look, vertical);

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
  if (IsDefined())
    DeleteWindow();

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
  assert(window->GetParent() == &GetWindow());
#endif
  assert(window->IsVisible());
  /* cannot append rows after a REMAINING row */
  assert(rows.empty() || rows.back().type != Row::Type::REMAINING);

  rows.push_back(Row(type, window));
}

void
RowFormWidget::AddSpacer()
{
  assert(IsDefined());

  HLine *window = new HLine(GetLook());
  ContainerWindow &panel = (ContainerWindow &)GetWindow();
  const PixelRect rc = InitialControlRect(Layout::Scale(3));
  window->Create(panel, rc);
  Add(window);
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

  ContainerWindow &panel = (ContainerWindow &)GetWindow();
  LargeTextWindow *ltw = new LargeTextWindow();
  ltw->Create(panel, rc, style);
  ltw->SetFont(*look.text_font);

  if (text != nullptr)
    ltw->SetText(text);

  Add(Row::Type::MULTI_LINE, ltw);
}

WndButton *
RowFormWidget::AddButton(const TCHAR *label, ActionListener &listener, int id)
{
  assert(IsDefined());

  const PixelRect button_rc =
    InitialControlRect(Layout::GetMinimumControlHeight());

  ButtonWindowStyle button_style;
  button_style.TabStop();
  button_style.multiline();

  ContainerWindow &panel = (ContainerWindow &)GetWindow();

  WndButton *button = new WndButton(panel, look.button, label, button_rc,
                                    button_style, listener, id);

  Add(Row::Type::BUTTON, button);
  return button;
}

void
RowFormWidget::SetMultiLineText(unsigned i, const TCHAR *text)
{
  assert(text != nullptr);
  assert(rows[i].type == Row::Type::MULTI_LINE);

  LargeTextWindow &ltw = *(LargeTextWindow *)rows[i].window;
  ltw.SetText(text);
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
  PixelRect current_rect = GetWindow().GetClientRect();
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

    min_height += i.GetMinimumHeight(look, vertical);
    if (i.IsElastic(look, vertical))
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
    UPixelScalar height = i.GetMinimumHeight(look, vertical);
    if (excess_height > 0 && i.IsElastic(look, vertical)) {
      assert(n_elastic > 0);

      /* distribute excess height among all elastic rows */
      unsigned grow_height = excess_height / n_elastic;
      if (grow_height > 0) {
        height += grow_height;
        const UPixelScalar max_height = i.GetMaximumHeight(look, vertical);
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
        widget.Initialise((ContainerWindow &)GetWindow(), current_rect);
      }

      if (!i.prepared) {
        i.prepared = true;
        widget.Prepare((ContainerWindow &)GetWindow(), current_rect);
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
      size.cy += i.GetMinimumHeight(look, vertical);

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
    size.cy += i.GetMaximumHeight(look, vertical);

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
  Window &panel = GetWindow();
  panel.Move(rc);

  UpdateLayout();

  panel.Show();
}

void
RowFormWidget::Move(const PixelRect &rc)
{
  WindowWidget::Move(rc);

  UpdateLayout();
}

bool
RowFormWidget::SetFocus()
{
  if (rows.empty())
    return false;

  ContainerWindow &panel = (ContainerWindow &)GetWindow();
  return panel.FocusFirstControl();
}
