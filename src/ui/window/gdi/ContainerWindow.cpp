// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../ContainerWindow.hpp"

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
