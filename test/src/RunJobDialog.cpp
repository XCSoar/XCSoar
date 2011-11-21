/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Dialogs/JobDialog.hpp"
#include "Look/DialogLook.hpp"
#include "Fonts.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Init.hpp"
#include "Screen/Layout.hpp"
#include "Job.hpp"
#include "Operation.hpp"
#include "ResourceLoader.hpp"

class TestJob : public Job {
public:
  virtual void Run(OperationEnvironment &env) {
    env.SetText(_T("Working..."));
    env.SetProgressRange(30);
    for (unsigned i = 0; i < 30 && !env.IsCancelled(); ++i) {
      env.SetProgressPosition(i);
      env.Sleep(500);
    }
  }
};

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
  ScreenGlobalInit screen_init;

#ifdef WIN32
  ResourceLoader::Init(hInstance);
#endif

  InitialiseFonts();
  DialogLook *look = new DialogLook();
  look->Initialise(bold_font, normal_font, bold_font, bold_font);

  Layout::Initialize(320,240);
  SingleWindow main_window;
  main_window.set(_T("STATIC"), _T("RunProgressWindow"),
                  0, 0, 640, 480);
  main_window.show();

  TestJob job;
  JobDialog(main_window, *look, _T("RunJobDialog"), job);

  delete look;
  DeinitialiseFonts();

  return 0;
}
