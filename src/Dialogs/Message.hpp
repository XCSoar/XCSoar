// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

#ifdef _WIN32
#include <windef.h> // for HWND (needed by winuser.h)
#include <winuser.h>
#else

enum {
  IDCANCEL = 3,
  IDOK,
  IDYES,
  IDNO,
  IDRETRY,
  IDABORT,
  IDIGNORE,
};

enum {
  MB_OKCANCEL,
  MB_OK,
  MB_YESNO,
  MB_YESNOCANCEL,
  MB_RETRYCANCEL,
  MB_ABORTRETRYIGNORE,
  MB_ICONINFORMATION = 0x10,
  MB_ICONWARNING = 0x20,
  MB_ICONEXCLAMATION = 0x40,
  MB_ICONQUESTION = 0x80,
  MB_ICONERROR = 0x100,
};

#endif

/**
 * Displays a MessageBox and returns the pressed button
 * @param lpText Text displayed inside the MessageBox
 * @param lpCaption Text displayed in the Caption of the MessageBox
 * @param uType Type of MessageBox to display (OK+Cancel, Yes+No, etc.)
 * @return
 */
int
ShowMessageBox(const char *text, const char *caption,
               unsigned flags) noexcept;
