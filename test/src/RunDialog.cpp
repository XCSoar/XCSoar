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

#define ENABLE_XML_DIALOG
#define ENABLE_MAIN_WINDOW
#define ENABLE_CMDLINE
#define USAGE "XMLFILE"

#include "Main.hpp"
#include "Form/Form.hpp"
#include "LocalPath.hpp"
#include "OS/FileUtil.hpp"

#include <tchar.h>
#include <stdio.h>

void VisitDataFiles(const TCHAR* filter, File::Visitor &visitor) {}

static tstring xmlfile;

static void
ParseCommandLine(Args &args)
{
  xmlfile = args.ExpectNextT();
}

static void
Main()
{
  WndForm *form = LoadDialog(NULL, main_window, xmlfile.c_str());
  if (form == NULL) {
    _ftprintf(stderr, _T("Failed to load resource '%s'\n"), xmlfile.c_str());
    return;
  }

  form->ShowModal();
  delete form;
}
