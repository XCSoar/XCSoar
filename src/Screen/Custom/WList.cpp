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

#include "WList.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Screen/SubCanvas.hpp"

#include <iterator>

void
WindowList::Clear()
{
  /* destroy all child windows */
  List::iterator i;
  while ((i = list.begin()) != list.end()) {
    Window &w = *i;
    w.Destroy();

    assert(!Contains(w));
  }

  assert(list.empty());
}

bool
WindowList::Contains(const Window &w) const
{
  for (const auto &i : list)
    if (&i == &w)
      return true;

  return false;
}

bool
WindowList::IsCovered(const Window &w) const
{
  const PixelRect rc = w.GetPosition();

  /* find the last full window which covers all the other windows
     behind it */
  for (auto i = list.begin();; ++i) {
    assert(i != list.end());

    const Window &child = *i;
    if (&child == &w)
      /* didn't find a covering sibling so far */
      return false;

    if (child.IsVisible() && !child.IsTransparent() &&
        child.GetPosition().Contains(rc))
      /* this sibling covers the specified window completely */
      return true;
  }
}

void
WindowList::BringToTop(Window &w)
{
  assert(Contains(w));

  list.erase(list.iterator_to(w));
  list.push_front(w);
}

void
WindowList::BringToBottom(Window &w)
{
  assert(Contains(w));

  list.erase(list.iterator_to(w));
  list.push_back(w);
}

gcc_pure
static bool
IsAt(Window &w, PixelPoint p)
{
  return w.IsVisible() && w.GetPosition().Contains(p);
}

Window *
WindowList::FindAt(PixelPoint p)
{
  for (Window &w : list)
    if (w.IsEnabled() && IsAt(w, p))
      return &w;

  return nullptr;
}

gcc_pure
Window *
WindowList::FindControl(List::iterator i, WindowList::List::iterator end)
{
  for (; i != end; ++i) {
    Window &child = *i;
    if (!child.IsVisible() || !child.IsEnabled())
      continue;

    if (child.IsTabStop())
      return &child;

    if (child.IsControlParent()) {
      ContainerWindow &container = (ContainerWindow &)child;
      Window *control = container.children.FindFirstControl();
      if (control != nullptr)
        return control;
    }
  }

  return nullptr;
}

gcc_pure
Window *
WindowList::FindControl(WindowList::List::reverse_iterator i,
                        WindowList::List::reverse_iterator end)
{
  for (; i != end; ++i) {
    Window &child = *i;
    if (!child.IsVisible() || !child.IsEnabled())
      continue;

    if (child.IsTabStop())
      return &child;

    if (child.IsControlParent()) {
      ContainerWindow &container = (ContainerWindow &)child;
      Window *control = container.children.FindLastControl();
      if (control != nullptr)
        return control;
    }
  }

  return nullptr;
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
  assert(reference != nullptr);
  assert(Contains(*reference));

  auto i = list.iterator_to(*reference);
  assert(i != list.end());

  return FindControl(++i, list.end());
}

Window *
WindowList::FindPreviousChildControl(Window *reference)
{
  assert(reference != nullptr);
  assert(Contains(*reference));

  /* the std::next() is necessary because reverse iterators
     dereference to std::previous() - sounds like an awkward design
     decision, but that's how it is */
  List::reverse_iterator i(std::next(list.iterator_to(*reference)));
  assert(i != list.rend());

  return FindControl(++i, list.rend());
}

gcc_pure
static bool
IsFullWindow(const Window &w, int width, int height)
{
  return w.IsVisible() &&
    w.GetPosition().Contains(PixelRect(0, 0, width, height));
}

void
WindowList::Paint(Canvas &canvas)
{
  auto begin = list.rbegin(), end = list.rend();

  /* find the last full window which covers all the other windows
     behind it */
  for (auto i = begin; i != end; ++i) {
    Window &child = *i;
    if (IsFullWindow(child, canvas.GetWidth(), canvas.GetHeight()) &&
        !child.IsTransparent())
      begin = i;
  }

  for (auto i = begin; i != end; ++i) {
    PaintWindow &child = (PaintWindow &)*i;
    if (!child.IsVisible())
      continue;

    SubCanvas sub_canvas(canvas, child.GetTopLeft(),
                         child.GetSize());
#ifdef USE_MEMORY_CANVAS
    if (sub_canvas.GetWidth() == 0 || sub_canvas.GetHeight() == 0)
      /* this child window is completely outside the physical
         screen */
      continue;
#endif

    child.OnPaint(sub_canvas);
  }
}
