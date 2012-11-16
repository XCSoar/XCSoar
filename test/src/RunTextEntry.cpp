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

#include "UIGlobals.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Init.hpp"
#include "Fonts.hpp"
#include "Look/DialogLook.hpp"
#include "ResourceLoader.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Util/Macros.hpp"
#include "LocalPath.hpp"

void VisitDataFiles(const TCHAR* filter, File::Visitor &visitor) {}

static DialogSettings dialog_settings;
static DialogLook dialog_look;
static SingleWindow main_window;

const DialogSettings &
UIGlobals::GetDialogSettings()
{
  return dialog_settings;
}

const DialogLook &
UIGlobals::GetDialogLook()
{
  return dialog_look;
}

SingleWindow &
UIGlobals::GetMainWindow()
{
  return main_window;
}

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

#ifdef USE_GDI
  ResourceLoader::Init(hInstance);
#endif

  InitialiseFonts();

  dialog_settings.SetDefaults();
  dialog_look.Initialise(bold_font, normal_font, small_font,
                         bold_font, bold_font);
  SetXMLDialogLook(dialog_look);

  main_window.Create(_T("RunTextEntry"), PixelRect{0, 0, 640, 480});
  main_window.Show();

  TCHAR text[64] = _T("");
  dlgTextEntryShowModal(text, ARRAY_SIZE(text), _T("The caption"));

  main_window.Destroy();

  DeinitialiseFonts();

  return 0;
}
