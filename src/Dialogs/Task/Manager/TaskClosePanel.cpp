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

#include "TaskClosePanel.hpp"
#include "Internal.hpp"
#include "Form/Button.hpp"
#include "Form/Frame.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Font.hpp"
#include "Look/DialogLook.hpp"
#include "Language/Language.hpp"

TaskClosePanel::Layout::Layout(PixelRect rc, const DialogLook &look) noexcept
{
  const unsigned padding = ::Layout::GetMinimumControlHeight();
  const unsigned button_height = ::Layout::GetMaximumControlHeight();

  close_button.left = rc.left + padding;
  close_button.right = rc.right - padding;
  close_button.top = rc.top + padding;
  close_button.bottom = close_button.top + button_height;

  message.left = close_button.left;
  message.right = close_button.right;
  message.top = close_button.bottom + padding;
  message.bottom = message.top + look.text_font.GetHeight();

  revert_button.left = close_button.left;
  revert_button.right = close_button.right;
  revert_button.top = message.bottom + padding;
  revert_button.bottom = revert_button.top + button_height;
}

TaskClosePanel::TaskClosePanel(TaskManagerDialog &_dialog,
                               bool *_task_modified,
                               const DialogLook &_look) noexcept
  :dialog(_dialog), task_modified(_task_modified),
   look(_look),
   message(look) {}

void
TaskClosePanel::RefreshStatus() noexcept
{
  message.SetText(*task_modified ?
                  _("Task has been modified") : _("Task unchanged"));

  revert_button.SetVisible(*task_modified);
}

void
TaskClosePanel::CommitAndClose() noexcept
{
  if (dialog.Commit())
    dialog.SetModalResult(mrOK);
}

void
TaskClosePanel::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  const Layout layout(rc, look);

  WindowStyle button_style;
  button_style.Hide();
  button_style.TabStop();

  WindowStyle style;
  style.Hide();

  close_button.Create(parent, look.button, _("Close"),
                      layout.close_button, button_style,
                      [this](){ CommitAndClose(); });

  message.Create(parent, layout.message, style);
  message.SetAlignCenter();
  message.SetVAlignCenter();

  revert_button.Create(parent, look.button, _("Revert Changes"),
                       layout.revert_button, button_style,
                       [this](){
                         dialog.Revert();
                         RefreshStatus();
                       });
}

bool
TaskClosePanel::Click() noexcept
{
  if (!(*task_modified)) {
    CommitAndClose();
    return false;
  }

  return true;
}

void
TaskClosePanel::ReClick() noexcept
{
  CommitAndClose();
}

void
TaskClosePanel::Show(const PixelRect &rc) noexcept
{
  RefreshStatus();

  const Layout layout(rc, look);
  close_button.MoveAndShow(layout.close_button);
  message.MoveAndShow(layout.message);
  revert_button.MoveAndShow(layout.revert_button);
}

void
TaskClosePanel::Hide() noexcept
{
  close_button.Hide();
  message.Hide();
  revert_button.Hide();
}

void
TaskClosePanel::Move(const PixelRect &rc) noexcept
{
  const Layout layout(rc, look);
  close_button.Move(layout.close_button);
  message.Move(layout.message);
  revert_button.Move(layout.revert_button);
}

bool
TaskClosePanel::SetFocus() noexcept
{
  close_button.SetFocus();
  return true;
}
