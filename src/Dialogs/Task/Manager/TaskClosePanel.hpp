/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_TASK_CLOSE_PANEL_HPP
#define XCSOAR_TASK_CLOSE_PANEL_HPP

#include "Widget/Widget.hpp"
#include "Form/Frame.hpp"
#include "Form/Button.hpp"

class TaskManagerDialog;

class TaskClosePanel final : public NullWidget {
  enum Buttons {
    CLOSE,
    REVERT,
  };

  struct Layout {
    PixelRect close_button, message, revert_button;

    Layout(PixelRect rc, const DialogLook &look) noexcept;
  };

public:
  TaskManagerDialog &dialog;

private:
  bool *task_modified;

  const DialogLook &look;

  Button close_button;
  WndFrame message;
  Button revert_button;

public:
  TaskClosePanel(TaskManagerDialog &_dialog, bool *_task_modified,
                 const DialogLook &_look) noexcept;

  void CommitAndClose() noexcept;
  void RefreshStatus() noexcept;

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Click() noexcept override;
  void ReClick() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
};

#endif
