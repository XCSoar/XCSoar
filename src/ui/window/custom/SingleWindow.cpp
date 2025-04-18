// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../SingleWindow.hpp"

namespace UI {

[[gnu::pure]]
static const ContainerWindow *
IsAncestor(const Window *maybe_ancestor, const Window *w) noexcept
{
  while (true) {
    const ContainerWindow *parent = w->GetParent();
    if (parent == nullptr)
      return nullptr;

    if (parent == maybe_ancestor)
      return parent;

    w = parent;
  }
}

bool
SingleWindow::FilterMouseEvent(PixelPoint pt,
                               Window *allowed) const noexcept
{
  const ContainerWindow *container = this;
  while (true) {
    const Window *child =
      const_cast<ContainerWindow *>(container)->EventChildAt(pt);
    if (child == nullptr)
      /* no receiver for the event */
      return false;

    if (child == allowed)
      /* the event reaches an allowed window: success */
      return true;

    const ContainerWindow *next = IsAncestor(allowed, child);
    if (next == nullptr)
      return false;

    container = next;
  }
}

} // namespace UI
