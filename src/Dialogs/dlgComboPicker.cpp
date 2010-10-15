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
#include "Dialogs/ListPicker.hpp"
#include "InputEvents.h"
#include "DataField/Base.hpp"
#include "DataField/ComboList.hpp"
#include "Screen/Layout.hpp"

#include <assert.h>

static WndProperty *wComboPopupWndProperty;
static DataField *ComboPopupDataField;
static ComboList *ComboListPopup;

enum { ComboPopupITEMMAX = 100 };
static TCHAR sSavedInitialValue[ComboPopupITEMMAX];
static int iSavedInitialDataIndex = -1;

static void
OnPaintComboPopupListItem(Canvas &canvas, const RECT rc, unsigned i)
{
  assert(i < (unsigned)ComboListPopup->ComboPopupItemCount);

  canvas.text_clipped(rc.left + Layout::FastScale(2),
                      rc.top + Layout::FastScale(2), rc,
                      ComboListPopup->ComboPopupItemList[i]->StringValueFormatted);
}

static void
OnHelpClicked(unsigned i)
{
  if (i < ComboListPopup->ComboPopupItemCount) {
    int iDataIndex = ComboListPopup->ComboPopupItemList[i]->DataFieldIndex;
    ComboPopupDataField->SetFromCombo(iDataIndex,
      ComboListPopup->ComboPopupItemList[i]->StringValue);
  }

  wComboPopupWndProperty->OnHelp();
}

int
dlgComboPicker(SingleWindow &parent, WndProperty *theProperty)
{
  static bool bInComboPicker = false;
  bool bInitialPage = true;
  // used to exit loop (optionally reruns combo with
  // lower/higher index of items for int/float
  bool bOpenCombo = true;

  // prevents multiple instances
  if (bInComboPicker)
    return 0;

  bInComboPicker = true;

  while (bOpenCombo) {
    assert(theProperty != NULL);
    wComboPopupWndProperty = theProperty;

    ComboPopupDataField = wComboPopupWndProperty->GetDataField();
    ComboListPopup = ComboPopupDataField->GetCombo();
    assert(ComboPopupDataField != NULL);

    ComboPopupDataField->CreateComboList();
    if (bInitialPage) { // save values for "Cancel" from first page only
      bInitialPage = false;
      iSavedInitialDataIndex = ComboListPopup->
          ComboPopupItemList[ComboListPopup->ComboPopupItemSavedIndex]->
          DataFieldIndex;
      ComboPopupDataField->CopyString(sSavedInitialValue, false);
    }

    int idx = ListPicker(parent, theProperty->GetCaption(),
                         ComboListPopup->ComboPopupItemCount,
                         ComboListPopup->ComboPopupItemSavedIndex,
                         Layout::Scale(18),
                         OnPaintComboPopupListItem,
                         OnHelpClicked);

    bOpenCombo = false; //tell  combo to exit loop after close

    if (idx >= 0 && (unsigned)idx < ComboListPopup->ComboPopupItemCount) {
      // OK/Select
      if (ComboListPopup->ComboPopupItemList[idx]->DataFieldIndex ==
          ComboList::Item::NEXT_PAGE) {
        // we're last in list and the want more past end of list so select last real list item and reopen
        ComboPopupDataField->SetDetachGUI(true);
        // we'll reopen, so don't call xcsoar data changed routine yet
        --idx;
        bOpenCombo = true; // reopen combo with new selected index at center
      } else if (ComboListPopup->ComboPopupItemList[idx]->DataFieldIndex ==
                 ComboList::Item::PREVIOUS_PAGE) {
        // same as above but lower items needed
        ComboPopupDataField->SetDetachGUI(true);
        ++idx;
        bOpenCombo = true;
      }
      int iDataIndex = ComboListPopup->ComboPopupItemList[idx]->DataFieldIndex;
      ComboPopupDataField->SetFromCombo(iDataIndex,
          ComboListPopup->ComboPopupItemList[idx]->StringValue);
    } else {
      // Cancel
      // if we've detached the GUI during the load, then there is nothing to do here
      assert(iSavedInitialDataIndex >= 0);
      if (iSavedInitialDataIndex >= 0)
        // use statics here - saved from first page if multiple were used
        ComboPopupDataField->SetFromCombo(iSavedInitialDataIndex,
            sSavedInitialValue);
    }

    wComboPopupWndProperty->RefreshDisplay();
    ComboListPopup->Clear();
  } // loop reopen combo if <<More>>  or <<Less>> picked

  bInComboPicker = false;
  return 1;
}
