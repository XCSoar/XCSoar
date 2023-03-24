// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"
#include "ui/control/List.hpp"

class Window;

/**
 * A wrapper that turns a #ListControl into a #Widget.  Call
 * CreateList() in your implementation to create and set the
 * #ListControl.  It is not deleted automatically; call
 * WindowWidget::DeleteWindow() to do that at your choice.
 */
class ListWidget
  : public WindowWidget, protected ListItemRenderer,
    protected ListCursorHandler {
protected:
  const ListControl &GetList() const noexcept {
    return (const ListControl &)GetWindow();
  }

  ListControl &GetList() noexcept {
    return (ListControl &)GetWindow();
  }

  ListControl &CreateList(ContainerWindow &parent, const DialogLook &look,
                          const PixelRect &rc, unsigned row_height) noexcept;

public:
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;

  bool SetFocus() noexcept override {
    GetList().SetFocus();
    return true;
  }
};
