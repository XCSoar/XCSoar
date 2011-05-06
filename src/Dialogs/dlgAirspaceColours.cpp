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

#include "Dialogs/Internal.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "MainWindow.hpp"

#include <assert.h>

static WndForm *wf = NULL;
static WndListFrame *wAirspaceColoursList = NULL;

static int ItemIndex = -1;

static void
OnAirspaceColoursPaintListItem(Canvas &canvas, const PixelRect rc, unsigned i)
{
  assert(i < NUMAIRSPACECOLORS);

  canvas.black_pen();
#ifdef ENABLE_SDL
  Color color = Graphics::GetAirspaceColour(i);
  canvas.select(Brush(color));
#else
  canvas.set_background_color(COLOR_WHITE);
  canvas.select(Graphics::GetAirspaceBrush(1)); // this is the solid brush
  canvas.set_text_color(Graphics::GetAirspaceColour(i));
#endif
  canvas.rectangle(rc.left + Layout::FastScale(2),
                   rc.top + Layout::FastScale(2),
                   rc.right - Layout::FastScale(2),
                   rc.bottom - Layout::FastScale(2));
}

static void
OnAirspaceColoursListEnter(gcc_unused unsigned i)
{
  wf->SetModalResult(mrOK);
}

static void
OnCloseClicked(gcc_unused WndButton &Sender)
{
  ItemIndex = -1;
  wf->SetModalResult(mrOK);
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

int
dlgAirspaceColoursShowModal(void)
{
  ItemIndex = -1;

  if (!Layout::landscape) {
    wf = LoadDialog(CallBackTable,
        XCSoarInterface::main_window, _T("IDR_XML_AIRSPACECOLOURS_L"));
  } else {
    wf = LoadDialog(CallBackTable,
        XCSoarInterface::main_window, _T("IDR_XML_AIRSPACECOLOURS"));
  }

  if (!wf)
    return -1;

  assert(wf != NULL);

  wAirspaceColoursList = (WndListFrame*)wf->FindByName(
      _T("frmAirspaceColoursList"));
  assert(wAirspaceColoursList != NULL);
  wAirspaceColoursList->SetActivateCallback(OnAirspaceColoursListEnter);
  wAirspaceColoursList->SetPaintItemCallback(OnAirspaceColoursPaintListItem);
  wAirspaceColoursList->SetLength(NUMAIRSPACECOLORS);

  int result = wf->ShowModal();
  result = result == mrOK ? (int)wAirspaceColoursList->GetCursorIndex() : -1;

  // now retrieve back the properties...

  delete wf;

  wf = NULL;

  return result;
}
