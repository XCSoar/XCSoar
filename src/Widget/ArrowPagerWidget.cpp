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

#include "ArrowPagerWidget.hpp"
#include "Screen/Layout.hpp"
#include "Event/KeyCode.hpp"
#include "Language/Language.hpp"
#include "Form/Form.hpp"
#include "Renderer/SymbolButtonRenderer.hpp"

ArrowPagerWidget::Layout::Layout(PixelRect rc, const Widget *extra_widget)
  :main(rc)
{
  const unsigned width = rc.GetWidth(), height = rc.GetHeight();
  const unsigned button_height = ::Layout::GetMaximumControlHeight();

  main = rc;

  if (width > height) {
    /* landscape */

    main.left += ::Layout::Scale(70);

    /* close button on the bottom left */

    close_button.left = rc.left;
    close_button.right = main.left;
    close_button.bottom = rc.bottom;
    close_button.top = close_button.bottom - button_height;

    /* previous/next buttons above the close button */

    previous_button = close_button;
    previous_button.bottom = previous_button.top;
    previous_button.top = previous_button.bottom - button_height;
    previous_button.right = (previous_button.left + previous_button.right) / 2;

    next_button = previous_button;
    next_button.left = next_button.right;
    next_button.right = close_button.right;

    /* the remaining area is "extra" */

    extra.left = close_button.left;
    extra.right = close_button.right;
    extra.top = rc.top;
    extra.bottom = previous_button.top;
  } else {
    /* portrait */

    main.bottom -= button_height;

    /* buttons distributed on the bottom line */

    previous_button.top = next_button.top =
      close_button.top = main.bottom;
    previous_button.bottom = next_button.bottom =
      close_button.bottom = rc.bottom;

    previous_button.left = rc.left;
    close_button.right = rc.right;
    close_button.left = (rc.left + rc.right) / 2;

    next_button.right = close_button.left;
    previous_button.right = next_button.left =
      (rc.left * 3 + rc.right) / 4;
    previous_button.left = rc.left;

    /* "extra" gets another row */

    if (extra_widget != nullptr) {
      extra.left = main.left;
      extra.right = main.right;
      extra.bottom = main.bottom;
      extra.top = main.bottom -= button_height;
    }
  }
}

ArrowPagerWidget::~ArrowPagerWidget()
{
  delete extra;
}

PixelSize
ArrowPagerWidget::GetMinimumSize() const
{
  PixelSize result = PagerWidget::GetMinimumSize();
  result.cx += ::Layout::Scale(50);
  result.cy += 2 * ::Layout::GetMinimumControlHeight();
  return result;
}

PixelSize
ArrowPagerWidget::GetMaximumSize() const
{
  PixelSize result = PagerWidget::GetMinimumSize();
  result.cx += ::Layout::Scale(80);
  result.cy += 2 * ::Layout::GetMaximumControlHeight();
  return result;
}

void
ArrowPagerWidget::Initialise(ContainerWindow &parent,
                             const PixelRect &rc)
{
  PagerWidget::Initialise(parent, rc);

  if (extra != nullptr)
    extra->Initialise(parent, rc);
}

void
ArrowPagerWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const Layout layout(rc, extra);
  PagerWidget::Prepare(parent, layout.main);

  if (extra != nullptr)
    extra->Prepare(parent, layout.extra);

  WindowStyle style;
  style.Hide();
  style.TabStop();

  previous_button.Create(parent, layout.previous_button, style,
                         new SymbolButtonRenderer(look, _T("<")),
                         *this, PREVIOUS);
  next_button.Create(parent, layout.next_button, style,
                     new SymbolButtonRenderer(look, _T(">")),
                     *this, NEXT);
  close_button.Create(parent, look, _("Close"), layout.close_button,
                      style, action_listener, mrOK);
}

void
ArrowPagerWidget::Show(const PixelRect &rc)
{
  const Layout layout(rc, extra);
  PagerWidget::Show(layout.main);

  previous_button.MoveAndShow(layout.previous_button);
  next_button.MoveAndShow(layout.next_button);
  close_button.MoveAndShow(layout.close_button);

  if (extra != nullptr)
    extra->Show(layout.extra);
}

void
ArrowPagerWidget::Hide()
{
  PagerWidget::Hide();

  previous_button.Hide();
  next_button.Hide();
  close_button.Hide();

  if (extra != nullptr)
    extra->Hide();
}

void
ArrowPagerWidget::Move(const PixelRect &rc)
{
  const Layout layout(rc, extra);
  PagerWidget::Move(layout.main);

  previous_button.Move(layout.previous_button);
  next_button.Move(layout.next_button);
  close_button.Move(layout.close_button);

  if (extra != nullptr)
    extra->Move(layout.extra);
}

bool
ArrowPagerWidget::SetFocus()
{
  if (!PagerWidget::SetFocus())
    close_button.SetFocus();

  return true;
}

bool
ArrowPagerWidget::KeyPress(unsigned key_code)
{
  if (PagerWidget::KeyPress(key_code))
    return true;

  if (extra != nullptr && extra->KeyPress(key_code))
    return true;

  switch (key_code) {
  case KEY_LEFT:
    Previous(true);
    return true;

  case KEY_RIGHT:
    Next(true);
    return true;

  default:
    return false;
  }
}

void
ArrowPagerWidget::OnAction(int id)
{
  switch (id) {
  case PREVIOUS:
    Previous(false);
    break;

  case NEXT:
    Next(false);
    break;
  }
}
