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

#include "ArrowPagerWidget.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Language/Language.hpp"
#include "Form/Form.hpp"

#include <assert.h>

ArrowPagerWidget::Layout::Layout(PixelRect rc)
  :main(rc)
{
  const unsigned width = rc.right - rc.left;
  const unsigned height = rc.bottom - rc.top;

  main = rc;

  if (width > height) {
    /* landscape */

    main.left += ::Layout::Scale(70);

    /* close button on the bottom left */

    close_button.left = rc.left;
    close_button.right = main.left;
    close_button.bottom = rc.bottom;
    close_button.top = close_button.bottom - ::Layout::GetMaximumControlHeight();

    /* previous/next buttons above the close button */

    previous_button = close_button;
    previous_button.bottom = previous_button.top;
    previous_button.top = previous_button.bottom - ::Layout::GetMaximumControlHeight();
    previous_button.right = (previous_button.left + previous_button.right) / 2;

    next_button = previous_button;
    next_button.left = next_button.right;
    next_button.right = close_button.right;
  } else {
    /* portrait */

    main.bottom -= ::Layout::GetMaximumControlHeight();

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
  }
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
ArrowPagerWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const Layout layout(rc);
  PagerWidget::Prepare(parent, layout.main);

  ButtonWindowStyle style;
  style.Hide();
  style.TabStop();

  previous_button.Create(parent, _T("<"), layout.previous_button,
                         style, *this, PREVIOUS);
  next_button.Create(parent, _T(">"), layout.next_button, style, *this, NEXT);
  close_button.Create(parent, _("Close"), layout.close_button,
                      style, action_listener, mrOK);
}

void
ArrowPagerWidget::Show(const PixelRect &rc)
{
  const Layout layout(rc);
  PagerWidget::Show(layout.main);

  previous_button.MoveAndShow(layout.previous_button);
  next_button.MoveAndShow(layout.next_button);
  close_button.MoveAndShow(layout.close_button);
}

void
ArrowPagerWidget::Hide()
{
  PagerWidget::Hide();

  previous_button.Hide();
  next_button.Hide();
  close_button.Hide();
}

void
ArrowPagerWidget::Move(const PixelRect &rc)
{
  const Layout layout(rc);
  PagerWidget::Move(layout.main);

  previous_button.Move(layout.previous_button);
  next_button.Move(layout.next_button);
  close_button.Move(layout.close_button);
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

  switch (key_code) {
  case KEY_LEFT:
#ifdef GNAV
  case '6':
#endif
    if (Previous(true))
      OnPageFlipped();
    return true;

  case KEY_RIGHT:
#ifdef GNAV
  case '7':
#endif
    if (Next(true))
      OnPageFlipped();
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
    if (Previous(false))
      OnPageFlipped();
    break;

  case NEXT:
    if (Next(false))
      OnPageFlipped();
    break;
  }
}
