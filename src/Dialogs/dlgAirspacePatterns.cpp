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

#include "Dialogs/Airspace.hpp"
#include "Dialogs/Internal.hpp"
#include "Screen/Layout.hpp"
#include "MainWindow.hpp"
#include "Look/Look.hpp"

#include <assert.h>

/* brush patterns are only available on GDI */
#ifdef HAVE_HATCHED_BRUSH

static WndForm *wf = NULL;

static void
OnAirspacePatternsPaintListItem(Canvas &canvas, const RECT rc, unsigned i)
{
  assert(i < NUMAIRSPACECOLORS);

  const AirspaceLook &look = CommonInterface::main_window.look->map.airspace;

  canvas.background_transparent();
  canvas.select(look.brushes[i]);
  canvas.set_text_color(COLOR_BLACK);
  canvas.rectangle(rc.left + Layout::FastScale(2),
                   rc.top + Layout::FastScale(2),
                   rc.right - Layout::FastScale(2),
                   rc.bottom - Layout::FastScale(2));
}

static void
OnAirspacePatternsListEnter(gcc_unused unsigned i)
{
  wf->SetModalResult(mrOK);
}

static void
OnCloseClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrCancel);
}


static gcc_constexpr_data CallBackTableEntry CallBackTable[]={
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};


int dlgAirspacePatternsShowModal(void){
  if (!Layout::landscape) {
    wf = LoadDialog(CallBackTable,
                        XCSoarInterface::main_window,
                        _T("IDR_XML_AIRSPACEPATTERNS_L"));
  } else {
    wf = LoadDialog(CallBackTable,
                        XCSoarInterface::main_window,
                        _T("IDR_XML_AIRSPACEPATTERNS"));
  }

  if (!wf)
    return -1;

  assert(wf!=NULL);

  WndListFrame *wAirspacePatternsList =
    (WndListFrame*)wf->FindByName(_T("frmAirspacePatternsList"));
  assert(wAirspacePatternsList!=NULL);
  wAirspacePatternsList->SetActivateCallback(OnAirspacePatternsListEnter);
  wAirspacePatternsList->SetPaintItemCallback(OnAirspacePatternsPaintListItem);
  wAirspacePatternsList->SetLength(NUMAIRSPACEBRUSHES);

  int result = wf->ShowModal();
  result = result == mrOK
    ? (int)wAirspacePatternsList->GetCursorIndex()
    : -1;

  // now retrieve back the properties...
  delete wf;
  wf = NULL;

  return result;
}

#endif /* HAVE_HATCHED_BRUSH */
