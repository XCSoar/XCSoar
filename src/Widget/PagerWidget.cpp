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

#include "PagerWidget.hpp"

#include <assert.h>

PagerWidget::~PagerWidget()
{
  assert(!initialised || !prepared);

  Clear();
}

void
PagerWidget::Add(Widget *w)
{
  const bool was_empty = children.empty();
  if (was_empty) {
    current = 0;
  } else {
    assert(current < children.size());
  }

  children.append(w);

  if (initialised) {
    w->Initialise(*parent, position);

    if (prepared) {
      children.back().prepared = true;
      w->Prepare(*parent, position);

      if (visible && was_empty)
        w->Show(position);
    }
  }
}

void
PagerWidget::Clear()
{
  assert(!initialised || !prepared);

  for (auto &i : children) {
    assert(!i.prepared);

    delete i.widget;
  }

  children.clear();
}

void
PagerWidget::PrepareWidget(unsigned i)
{
  assert(initialised);
  assert(prepared);

  Child &child = children[i];
  if (!child.prepared) {
    child.prepared = true;
    child.widget->Prepare(*parent, position);
  }
}

bool
PagerWidget::SetCurrent(unsigned i, bool click)
{
  assert(i < children.size());

  if (!initialised || !prepared) {
    /* quick code path: not yet prepared, quickly switch without
       checks */
    assert(!click);
    current = i;
    return true;
  }

  assert(current < children.size());
  assert(!visible || children[current].prepared);
  assert(!click || visible);

  Child &old_child = children[current];
  Child &new_child = children[i];

  if (i == current) {
    if (click) {
      assert(visible);
      assert(new_child.prepared);
      new_child.widget->ReClick();
      return true;
    } else {
      return true;
    }
  }

  assert(!visible || old_child.prepared);
  if (visible && !old_child.widget->Leave())
    return false;

  if (click && !new_child.widget->Click())
    return false;

  if (visible)
    old_child.widget->Hide();

  current = i;

  if (!new_child.prepared) {
    new_child.prepared = true;
    new_child.widget->Prepare(*parent, position);
  }

  if (visible)
    new_child.widget->Show(position);

  OnPageFlipped();
  return true;
}

bool
PagerWidget::Next(bool wrap)
{
  if (children.size() < 2)
    return false;

  assert(current < children.size());

  unsigned i = current + 1;
  if (i >= children.size()) {
    if (!wrap)
      return false;

    i = 0;
  }

  return SetCurrent(i);
}

bool
PagerWidget::Previous(bool wrap)
{
  if (children.size() < 2)
    return false;

  assert(current < children.size());

  unsigned i = current;
  if (i == 0) {
    if (!wrap)
      return false;

    i = children.size();
  }

  return SetCurrent(i - 1);
}

PixelSize
PagerWidget::GetMinimumSize() const
{
  /* determine the largest "minimum" size of all pages */

  PixelSize result{0, 0};

  for (const auto &i : children) {
    PixelSize size = i.widget->GetMinimumSize();
    if (size.cx > result.cx)
      result.cx = size.cx;
    if (size.cy > result.cy)
      result.cy = size.cy;
  }

  return result;
}

PixelSize
PagerWidget::GetMaximumSize() const
{
  /* determine the largest "maximum" size of all pages */

  PixelSize result{0, 0};

  for (const auto &i : children) {
    PixelSize size = i.widget->GetMaximumSize();
    if (size.cx > result.cx)
      result.cx = size.cx;
    if (size.cy > result.cy)
      result.cy = size.cy;
  }

  return result;
}

void
PagerWidget::Initialise(ContainerWindow &_parent, const PixelRect &rc)
{
  assert(!initialised);

  initialised = true;
  prepared = false;
  parent = &_parent;
  position = rc;

  for (auto &i : children)
    i.widget->Initialise(*parent, position);
}

void
PagerWidget::Prepare(ContainerWindow &_parent, const PixelRect &rc)
{
  assert(initialised);
  assert(!prepared);

  prepared = true;
  visible = false;
  parent = &_parent;
  position = rc;

  for (auto &i : children) {
    assert(!i.prepared);
    i.prepared = true;
    i.widget->Prepare(*parent, position);
  }
}

void
PagerWidget::Unprepare()
{
  assert(initialised);
  assert(prepared);
  assert(!visible);

  prepared = false;

  for (auto &i : children) {
    if (i.prepared) {
      i.prepared = false;
      i.widget->Unprepare();
    }
  }
}

bool
PagerWidget::Save(bool &changed)
{
  assert(initialised);
  assert(prepared);

  for (auto &i : children)
    if (i.prepared && !i.widget->Save(changed))
      return false;

  return true;
}

bool
PagerWidget::Click()
{
  return children.empty() || children[current].widget->Click();
}

void
PagerWidget::ReClick()
{
  assert(initialised);
  assert(prepared);
  assert(visible);
  assert(!children.empty());
  assert(children[current].prepared);

  children[current].widget->ReClick();
}

void
PagerWidget::Show(const PixelRect &rc)
{
  assert(initialised);
  assert(prepared);
  assert(!visible);

  visible = true;
  position = rc;

  if (children.empty())
    /* we cannot show anything yet; this is an allowed transitional
       state, and the caller is required to use Add() quickly, to
       actually show something */
    return;

  Child &child = children[current];
  if (!child.prepared) {
    child.prepared = true;
    child.widget->Prepare(*parent, position);
  }

  child.widget->Show(position);
}

void
PagerWidget::Hide()
{
  assert(initialised);
  assert(prepared);
  assert(visible);
  assert(!children.empty());
  assert(children[current].prepared);

  visible = false;
  return children[current].widget->Hide();
}

bool
PagerWidget::Leave()
{
  assert(initialised);
  assert(prepared);
  assert(visible);
  assert(!children.empty());
  assert(children[current].prepared);

  return children[current].widget->Leave();
}

void
PagerWidget::Move(const PixelRect &rc)
{
  assert(initialised);
  assert(prepared);
  assert(visible);

  position = rc;

  if (children.empty())
    /* allowed transitional state, see Show() for explanation */
    return;

  assert(children[current].prepared);

  children[current].widget->Move(rc);
}

bool
PagerWidget::SetFocus()
{
  assert(initialised);
  assert(prepared);
  assert(visible);
  assert(!children.empty());
  assert(children[current].prepared);

  return children[current].widget->SetFocus();
}

bool
PagerWidget::KeyPress(unsigned key_code)
{
  assert(initialised);
  assert(prepared);
  assert(visible);
  assert(!children.empty());
  assert(children[current].prepared);

  return children[current].widget->KeyPress(key_code);
}

void
PagerWidget::OnPageFlipped()
{
  if (page_flipped_callback)
    page_flipped_callback();
}
