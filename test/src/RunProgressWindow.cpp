/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Screen/SingleWindow.hpp"
#include "Screen/ProgressWindow.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Init.hpp"
#include "ResourceLoader.hpp"

#ifndef WIN32
int main(int argc, char **argv)
#else
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
#ifdef _WIN32_WCE
        LPWSTR lpCmdLine,
#else
        LPSTR lpCmdLine2,
#endif
        int nCmdShow)
#endif
{
  PixelRect screen_rc{0, 0, 640, 480};

  ScreenGlobalInit screen_init;

#ifdef WIN32
  ResourceLoader::Init(hInstance);
#endif

  Layout::Initialize(screen_rc.right - screen_rc.left,
                     screen_rc.bottom - screen_rc.top);

  SingleWindow main_window;
  main_window.Create(_T("STATIC"), _T("RunProgressWindow"), screen_rc);
  main_window.Show();

  ProgressWindow progress(main_window);
  progress.SetMessage(_T("Testing..."));
  progress.SetRange(0, 1024);
  progress.SetValue(768);

  main_window.RunEventLoop();

  return 0;
}
