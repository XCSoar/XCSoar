/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

#include "Dialogs/Internal.hpp"
#include "Screen/Blank.hpp"
#include "Screen/TopWindow.hpp"
#include "wcecompat/ts_string.h"
#include "Screen/Layout.hpp"
#include "UtilsSystem.hpp"
#include "InputEvents.h"
#include "PopupMessage.hpp"
#include "MapWindow.h"
#include "StatusMessage.hpp"
#include "Asset.hpp"

#include <tchar.h>

#if defined(PNA) || defined(FIVV)
int GlobalModelType = MODELTYPE_UNKNOWN;
bool needclipping = false;
#endif

#ifdef HAVE_BLANK
int DisplayTimeOut = 0;
#endif

bool
FileExistsA(const char *FileName)
{
  FILE *file = fopen(FileName, "r");
  if (file != NULL) {
    fclose(file);
    return(true);
  }
  return false;
}

void
LocalPath(TCHAR *buf, const TCHAR* file, int loc)
{
  _tcscpy(buf, file);
}

void
LocalPathS(char *buf, const TCHAR* file, int loc)
{
  strcpy(buf, (const char *)file);
  //unicode2ascii(file, buf);
}

const TCHAR *
gettext(const TCHAR *text)
{
  return text;
}

int WINAPI
MessageBoxX(LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
{
  return -1;
}

void dlgHelpShowModal(const TCHAR* Caption, const TCHAR* HelpText)
{
}

int DLGSCALE(int x)
{
  return x;
}

pt2Event
InputEvents::findEvent(const TCHAR *)
{
  return NULL;
}

void
PopupMessage::BlockRender(bool doblock)
{
}

#ifndef ENABLE_SDL
bool
MapWindow::identify(HWND hWnd)
{
  return false;
}
#endif /* !ENABLE_SDL */

StatusMessageList::StatusMessageList() {}

HINSTANCE CommonInterface::hInst;
bool CommonInterface::EnableAnimation;

static const StatusMessageList messages;

bool XCSoarInterface::Debounce() { return false; }
void XCSoarInterface::InterfaceTimeoutReset(void) {}

Font MapWindowFont;
Font MapWindowBoldFont;
Font TitleWindowFont;
Font CDIWindowFont;
Font InfoWindowFont;

#ifndef WIN32
int main(int argc, char **argv)
#else
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPTSTR lpCmdLine, int nCmdShow)
#endif
{
#ifdef WIN32
  CommonInterface::hInst = hInstance;

  PaintWindow::register_class(hInstance);
#else
  const TCHAR *lpCmdLine = argv[1];

  if (argc != 2) {
    fprintf(stderr, "Usage: %s XMLFILE\n", argv[0]);
    return 1;
  }
#endif

  TopWindow main_window;
  main_window.set(_T("STATIC"), _T("RunDialog"),
                  0, 0, 640, 480);
  main_window.show();

  WndForm *form = dlgLoadFromXML(NULL, lpCmdLine, main_window);
  if (form == NULL) {
    fprintf(stderr, "Failed to load XML file\n");
    return 1;
  }

  form->ShowModal();
  delete form;

  return 0;
}
