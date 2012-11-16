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

#define ENABLE_XML_DIALOG
#define ENABLE_CMDLINE
#define USAGE "XMLFILE [-portrait]"

#include "Main.hpp"
#include "Form/Form.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "StatusMessage.hpp"
#include "Asset.hpp"
#include "LocalPath.hpp"
#include "OS/PathName.hpp"
#include "OS/FileUtil.hpp"

#include <tchar.h>
#include <stdio.h>

#ifdef WIN32
#include <shellapi.h>
#endif

void VisitDataFiles(const TCHAR* filter, File::Visitor &visitor) {}

static tstring xmlfile;
static bool portrait;

static void
ParseCommandLine(Args &args)
{
  xmlfile = args.ExpectNextT();

  const char *p = args.PeekNext();
  if (p != NULL) {
    if (strcmp(p, "-portrait") == 0) {
      args.Skip();
      portrait = true;
    }
  }
}

static void
Main()
{
  PixelRect screen_rc{0, 0, 320, 240};
  if (portrait) {
    screen_rc.right = 240;
    screen_rc.bottom = 320;
  }

  Layout::Initialize(screen_rc.right - screen_rc.left,
                     screen_rc.bottom - screen_rc.top);
  SingleWindow main_window;
  main_window.Create(_T("RunDialog"), screen_rc);
  main_window.Show();

  WndForm *form = LoadDialog(NULL, main_window, xmlfile.c_str());
  if (form == NULL) {
    fprintf(stderr, "Failed to load resource '%s'\n",
            (const char *)NarrowPathName(xmlfile.c_str()));
    return;
  }

  form->ShowModal();
  delete form;
}
