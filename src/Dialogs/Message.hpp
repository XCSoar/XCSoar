// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/* Portable copies of the Win32 message-box constants.  ShowMessageBox
   is XCSoar's own dialog; these names match the Win32 values so a
   prior windows.h include is harmless. */

#ifndef IDOK
enum {
  IDOK = 1,
  IDCANCEL = 2,
  IDABORT = 3,
  IDRETRY = 4,
  IDIGNORE = 5,
  IDYES = 6,
  IDNO = 7,
};
#endif

#ifndef MB_OK
enum {
  MB_OK = 0x0000,
  MB_OKCANCEL = 0x0001,
  MB_ABORTRETRYIGNORE = 0x0002,
  MB_YESNOCANCEL = 0x0003,
  MB_YESNO = 0x0004,
  MB_RETRYCANCEL = 0x0005,

  MB_ICONERROR = 0x10,
  MB_ICONQUESTION = 0x20,
  MB_ICONEXCLAMATION = 0x30,
  MB_ICONWARNING = 0x30,
  MB_ICONINFORMATION = 0x40,
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
