/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_DIALOGS_MESSAGE_HPP
#define XCSOAR_DIALOGS_MESSAGE_HPP

#include <tchar.h>

#ifdef WIN32
#include <windows.h>
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
ShowMessageBox(const TCHAR *text, const TCHAR *caption, unsigned flags);

#endif
