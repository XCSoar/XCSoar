// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../ContainerWindow.hpp"
#include "../Window.hpp"

Window *
ContainerWindow::GetFocusedWindow() noexcept
{
  const HWND h_focus = ::GetFocus();
  if (h_focus == nullptr)
    return nullptr;

  for (HWND h = h_focus; h != nullptr; h = ::GetParent(h)) {
    if (h == hWnd)
      return GetChecked(hWnd);

    if (!::IsChild(hWnd, h))
      return nullptr;

    if (Window *const w = GetChecked(h); w != nullptr)
      return w;
  }

  return nullptr;
}

bool
ContainerWindow::FocusFirstControl() noexcept
{
  HWND hControl = ::GetNextDlgTabItem(hWnd, nullptr, false);
  if (hControl == nullptr)
    return false;

  ::SetFocus(hControl);
  return true;
}

bool
ContainerWindow::FocusNextControl() noexcept
{
  HWND hControl = ::GetNextDlgTabItem(hWnd, ::GetFocus(), false);
  if (hControl == nullptr)
    return false;

  ::SetFocus(hControl);
  return true;
}

bool
ContainerWindow::FocusPreviousControl() noexcept
{
  HWND hFocus = ::GetFocus();

  HWND hControl = ::GetNextDlgTabItem(hWnd, hFocus, true);
  if (hControl == nullptr)
    return false;

  ::SetFocus(hControl);
  return true;
}
