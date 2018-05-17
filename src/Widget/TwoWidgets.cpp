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

#include "TwoWidgets.hpp"

#include <algorithm>

TwoWidgets::~TwoWidgets()
{
  delete second;
  delete first;
}

void
TwoWidgets::UpdateLayout()
{
  const auto layout = CalculateLayout(rc);
  first->Move(layout.first);
  second->Move(layout.second);
}

gcc_const
static int
CalculateSplit(int top, int bottom, unsigned min_a,
               unsigned min_b, unsigned max_b)
{
  assert(bottom >= top);
  assert(min_b <= max_b);

  const unsigned height = bottom - top;

  if (min_a <= 0 || min_b <= 0)
    /* at least one Widet doesn't know its minimums size; there may be
       better solutions for this, but this workaround is good enough
       for fixing the assertion failure in DevicesConfigPanel */
    return (top + bottom) / 2;
  else if (height >= min_a + max_b)
    /* more than enough space: align the second Widget at the bottom
       and give the rest to the first Widget */
    return bottom - max_b;
  else if (height >= min_a + min_b)
    /* still somewhat enough space */
    return bottom - min_b;
  else {
    /* give the best for the rest */
    const unsigned first_height = std::min(min_a, height / 2);
    return top + first_height;
  }
}

int
TwoWidgets::CalculateSplit(const PixelRect &rc) const
{
  const PixelSize min_a = first->GetMinimumSize();
  const PixelSize min_b = second->GetMinimumSize();
  const PixelSize max_b = second->GetMaximumSize();

  return vertical
    ? ::CalculateSplit(rc.top, rc.bottom, min_a.cy,
                       min_b.cy, max_b.cy)
    : ::CalculateSplit(rc.left, rc.right, min_a.cx,
                       min_b.cx, max_b.cx);
}

std::pair<PixelRect,PixelRect>
TwoWidgets::CalculateLayout(const PixelRect &rc) const
{
  PixelRect a = rc, b = rc;
  if (vertical)
    a.bottom = b.top = CalculateSplit(rc);
  else
    a.right = b.left = CalculateSplit(rc);
  return std::make_pair(a, b);
}

PixelSize
TwoWidgets::GetMinimumSize() const
{
  const PixelSize a = first->GetMinimumSize();
  const PixelSize b = second->GetMinimumSize();

  return vertical
    ? PixelSize{ std::max(a.cx, b.cx), a.cy + b.cy }
    : PixelSize{ a.cx + b.cx, std::max(a.cy, b.cy) };
}

PixelSize
TwoWidgets::GetMaximumSize() const
{
  const PixelSize a = first->GetMaximumSize();
  const PixelSize b = second->GetMaximumSize();

  return vertical
    ? PixelSize{ std::max(a.cx, b.cx), a.cy + b.cy }
    : PixelSize{ a.cx + b.cx, std::max(a.cy, b.cy) };
}

/**
 * Calculates a "dummy" layout that is splitted in the middle.  In
 * TwoWidgets::Initialise() and TwoWidgets::Prepare(), we are not
 * allowed to call Widget::GetMinimumSize() yet.
 */
gcc_const
static std::pair<PixelRect,PixelRect>
DummyLayout(const PixelRect rc, bool vertical)
{
  PixelRect a = rc, b = rc;
  if (vertical)
    a.bottom = b.top = (rc.top + rc.bottom) / 2;
  else
    a.right = b.left = (rc.left + rc.right) / 2;
  return std::make_pair(a, b);
}

void
TwoWidgets::Initialise(ContainerWindow &parent, const PixelRect &rc)
{
  this->rc = rc;
  const auto layout = DummyLayout(rc, vertical);
  first->Initialise(parent, layout.first);
  second->Initialise(parent, layout.second);
}

void
TwoWidgets::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  this->rc = rc;
  const auto layout = DummyLayout(rc, vertical);
  first->Prepare(parent, layout.first);
  second->Prepare(parent, layout.second);
}

void
TwoWidgets::Unprepare()
{
  first->Unprepare();
  second->Unprepare();
}

bool
TwoWidgets::Save(bool &changed)
{
  return first->Save(changed) && second->Save(changed);
}

bool
TwoWidgets::Click()
{
  return first->Click() || second->Click();
}

void
TwoWidgets::ReClick()
{
  first->ReClick();
  second->ReClick();
}

void
TwoWidgets::Show(const PixelRect &rc)
{
  this->rc = rc;
  const auto layout = CalculateLayout(rc);
  first->Show(layout.first);
  second->Show(layout.second);
}

bool
TwoWidgets::Leave()
{
  return first->Leave() && second->Leave();
}

void
TwoWidgets::Hide()
{
  first->Hide();
  second->Hide();
}

void
TwoWidgets::Move(const PixelRect &rc)
{
  this->rc = rc;
  const auto layout = CalculateLayout(rc);
  first->Move(layout.first);
  second->Move(layout.second);
}

bool
TwoWidgets::SetFocus()
{
  return first->SetFocus() || second->SetFocus();
}

bool
TwoWidgets::KeyPress(unsigned key_code)
{
  return first->KeyPress(key_code) || second->KeyPress(key_code);
}
