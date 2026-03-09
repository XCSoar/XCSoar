// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WList.hpp"
#include "../ContainerWindow.hpp"
#include "ui/canvas/SubCanvas.hpp"
#ifdef USE_MEMORY_CANVAS
#include "ui/canvas/memory/ActivePixelTraits.hpp"
#include "ui/canvas/memory/Buffer.hpp"
#endif

#include <algorithm>
#include <iterator>

void
WindowList::Clear() noexcept
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
WindowList::Contains(const Window &w) const noexcept
{
  for (const auto &i : list)
    if (&i == &w)
      return true;

  return false;
}

bool
WindowList::IsCovered(const Window &w) const noexcept
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
WindowList::BringToTop(Window &w) noexcept
{
  assert(Contains(w));

  list.erase(list.iterator_to(w));
  list.push_front(w);
}

void
WindowList::BringToBottom(Window &w) noexcept
{
  assert(Contains(w));

  list.erase(list.iterator_to(w));
  list.push_back(w);
}

[[gnu::pure]]
static bool
IsAt(Window &w, PixelPoint p) noexcept
{
  return w.IsVisible() && w.GetPosition().Contains(p);
}

Window *
WindowList::FindAt(PixelPoint p) noexcept
{
  for (Window &w : list)
    if (w.IsEnabled() && IsAt(w, p))
      return &w;

  return nullptr;
}

[[gnu::pure]]
Window *
WindowList::FindControl(List::iterator i,
                        WindowList::List::iterator end) noexcept
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

[[gnu::pure]]
Window *
WindowList::FindControl(WindowList::List::reverse_iterator i,
                        WindowList::List::reverse_iterator end) noexcept
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
WindowList::FindFirstControl() noexcept
{
  return FindControl(list.begin(), list.end());
}

Window *
WindowList::FindLastControl() noexcept
{
  return FindControl(list.rbegin(), list.rend());
}

Window *
WindowList::FindNextChildControl(Window *reference) noexcept
{
  assert(reference != nullptr);
  assert(Contains(*reference));

  auto i = list.iterator_to(*reference);
  assert(i != list.end());

  return FindControl(++i, list.end());
}

Window *
WindowList::FindPreviousChildControl(Window *reference) noexcept
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

[[gnu::pure]]
static bool
IsFullWindow(const Window &w, const PixelRect &rc) noexcept
{
  return w.IsVisible() &&
    w.GetPosition().Contains(rc);
}

void
WindowList::Paint(Canvas &canvas) noexcept
{
  auto begin = list.rbegin(), end = list.rend();

  /* find the last full window which covers all the other windows
     behind it */
  for (auto i = begin; i != end; ++i) {
    Window &child = *i;
    if (IsFullWindow(child, canvas.GetRect()) &&
        !child.IsTransparent())
      begin = i;
  }

  for (auto i = begin; i != end; ++i) {
    PaintWindow &child = (PaintWindow &)*i;
    if (!child.IsVisible())
      continue;

#ifdef USE_MEMORY_CANVAS
    const PixelPoint child_pos = child.GetTopLeft();
    const PixelSize child_size = child.GetSize();
    const bool partially_outside =
      child_pos.x < 0 || child_pos.y < 0 ||
      child_pos.x + (int)child_size.width > (int)canvas.GetWidth() ||
      child_pos.y + (int)child_size.height > (int)canvas.GetHeight();

    if (partially_outside) {
      WritableImageBuffer<ActivePixelTraits> scratch =
        WritableImageBuffer<ActivePixelTraits>::Empty();
      scratch.Allocate(child_size);

      const PixelPoint src_pos{
        std::max(0, -child_pos.x),
        std::max(0, -child_pos.y),
      };
      const PixelPoint dest_pos{
        std::max(0, child_pos.x),
        std::max(0, child_pos.y),
      };
      /* std::min of signed extents can be negative; PixelSize is unsigned and
         would wrap to huge values and bypass the > 0 guards below. */
      const int copy_w = std::max(
        0, std::min((int)child_size.width - src_pos.x,
                    (int)canvas.GetWidth() - dest_pos.x));
      const int copy_h = std::max(
        0, std::min((int)child_size.height - src_pos.y,
                    (int)canvas.GetHeight() - dest_pos.y));
      const PixelSize copy_size{
        static_cast<unsigned>(copy_w),
        static_cast<unsigned>(copy_h),
      };

      {
        Canvas offscreen_canvas(scratch);
        offscreen_canvas.CopyStateFrom(canvas);
        if (copy_size.width > 0 && copy_size.height > 0)
          offscreen_canvas.Copy(src_pos, copy_size, canvas, dest_pos);
        child.OnPaint(offscreen_canvas);

        if (copy_size.width > 0 && copy_size.height > 0)
          canvas.Copy(dest_pos, copy_size, offscreen_canvas, src_pos);
      }

      scratch.Free();
      continue;
    }
#endif

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
