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
#include "Units.hpp"
#include "Device/device.h"
#include "InputEvents.h"
#include "DataField/Base.hpp"
#include "DataField/ComboList.hpp"
#include "Screen/Layout.hpp"

#include <assert.h>

static WndForm *wf=NULL;

static WndProperty *wComboPopupWndProperty;
static DataField *ComboPopupDataField;
static ComboList *ComboListPopup;
static WndListFrame *wComboPopupListFrame;

static TCHAR sSavedInitialValue[ComboPopupITEMMAX];
static int iSavedInitialDataIndex=-1;

static void
OnPaintComboPopupListItem(Canvas &canvas, const RECT rc, unsigned i)
{
  if (i >= (unsigned)ComboListPopup->ComboPopupItemCount)
    return;

  canvas.text_clipped(rc.left + Layout::FastScale(2),
                      rc.top + Layout::FastScale(2), rc,
                      ComboListPopup->ComboPopupItemList[i]->StringValueFormatted);
}

static void OnHelpClicked(WindowControl * Sender){
  (void)Sender;

  int i = wComboPopupListFrame->GetCursorIndex();

  if (i >= 0 && i < ComboListPopup->ComboPopupItemCount) {
    int iDataIndex = ComboListPopup->ComboPopupItemList[i]->DataFieldIndex;
    ComboPopupDataField->SetFromCombo(iDataIndex,
      ComboListPopup->ComboPopupItemList[i]->StringValue);
  }

  wComboPopupWndProperty->OnHelp();
}

static void OnCloseClicked(WindowControl * Sender){
  (void)Sender;
  wf->SetModalResult(mrOK);
}

static void OnComboPopupListEnter(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo)
{ // double-click on item -- NOT in callback table because added manually
  (void)Sender;
  OnCloseClicked(Sender);
}

static void OnCancelClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrCancel);
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnHelpClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnCancelClicked),
  DeclareCallBackEntry(NULL)
};

int
dlgComboPicker(ContainerWindow &parent, WndProperty *theProperty)
{
  static bool bInComboPicker=false;
  bool bInitialPage=true;
  bool bOpenCombo=true; // used to exit loop (optionally reruns combo with
                        //lower/higher index of items for int/float

  if (bInComboPicker) // prevents multiple instances
    return 0;
  else
    bInComboPicker=true;

  while (bOpenCombo)
  {
    assert(theProperty!=NULL);
    wComboPopupWndProperty = theProperty;


    if (!Layout::landscape) {
      wf = dlgLoadFromXML(CallBackTable,
                          _T("dlgComboPicker_L.xml"),
                          parent,
                          _T("IDR_XML_COMBOPICKER_L"));
    } else {
      wf = dlgLoadFromXML(CallBackTable,
                          _T("dlgWayComboPicker.xml"),
                          parent,
                          _T("IDR_XML_COMBOPICKER"));
    }

    if (!wf) return -1;

    assert(wf!=NULL);
    //assert(wf->GetWidth() <1200);  // sometimes we have a bogus window, setfocus goes nuts

    wf->SetCaption(theProperty->GetCaption());

    wComboPopupListFrame =
      (WndListFrame*)wf->FindByName(_T("frmComboPopupList"));
    assert(wComboPopupListFrame!=NULL);
    wComboPopupListFrame->SetBorderKind(BORDERLEFT | BORDERTOP | BORDERRIGHT|BORDERBOTTOM);
    wComboPopupListFrame->SetEnterCallback(OnComboPopupListEnter);
    wComboPopupListFrame->SetPaintItemCallback(OnPaintComboPopupListItem);

    ComboPopupDataField = wComboPopupWndProperty->GetDataField();
    ComboListPopup = ComboPopupDataField->GetCombo();
    assert(ComboPopupDataField!=NULL);

    ComboPopupDataField->CreateComboList();
    wComboPopupListFrame->SetLength(ComboListPopup->ComboPopupItemCount);
    wComboPopupListFrame->SetCursorIndex(ComboListPopup->ComboPopupItemSavedIndex);
    if (bInitialPage) { // save values for "Cancel" from first page only
      bInitialPage=false;
      iSavedInitialDataIndex=ComboListPopup->ComboPopupItemList[ComboListPopup->ComboPopupItemSavedIndex]->DataFieldIndex;
      ComboPopupDataField->CopyString(sSavedInitialValue,false);
    }


    int idx = wf->ShowModal() == mrOK
      ? wComboPopupListFrame->GetCursorIndex()
      : -1;

    bOpenCombo=false;  //tell  combo to exit loop after close

    if (idx >= 0 && idx < ComboListPopup->ComboPopupItemCount) // OK/Select
    {
      if (ComboListPopup->ComboPopupItemList[idx]->DataFieldIndex
                          ==ComboPopupReopenMOREDataIndex)
      { // we're last in list and the want more past end of list so select last real list item and reopen
        ComboPopupDataField->SetDetachGUI(true);  // we'll reopen, so don't call xcsoar data changed routine yet
        --idx;
        bOpenCombo=true; // reopen combo with new selected index at center
      }
      else if (ComboListPopup->ComboPopupItemList[idx]->DataFieldIndex
                          ==ComboPopupReopenLESSDataIndex) // same as above but lower items needed
      {
        ComboPopupDataField->SetDetachGUI(true);
        ++idx;
        bOpenCombo=true;
      }
      int iDataIndex = ComboListPopup->ComboPopupItemList[idx]->DataFieldIndex;
      ComboPopupDataField->SetFromCombo(iDataIndex,
        ComboListPopup->ComboPopupItemList[idx]->StringValue);
    }
    else // Cancel
    { // if we've detached the GUI during the load, then there is nothing to do here
      assert(iSavedInitialDataIndex >=0);
      if (iSavedInitialDataIndex >=0) {
        // use statics here - saved from first page if multiple were used
        ComboPopupDataField->SetFromCombo(iSavedInitialDataIndex, sSavedInitialValue);
      }
    }


    wComboPopupWndProperty->RefreshDisplay();
    ComboListPopup->FreeComboPopupItemList();

    delete wf;

    wf = NULL;

  } // loop reopen combo if <<More>> << LESS>> picked

  bInComboPicker=false;
  return 1;

}
