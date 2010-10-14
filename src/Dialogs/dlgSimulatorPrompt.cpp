/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
  Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Screen/Bitmap.hpp"
#include "Dialogs/Internal.hpp"
#include "resource.h"
#include "Screen/Layout.hpp"
#include "MainWindow.hpp"
#include "Simulator.hpp"
#include "Version.hpp"

#include <stdio.h>

#ifdef SIMULATOR_AVAILABLE

static WndForm *wf = NULL;

static void
OnSplashPaint(WindowControl *Sender, Canvas &canvas)
{
  Bitmap splash_bitmap;
  splash_bitmap.load((Layout::scale_1024 > 1024 * 3 / 2) ?
                     IDB_SWIFT : IDB_SWIFT2);
  BitmapCanvas bitmap_canvas(canvas, splash_bitmap);
  canvas.stretch(bitmap_canvas);
}

static void
OnSimulatorClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static void
OnFlyClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrCancel);
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnSplashPaint),
  DeclareCallBackEntry(NULL)
};

#endif

bool
dlgSimulatorPromptShowModal()
{
#ifdef SIMULATOR_AVAILABLE
  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  Layout::landscape ? _T("IDR_XML_SIMULATORPROMPT_L") :
                                      _T("IDR_XML_SIMULATORPROMPT"));
  assert(wf != NULL);

  TCHAR temp[MAX_PATH];
  _stprintf(temp, _T("XCSoar v%s"), XCSoar_VersionString);

  WindowControl* wc;
  wc = ((WindowControl *)wf->FindByName(_T("lblVersion")));
  assert(wc != NULL);
  wc->SetCaption(temp);

  WndButton* wb;
  wb = ((WndButton *)wf->FindByName(_T("cmdSimulator")));
  assert(wb != NULL);
  wb->SetOnClickNotify(OnSimulatorClicked);

  wb = ((WndButton *)wf->FindByName(_T("cmdFly")));
  assert(wb != NULL);
  wb->SetOnClickNotify(OnFlyClicked);

  bool retval = (wf->ShowModal() == mrOK);

  delete wf;

  return retval;
#else
  return false;
#endif
}

