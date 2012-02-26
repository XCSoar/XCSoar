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

#include "Dialogs/Airspace.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/Internal.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"
#include "Look/MapLook.hpp"

#include <assert.h>

static WndForm *wf = NULL;
static WndListFrame *wAirspaceColoursList = NULL;

static int ItemIndex = -1;

static void
OnAirspaceColoursPaintListItem(Canvas &canvas, const PixelRect rc, unsigned i)
{
  assert(i < NUMAIRSPACECOLORS);

  const AirspaceLook &look = UIGlobals::GetMapLook().airspace;

  canvas.SelectBlackPen();
#ifndef HAVE_HATCHED_BRUSH
  canvas.Select(look.solid_brushes[i]);
#else
  canvas.SetBackgroundTransparent();
  canvas.Select(look.brushes[1]); // this is the solid brush
  canvas.SetTextColor(look.preset_colors[i]);
#endif
  canvas.Rectangle(rc.left + Layout::FastScale(2),
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

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

int
dlgAirspaceColoursShowModal()
{
  ItemIndex = -1;

  wf = LoadDialog(CallBackTable, UIGlobals::GetMainWindow(),
                  Layout::landscape
                  ? _T("IDR_XML_AIRSPACECOLOURS")
                  : _T("IDR_XML_AIRSPACECOLOURS_L"));
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
