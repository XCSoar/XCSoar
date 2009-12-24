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
#include "XCSoar.h"
#include "Message.h"
#include "Profile.hpp"
#include "Registry.hpp"
#include "UtilsProfile.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "MainWindow.hpp"
#include "SettingsAirspace.hpp"
#include "Airspace/AirspaceClass.hpp"

#include <assert.h>

static WndForm *wf = NULL;
static WndListFrame *wAirspaceList = NULL;

static bool colormode = false;

static void
OnAirspacePaintListItem(Canvas &canvas, const RECT rc, unsigned i)
{
  if (i >= AIRSPACECLASSCOUNT)
    return;

  int w1, w2, x0;
  int w0 = rc.right - rc.left - Layout::FastScale(4);

  w1 = canvas.text_width(gettext(_T("Warn"))) + Layout::FastScale(10);
  w2 = canvas.text_width(gettext(_T("Display"))) + Layout::FastScale(10);
  x0 = w0 - w1 - w2;

  canvas.text_clipped(rc.left + Layout::FastScale(2),
                      rc.top + Layout::FastScale(2),
                      x0 - Layout::FastScale(10), 
                      airspace_class_as_text((AirspaceClass_t)i,false).c_str());

  if (colormode) {
    canvas.white_pen();
    canvas.set_text_color(MapGfx.GetAirspaceColourByClass(i,
        XCSoarInterface::SettingsMap()));
    canvas.set_background_color(Color(0xFF, 0xFF, 0xFF));
    canvas.select(MapGfx.GetAirspaceBrushByClass(i,
        XCSoarInterface::SettingsMap()));
    canvas.rectangle(rc.left + x0, rc.top + Layout::FastScale(2),
                     rc.right - Layout::FastScale(2),
                     rc.bottom - Layout::FastScale(2));
  } else {
    bool iswarn;
    bool isdisplay;

    iswarn = (XCSoarInterface::SettingsComputer().iAirspaceMode[i] >= 2);
    isdisplay = ((XCSoarInterface::SettingsComputer().iAirspaceMode[i] % 2) > 0);

    if (iswarn) {
      canvas.text(rc.left + w0 - w1 - w2, rc.top + Layout::FastScale(2),
                  gettext(_T("Warn")));
    }
    if (isdisplay) {
      canvas.text(rc.left + w0 - w2, rc.top + Layout::FastScale(2), 
                  gettext(_T("Display")));
    }
  }
}

static bool changed = false;

static void
OnAirspaceListEnter(unsigned ItemIndex)
{
  if (ItemIndex >= AIRSPACECLASSCOUNT) {
    ItemIndex = AIRSPACECLASSCOUNT - 1;
  }

  if (colormode) {
    int c = dlgAirspaceColoursShowModal();
    if (c >= 0) {
      XCSoarInterface::SetSettingsMap().iAirspaceColour[ItemIndex] = c;
      SetRegistryColour(ItemIndex,
                        XCSoarInterface::SettingsMap().iAirspaceColour[ItemIndex]);
      changed = true;
    }

    int p = dlgAirspacePatternsShowModal();
    if (p >= 0) {
      XCSoarInterface::SetSettingsMap().iAirspaceBrush[ItemIndex] = p;
      SetRegistryBrush(ItemIndex,
                       XCSoarInterface::SettingsMap().iAirspaceBrush[ItemIndex]);
      changed = true;
    }
  } else {
    int v = (XCSoarInterface::SettingsComputer().iAirspaceMode[ItemIndex] + 1) % 4;
    XCSoarInterface::SetSettingsComputer().iAirspaceMode[ItemIndex] = v;
    Profile::SetRegistryAirspaceMode(ItemIndex);
    changed = true;
  }
}

static void
OnCloseClicked(WindowControl * Sender)
{
  (void)Sender;
  wf->SetModalResult(mrOK);
}

static void
OnLookupClicked(WindowControl * Sender)
{
  (void)Sender;
  dlgAirspaceSelect();
}

static CallBackTableEntry_t CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnLookupClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgAirspaceShowModal(bool coloredit)
{
  colormode = coloredit;

  if (!Layout::landscape)
    wf = dlgLoadFromXML(CallBackTable, _T("dlgAirspace_L.xml"),
        XCSoarInterface::main_window, _T("IDR_XML_AIRSPACE_L"));
  else
    wf = dlgLoadFromXML(CallBackTable, _T("dlgAirspace.xml"),
        XCSoarInterface::main_window, _T("IDR_XML_AIRSPACE"));

  if (!wf)
    return;

  assert(wf != NULL);

  wAirspaceList = (WndListFrame*)wf->FindByName(_T("frmAirspaceList"));
  assert(wAirspaceList!=NULL);
  wAirspaceList->SetBorderKind(BORDERLEFT);
  wAirspaceList->SetActivateCallback(OnAirspaceListEnter);
  wAirspaceList->SetPaintItemCallback(OnAirspacePaintListItem);
  wAirspaceList->SetLength(AIRSPACECLASSCOUNT);

  changed = false;

  wf->ShowModal();

  // now retrieve back the properties...
  if (changed) {
    Profile::StoreRegistry();
    Message::AddMessage(_T("Configuration saved"));
  }

  delete wf;

  wf = NULL;
}
