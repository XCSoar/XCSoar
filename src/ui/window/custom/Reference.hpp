// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticArray.hxx"

#include <cassert>

/**
 * A stable reference to a Window.  "Stable" means it notices when the
 * referenced Window has been destroyed.  It is used to remeber which
 * Window was focused before a new dialog was opened on top.
 */
class WindowReference {
  StaticArray<const ContainerWindow*, 6> parents;
  Window *window;

public:
  /**
   * Construct an empty reference, that points to no Window.
   */
  WindowReference():window(nullptr) {}

  WindowReference(const ContainerWindow &root, Window &_window)
    :window(&_window) {
    const ContainerWindow *parent = window->GetParent();
    while (true) {
      if (parent == &root)
        return;

      if (parent == nullptr || parents.full()) {
        window = nullptr;
        return;
      }

      parents.append(parent);
      parent = parent->GetParent();
    }
  }

  bool Defined() const {
    return window != nullptr;
  }

  /**
   * Check if the referenced Window still exists, and return it.
   * Returns nullptr if the referenced Window does not exist anymore.
   */
  Window *Get(const ContainerWindow &root) const {
    assert(window != nullptr);

    const ContainerWindow *parent = &root;
    for (int i = parents.size() - 1; i >= 0; --i) {
      const ContainerWindow &current = *parents[i];
      if (!parent->HasChild(current))
        return nullptr;

      parent = &current;
    }

    if (!parent->HasChild(*window))
      return nullptr;

    return window;
  }
};
