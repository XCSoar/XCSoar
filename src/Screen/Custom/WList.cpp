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

#include "WList.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Screen/SubCanvas.hpp"

#include <algorithm>

void
WindowList::Clear()
{
  /* destroy all child windows */
  std::list<Window *>::const_iterator i;
  while ((i = list.begin()) != list.end()) {
    Window &w = **i;
    w.Destroy();

    assert(!Contains(w));
  }

  assert(list.empty());
}

bool
WindowList::Contains(const Window &w) const
{
  return std::find(list.begin(), list.end(), &w) != list.end();
}

bool
WindowList::IsCovered(const Window &w) const
{
  const PixelRect rc = w.GetPosition();

  /* find the last full window which covers all the other windows
     behind it */
  for (auto i = list.begin();; ++i) {
    assert(i != list.end());

    Window &child = **i;
    if (&child == &w)
      /* didn't find a covering sibling so far */
      return false;

    if (child.IsVisible() &&
        child.GetLeft() <= rc.left &&
        child.GetRight() >= rc.right &&
        child.GetTop() <= rc.top &&
        child.GetBottom() >= rc.bottom)
      /* this sibling covers the specified window completely */
      return true;
  }
}

void
WindowList::BringToTop(Window &w)
{
  assert(Contains(w));

  list.remove(&w);
  list.insert(list.begin(), &w);
}

void
WindowList::BringToBottom(Window &w)
{
  assert(Contains(w));

  list.remove(&w);
  list.push_back(&w);
}

gcc_pure
static bool
IsAt(Window &w, PixelScalar x, PixelScalar y)
{
  return w.IsVisible() &&
    x >= w.GetLeft() && x < w.GetRight() &&
    y >= w.GetTop() && y < w.GetBottom();
}

Window *
WindowList::FindAt(PixelScalar x, PixelScalar y)
{
  for (Window *w : list)
    if (w->IsEnabled() && IsAt(*w, x, y))
      return w;

  return NULL;
}

gcc_pure
Window *
WindowList::FindControl(std::list<Window*>::const_iterator i,
                        std::list<Window*>::const_iterator end)
{
  for (; i != end; ++i) {
    Window &child = **i;
    if (!child.IsVisible() || !child.IsEnabled())
      continue;

    if (child.IsTabStop())
      return &child;

    if (child.IsControlParent()) {
      ContainerWindow &container = (ContainerWindow &)child;
      Window *control = container.children.FindFirstControl();
      if (control != NULL)
        return control;
    }
  }

  return NULL;
}

gcc_pure
Window *
WindowList::FindControl(std::list<Window*>::const_reverse_iterator i,
                        std::list<Window*>::const_reverse_iterator end)
{
  for (; i != end; ++i) {
    Window &child = **i;
    if (!child.IsVisible() || !child.IsEnabled())
      continue;

    if (child.IsTabStop())
      return &child;

    if (child.IsControlParent()) {
      ContainerWindow &container = (ContainerWindow &)child;
      Window *control = container.children.FindLastControl();
      if (control != NULL)
        return control;
    }
  }

  return NULL;
}

Window *
WindowList::FindFirstControl()
{
  return FindControl(list.begin(), list.end());
}

Window *
WindowList::FindLastControl()
{
  return FindControl(list.rbegin(), list.rend());
}

Window *
WindowList::FindNextChildControl(Window *reference)
{
  assert(reference != NULL);
  assert(Contains(*reference));

  std::list<Window*>::const_iterator i =
    std::find(list.begin(), list.end(), reference);
  assert(i != list.end());

  return FindControl(++i, list.end());
}

Window *
WindowList::FindPreviousChildControl(Window *reference)
{
  assert(reference != NULL);
  assert(Contains(*reference));

  std::list<Window*>::const_reverse_iterator i =
    std::find(list.rbegin(), list.rend(), reference);
#ifndef ANDROID
  /* Android's NDK r5b ships a cxx-stl which does not allow comparing
     two const_reverse_iterator objects for inequality */
  assert(i != list.rend());
#endif

  return FindControl(++i, list.rend());
}

gcc_pure
static bool
IsFullWindow(const Window &w, int width, int height)
{
  return w.IsVisible() &&
    w.GetLeft() <= 0 && w.GetRight() >= (int)width &&
    w.GetTop() <= 0 && w.GetBottom() >= (int)height;
}

void
WindowList::Paint(Canvas &canvas)
{
  const auto &list = this->list;

  auto begin = list.rbegin(), end = list.rend();

  /* find the last full window which covers all the other windows
     behind it */
  for (auto i = begin; i != end; ++i) {
    Window &child = **i;
    if (IsFullWindow(child, canvas.GetWidth(), canvas.GetHeight()))
      begin = i;
  }

  for (auto i = begin; i != end; ++i) {
    Window &child = **i;
    if (!child.IsVisible())
      continue;

    SubCanvas sub_canvas(canvas, { child.GetLeft(), child.GetTop() },
                         child.GetSize());
#ifdef USE_MEMORY_CANVAS
    if (sub_canvas.GetWidth() == 0 || sub_canvas.GetHeight() == 0)
      /* this child window is completely outside the physical
         screen */
      continue;
#endif

    child.Setup(sub_canvas);
    child.OnPaint(sub_canvas);
  }
}
