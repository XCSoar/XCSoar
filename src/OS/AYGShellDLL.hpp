/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_AYGSHELLDLL_HPP
#define XCSOAR_AYGSHELLDLL_HPP

#include "DynamicLibrary.hpp"

#include <aygshell.h>

class AYGShellDLL : public DynamicLibrary {
protected:
  typedef BOOL WINAPI (*SHFullScreen_t)(HWND hWnd, DWORD flags);
  typedef BOOL WINAPI (*SHHandleWMActivate_t)(HWND hwnd, WPARAM wParam,
                                              LPARAM lParam,
                                              SHACTIVATEINFO *psai,
                                              DWORD dwFlags);
  typedef BOOL WINAPI (*SHHandleWMSettingChange_t)(HWND hwnd, WPARAM wParam,
                                                   LPARAM lParam,
                                                   SHACTIVATEINFO *psai);
  typedef BOOL WINAPI (*SHSetAppKeyWndAssoc_t)(BYTE bVk, HWND hwnd);
  typedef HBITMAP WINAPI (*SHLoadImageFile_t)(LPCTSTR pszFileName);
  typedef HBITMAP WINAPI (*SHLoadImageResource_t)(HINSTANCE hinst,
                                                  UINT uIdImageFile);

  SHFullScreen_t SHFullScreen_p;
  SHHandleWMActivate_t SHHandleWMActivate_p;
  SHHandleWMSettingChange_t SHHandleWMSettingChange_p;
  SHSetAppKeyWndAssoc_t SHSetAppKeyWndAssoc_p;
  SHLoadImageFile_t SHLoadImageFile_p;
  SHLoadImageResource_t SHLoadImageResource_p;

public:
  AYGShellDLL()
    :DynamicLibrary(_T("aygshell")),
     SHFullScreen_p((SHFullScreen_t)Lookup(_T("SHFullScreen"))),
     SHHandleWMActivate_p((SHHandleWMActivate_t)Lookup(_T("SHHandleWMActivate"))),
     SHHandleWMSettingChange_p((SHHandleWMSettingChange_t)Lookup(_T("SHHandleWMSettingChange"))),
     SHSetAppKeyWndAssoc_p((SHSetAppKeyWndAssoc_t)Lookup(_T("SHSetAppKeyWndAssoc"))),
     SHLoadImageFile_p((SHLoadImageFile_t)Lookup(_T("SHLoadImageFile"))),
     SHLoadImageResource_p((SHLoadImageResource_t)Lookup(_T("SHLoadImageResource"))) {}

  BOOL SHFullScreen(HWND hWnd, DWORD flags) const {
    return SHFullScreen_p != NULL
      ? SHFullScreen_p(hWnd, flags)
      : false;
  }

  BOOL SHHandleWMActivate(HWND hwnd, WPARAM wParam, LPARAM lParam,
                          SHACTIVATEINFO *psai, DWORD dwFlags) const {
    return SHHandleWMActivate_p != NULL
      ? SHHandleWMActivate_p(hwnd, wParam, lParam, psai, dwFlags)
      : false;
  }

  BOOL SHHandleWMSettingChange(HWND hwnd, WPARAM wParam, LPARAM lParam,
                               SHACTIVATEINFO *psai) const {
    return SHHandleWMSettingChange_p != NULL
      ? SHHandleWMSettingChange_p(hwnd, wParam, lParam, psai)
      : NULL;
  }

  BOOL SHSetAppKeyWndAssoc(BYTE bVk, HWND hwnd) const {
    return SHSetAppKeyWndAssoc_p != NULL
      ? SHSetAppKeyWndAssoc_p(bVk, hwnd)
      : false;
  }

  HBITMAP SHLoadImageFile(LPCTSTR pszFileName) const {
    return SHLoadImageFile_p != NULL
      ? SHLoadImageFile_p(pszFileName)
      : NULL;
  }

  HBITMAP SHLoadImageResource(HINSTANCE hinst, UINT uIdImageFile) const {
    return SHLoadImageResource_p != NULL
      ? SHLoadImageResource_p(hinst, uIdImageFile)
      : NULL;
  }
};

#endif
