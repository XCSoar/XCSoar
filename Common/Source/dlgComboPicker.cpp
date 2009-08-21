/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

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

#include "StdAfx.h"
#include "externs.h"
#include "Units.h"
#include "device.h"
#include "InputEvents.h"
#include "WindowControls.h"
#include "dlgTools.h"
#include "InfoBoxLayout.h"
#include "Screen/Util.hpp"

#include <assert.h>

extern HWND   hWndMainWindow;
static WndForm *wf=NULL;

WndProperty * wComboPopupWndProperty;
WindowControl * wComboPopupListEntry;//RLD DEBUG
WndListFrame *wComboPopupListFrame; // RLD used to iterate datafield options
DataField * ComboPopupDataField = NULL;
ComboList * ComboListPopup=NULL;

static TCHAR sSavedInitialValue[ComboPopupITEMMAX];
static int iSavedInitialDataIndex=-1;

static void OnPaintComboPopupListItem(WindowControl * Sender, HDC hDC){

  (void)Sender;

  if ( ComboListPopup->ComboPopupDrawListIndex >= 0 &&
        ComboListPopup->ComboPopupDrawListIndex < ComboListPopup->ComboPopupItemCount )
  {

    int w;
    if (InfoBoxLayout::landscape) {
      w = 202*InfoBoxLayout::scale;
    } else {
      w = 225*InfoBoxLayout::scale;  // was 225.  xml was 226
    }
    w=Sender->GetWidth();

    ExtTextOutClip(hDC, 2*InfoBoxLayout::scale,
                  2*InfoBoxLayout::scale,
                  ComboListPopup->ComboPopupItemList[ComboListPopup->ComboPopupDrawListIndex]->StringValueFormatted,
                  w-InfoBoxLayout::scale*5);
  }
}

static void OnComboPopupListInfo(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo)
{ // callback function for the ComboPopup
  (void)Sender;
  if (ListInfo->DrawIndex == -1){ // initialize

    ListInfo->ItemCount = ComboListPopup->ComboPopupItemCount;
    ListInfo->ScrollIndex = 0;
    ListInfo->ItemIndex = ComboListPopup->ComboPopupItemSavedIndex;

  }
  else {
    ComboListPopup->ComboPopupDrawListIndex = ListInfo->DrawIndex + ListInfo->ScrollIndex;
    ComboListPopup->ComboPopupItemIndex=ListInfo->ItemIndex + ListInfo->ScrollIndex;
  }
}


static void OnHelpClicked(WindowControl * Sender){
  (void)Sender;
  if (ComboListPopup->ComboPopupItemIndex >=0) {

    int iDataIndex = ComboListPopup->ComboPopupItemList[ComboListPopup->ComboPopupItemIndex]->DataFieldIndex;
    ComboPopupDataField->SetFromCombo(iDataIndex,
      ComboListPopup->ComboPopupItemList[ComboListPopup->ComboPopupItemIndex]->StringValue);
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
  ComboListPopup->ComboPopupItemIndex= -1;
  wf->SetModalResult(mrCancle);
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnComboPopupListInfo),
  DeclareCallBackEntry(OnPaintComboPopupListItem),
  DeclareCallBackEntry(OnHelpClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnCancelClicked),
  DeclareCallBackEntry(NULL)
};







int dlgComboPicker(WndProperty* theProperty){

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


    if (!InfoBoxLayout::landscape) {
      wf = dlgLoadFromXML(CallBackTable,
                          TEXT("dlgComboPicker_L.xml"),
                          hWndMainWindow,
                          TEXT("IDR_XML_COMBOPICKER_L"));
    } else {
      wf = dlgLoadFromXML(CallBackTable,
                          TEXT("dlgWayComboPicker.xml"),
                          hWndMainWindow,
                          TEXT("IDR_XML_COMBOPICKER"));
    }

    if (!wf) return -1;

    assert(wf!=NULL);
    //assert(wf->GetWidth() <1200);  // sometimes we have a bogus window, setfocus goes nuts

    wf->SetCaption(theProperty->GetCaption());

    wComboPopupListFrame = (WndListFrame*)wf->FindByName(TEXT("frmComboPopupList"));
    assert(wComboPopupListFrame!=NULL);
    wComboPopupListFrame->SetBorderKind(BORDERLEFT | BORDERTOP | BORDERRIGHT|BORDERBOTTOM);
    wComboPopupListFrame->SetEnterCallback(OnComboPopupListEnter);

    // allow item to be focused / hightlighted
    wComboPopupListEntry = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmComboPopupListEntry"));
    assert(wComboPopupListEntry!=NULL);
    wComboPopupListEntry->SetCanFocus(true);
    wComboPopupListEntry->SetFocused(true, wComboPopupWndProperty->GetHandle());


    ComboPopupDataField = wComboPopupWndProperty->GetDataField();
    ComboListPopup = ComboPopupDataField->GetCombo();
    assert(ComboPopupDataField!=NULL);

    ComboPopupDataField->CreateComboList();
    wComboPopupListFrame->ResetList();
    wComboPopupListFrame->SetItemIndex(ComboListPopup->ComboPopupItemSavedIndex);
    if (bInitialPage) { // save values for "Cancel" from first page only
      bInitialPage=false;
      iSavedInitialDataIndex=ComboListPopup->ComboPopupItemList[ComboListPopup->ComboPopupItemSavedIndex]->DataFieldIndex;
      ComboPopupDataField->CopyString(sSavedInitialValue,false);
    }


    wf->ShowModal();

    bOpenCombo=false;  //tell  combo to exit loop after close

    if (ComboListPopup->ComboPopupItemIndex >=0) // OK/Select
    {
      if (ComboListPopup->ComboPopupItemList[ComboListPopup->ComboPopupItemIndex]->DataFieldIndex
                          ==ComboPopupReopenMOREDataIndex)
      { // we're last in list and the want more past end of list so select last real list item and reopen
        ComboPopupDataField->SetDetachGUI(true);  // we'll reopen, so don't call xcsoar data changed routine yet
        ComboListPopup->ComboPopupItemIndex--;
        bOpenCombo=true; // reopen combo with new selected index at center
      }
      else if (ComboListPopup->ComboPopupItemList[ComboListPopup->ComboPopupItemIndex]->DataFieldIndex
                          ==ComboPopupReopenLESSDataIndex) // same as above but lower items needed
      {
        ComboPopupDataField->SetDetachGUI(true);
        ComboListPopup->ComboPopupItemIndex++;
        bOpenCombo=true;
      }
      int iDataIndex = ComboListPopup->ComboPopupItemList[ComboListPopup->ComboPopupItemIndex]->DataFieldIndex;
      ComboPopupDataField->SetFromCombo(iDataIndex,
        ComboListPopup->ComboPopupItemList[ComboListPopup->ComboPopupItemIndex]->StringValue);
    }
    else // Cancel
    { // if we've detached the GUI during the load, then there is nothing to do here
      assert(iSavedInitialDataIndex >=0);
      if (iSavedInitialDataIndex >=0) {
        /// use statics here - saved from first page if multiple were used
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
