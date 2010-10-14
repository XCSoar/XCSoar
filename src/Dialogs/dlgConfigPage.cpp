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

#include "Screen/Layout.hpp"
#include "Pages.hpp"
#include "MainWindow.hpp"

using namespace Pages;

static WndForm *wf = NULL;
static PageLayout PageLayouts[4];

static void
OnOKClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static void
OnCancelClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrCancel);
}

static void
OnListItemPaint(Canvas &canvas, const RECT rc, unsigned i)
{
  using namespace Pages;

  if (i > 3)
    return;

  TCHAR buffer[255];
  PageLayouts[i].MakeTitle(buffer);

  canvas.text(rc.left + Layout::FastScale(2), rc.top + Layout::FastScale(2),
              buffer);
}

static void
OnListActivate(unsigned i)
{
  wf->SetModalResult(mrOK);
}

static void
FillList(WndListFrame* list, bool include_empty)
{
  PageLayouts[0].Type = PageLayout::t_Map;
  PageLayouts[0].MapInfoBoxes = PageLayout::mib_Normal;

  PageLayouts[1].Type = PageLayout::t_Map;
  PageLayouts[1].MapInfoBoxes = PageLayout::mib_Aux;

  PageLayouts[2].Type = PageLayout::t_Map;
  PageLayouts[2].MapInfoBoxes = PageLayout::mib_None;

  PageLayouts[3].Type = PageLayout::t_Empty;

  list->SetLength(include_empty ? 4 : 3);
  list->SetOrigin(0);
  list->SetCursorIndex(0);
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(NULL)
};

PageLayout
dlgConfigPageShowModal(PageLayout pl, bool include_empty)
{
  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  Layout::landscape ?
                  _T("IDR_XML_CONFIG_PAGE_L") : _T("IDR_XML_CONFIG_PAGE"));
  if (!wf)
    return pl;

  ((WndButton *)wf->FindByName(_T("cmdOk")))->
      SetOnClickNotify(OnOKClicked);
  ((WndButton *)wf->FindByName(_T("cmdCancel")))->
      SetOnClickNotify(OnCancelClicked);

  WndListFrame* list = NULL;
  list = ((WndListFrame *)wf->FindByName(_T("lstPageLayouts")));
  assert(list != NULL);
  list->SetPaintItemCallback(OnListItemPaint);
  list->SetActivateCallback(OnListActivate);
  FillList(list, include_empty);

  bool res = wf->ShowModal();
  delete wf;

  return (res == mrOK ? PageLayouts[list->GetCursorIndex()] : pl);
}

