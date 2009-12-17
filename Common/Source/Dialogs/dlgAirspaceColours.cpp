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

#include "Dialogs/Internal.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "MainWindow.hpp"

#include <assert.h>

static WndForm *wf=NULL;
static WndListFrame *wAirspaceColoursList=NULL;

static int ItemIndex = -1;

static void
OnAirspaceColoursPaintListItem(Canvas &canvas, const RECT rc, unsigned i)
{
  if (i >= NUMAIRSPACECOLORS)
    return;

  canvas.white_brush();
  canvas.black_pen();
  canvas.set_background_color(Color::WHITE);
  canvas.select(MapGfx.GetAirspaceBrush(1)); // this is the solid brush
  canvas.set_text_color(MapGfx.GetAirspaceColour(i));
  canvas.rectangle(rc.left + Layout::FastScale(2),
                   rc.top + Layout::FastScale(2),
                   rc.right - Layout::FastScale(2),
                   rc.bottom - Layout::FastScale(2));
}


static void OnAirspaceColoursListEnter(WindowControl * Sender,
				WndListFrame::ListInfo_t *ListInfo) {
  (void)Sender;
  wf->SetModalResult(mrOK);
}

static void OnCloseClicked(WindowControl * Sender){
  (void)Sender;
  ItemIndex = -1;
  wf->SetModalResult(mrOK);
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};


int dlgAirspaceColoursShowModal(void){

  ItemIndex = -1;

  if (!Layout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgAirspaceColours_L.xml"),
                        XCSoarInterface::main_window,
                        _T("IDR_XML_AIRSPACECOLOURS_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgAirspaceColours.xml"),
                        XCSoarInterface::main_window,
                        _T("IDR_XML_AIRSPACECOLOURS"));
  }

  if (!wf) return -1;

  assert(wf!=NULL);

  wAirspaceColoursList = (WndListFrame*)wf->FindByName(_T("frmAirspaceColoursList"));
  assert(wAirspaceColoursList!=NULL);
  wAirspaceColoursList->SetBorderKind(BORDERLEFT);
  wAirspaceColoursList->SetEnterCallback(OnAirspaceColoursListEnter);
  wAirspaceColoursList->SetPaintItemCallback(OnAirspaceColoursPaintListItem);
  wAirspaceColoursList->SetLength(NUMAIRSPACECOLORS);

  int result = wf->ShowModal();
  result = result == mrOK
    ? wAirspaceColoursList->GetCursorIndex()
    : -1;

  // now retrieve back the properties...

  delete wf;

  wf = NULL;

  return result;
}
