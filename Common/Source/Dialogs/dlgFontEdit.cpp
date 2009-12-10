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
#include "Registry.hpp"
#include "DataField/Boolean.hpp"
#include "DataField/Enum.hpp"
#include "DataField/Integer.hpp"
#include "Screen/Fonts.hpp"
#include "MainWindow.hpp"

extern Font InfoWindowFont;
extern Font TitleWindowFont;
extern Font MapWindowFont;
extern Font TitleSmallWindowFont;
extern Font MapWindowBoldFont;
extern Font CDIWindowFont; // New
extern Font MapLabelFont;
extern Font StatisticsFont;

static WndForm *wf=NULL;
static LOGFONT OriginalLogFont;
static LOGFONT NewLogFont;
static LOGFONT resetLogFont;
static Font NewFont;
const static TCHAR * OriginalFontRegKey;
static bool IsInitialized=false;

void LoadGUI();

static void OnCloseClicked(WindowControl * Sender){
  (void)Sender;
  wf->SetModalResult(mrOK);
}
static void OnCancelClicked(WindowControl * Sender){
  (void)Sender;
  wf->SetModalResult(mrCancel);
}
static void OnResetClicked(WindowControl * Sender){
(void)Sender;

  NewLogFont=resetLogFont;
  LoadGUI();
}



static void RedrawSampleFont(void)
{
  if (!IsInitialized) {
    return;
  }

  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpFontName"));
  if(wp) {
    _tcsncpy(NewLogFont.lfFaceName,wp->GetDataField()->GetAsString(), LF_FACESIZE-1);
  }
  wp = (WndProperty*)wf->FindByName(_T("prpFontHeight"));
  if(wp) {
    NewLogFont.lfHeight = wp->GetDataField()->GetAsInteger();
  }
  wp = (WndProperty*)wf->FindByName(_T("prpFontWeight"));

  if(wp) {
    NewLogFont.lfWeight= wp->GetDataField()->GetAsInteger();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFontItalic"));
  if(wp) {
    if ( wp->GetDataField()->GetAsInteger() ) {
      NewLogFont.lfItalic=1;
    }
    else {
      NewLogFont.lfItalic=0;
    }
  }
  wp = (WndProperty*)wf->FindByName(_T("prpFontPitchAndFamily"));
  if (wp) {
    NewLogFont.lfPitchAndFamily=wp->GetDataField()->GetAsInteger();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFontTrueType"));
  if(wp) {
    if ( wp->GetDataField()->GetAsBoolean() ) {
      NewLogFont.lfQuality = ANTIALIASED_QUALITY;
    }
    else {
      NewLogFont.lfQuality = NONANTIALIASED_QUALITY;
    }
  }

#ifdef ENABLE_SDL
  // XXX
#else /* !ENABLE_SDL */
  NewFont.set(&NewLogFont);
#endif /* !ENABLE_SDL */

  if ( _tcscmp(OriginalFontRegKey, szRegistryFontMapWindowBoldFont) == 0 ) {
    wf->SetFont(NewFont);
    wf->SetTitleFont(NewFont);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFontSample"));

  if(wp) {
    if (NewFont.defined()) {
      wp->SetFont(NewFont);
      wp->SetCaption(_T("Sample Text 123"));
    }
    else {
      wp->SetCaption(_T("Error Creating Font!"));
    }
  }
}




static void OnFontNameData(DataField *Sender, DataField::DataAccessKind_t Mode){
  switch(Mode){
    case DataField::daGet:
    break;

    case DataField::daPut:
    break;

    case DataField::daChange:
      RedrawSampleFont();

    break;
  }
}
static void OnFontWeightData(DataField *Sender, DataField::DataAccessKind_t Mode){
  switch(Mode){
    case DataField::daGet:
    break;

    case DataField::daPut:
    break;

    case DataField::daChange:

      RedrawSampleFont();

    break;
  }
}
static void OnFontHeightData(DataField *Sender, DataField::DataAccessKind_t Mode){
  switch(Mode){
    case DataField::daGet:
    break;

    case DataField::daPut:
    break;

    case DataField::daChange:
      RedrawSampleFont();

    break;
  }
}
static void OnFontItalicData(DataField *Sender, DataField::DataAccessKind_t Mode){
  switch(Mode){
    case DataField::daGet:
    break;

    case DataField::daPut:
    break;

    case DataField::daChange:
      RedrawSampleFont();

    break;
  }
}

static void OnFontTrueTypeData(DataField *Sender, DataField::DataAccessKind_t Mode){
  switch(Mode){
    case DataField::daGet:
    break;

    case DataField::daPut:
    break;

    case DataField::daChange:
      RedrawSampleFont();

    break;
  }
}
static void OnFontPitchAndFamilyData(DataField *Sender, DataField::DataAccessKind_t Mode){
  switch(Mode){
    case DataField::daGet:
    break;

    case DataField::daPut:
    break;

    case DataField::daChange:
      RedrawSampleFont();

    break;
  }
}




static CallBackTableEntry_t CallBackTable[]={

  DeclareCallBackEntry(OnFontTrueTypeData),
  DeclareCallBackEntry(OnFontPitchAndFamilyData),
  DeclareCallBackEntry(OnFontItalicData),
  DeclareCallBackEntry(OnFontWeightData),
  DeclareCallBackEntry(OnFontHeightData),
  DeclareCallBackEntry(OnFontNameData),
  DeclareCallBackEntry(OnResetClicked),
  DeclareCallBackEntry(OnCancelClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};


void SaveValues(const TCHAR * FontRegKey )
{
  // update reg key for font
  TCHAR sValue [256];
  wsprintf(sValue,_T("%d,%d,0,0,%d,%d,0,0,0,0,0,%d,%d,%s"),
                        NewLogFont.lfHeight,
                        NewLogFont.lfWidth,
                        NewLogFont.lfWeight,
                        NewLogFont.lfItalic,
                        NewLogFont.lfQuality,
                        NewLogFont.lfPitchAndFamily,
                        NewLogFont.lfFaceName);
  SetRegistryString(FontRegKey, sValue);
}

void InitGUI(const TCHAR * FontDescription)
{
#define FONTEDIT_GUI_MAX_TITLE 128

  WndProperty* wp;

  TCHAR sTitle[FONTEDIT_GUI_MAX_TITLE];
  TCHAR sTitlePrefix[]=_T("Edit Font: ");

  _tcscpy(sTitle, sTitlePrefix);
  _tcsncpy(sTitle + _tcslen(sTitlePrefix), FontDescription,FONTEDIT_GUI_MAX_TITLE - _tcslen(sTitlePrefix) -1);

  wf->SetCaption(sTitle);

  wp = (WndProperty*)wf->FindByName(_T("prpFontName"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Tahoma")));
    dfe->addEnumText(gettext(_T("TahomaBD")));
    dfe->addEnumText(gettext(_T("DejaVu Sans Condensed")));
    // RLD ToDo code: add more font faces, and validate their availabiliy
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFontPitchAndFamily"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(gettext(_T("Default")));
    dfe->addEnumText(gettext(_T("Fixed")));
    dfe->addEnumText(gettext(_T("Variable")));
  }
}


void LoadGUI()
{
#define MAX_ENUM 10
  IsInitialized=false;
  int i=0;
  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpFontName"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    if (dfe)
    {
      for (i=0 ;i < MAX_ENUM ; i++) {
        dfe->Dec();
      } // rewind

      bool bFound=false;
      for (i=0 ;i < MAX_ENUM ; i++ ) {
        if (_tcsncmp(dfe->GetAsString(), NewLogFont.lfFaceName, LF_FACESIZE) == 0) {
          bFound=true;
          break;
        }
        dfe->Inc();
      }
      if (!bFound) {
        dfe->addEnumText(NewLogFont.lfFaceName);
        for (i=0 ;i < MAX_ENUM ; i++) {
          dfe->Dec();
        } // rewind
        for (i=0 ;i < MAX_ENUM ; i++ ) {
          if (_tcsncmp(dfe->GetAsString(), NewLogFont.lfFaceName,LF_FACESIZE) == 0) {
            break;
          }
          dfe->Inc();
        }
      }
    }
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFontHeight"));
  if (wp) {
    DataFieldInteger * dfi;
    dfi = (DataFieldInteger*)wp->GetDataField();
    if (dfi)
    {
      dfi->Set(NewLogFont.lfHeight);
    }
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(_T("prpFontWeight"));
  if (wp) {
    DataFieldInteger* dfi;
    dfi = (DataFieldInteger*)wp->GetDataField();
    if (dfi)
    {
      dfi->Set(NewLogFont.lfWeight);
    }
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(_T("prpFontItalic"));
  if (wp) {
    DataFieldBoolean* dfb;
    dfb = (DataFieldBoolean*)wp->GetDataField();
    if (dfb)
    {
      dfb->Set(NewLogFont.lfItalic);
    }
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(_T("prpFontPitchAndFamily"));
  if (wp) {
    DataFieldEnum * dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    if (dfe)
    {
      dfe->SetAsInteger(NewLogFont.lfPitchAndFamily);
    }
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFontTrueType"));
  if (wp) {
    DataFieldBoolean* dfb;
    dfb = (DataFieldBoolean*)wp->GetDataField();
    if (dfb)
    {
      dfb->Set(NewLogFont.lfQuality == ANTIALIASED_QUALITY);
    }
    wp->RefreshDisplay();
  }

  IsInitialized=true;

  RedrawSampleFont();
}


bool dlgFontEditShowModal(const TCHAR * FontDescription,
                          const TCHAR * FontRegKey,
                          LOGFONT autoLogFont){

  bool bRetVal=false;

  IsInitialized=false;

  wf = dlgLoadFromXML(CallBackTable,
                      _T("dlgFontEdit.xml"),
		      XCSoarInterface::main_window,
		      _T("IDR_XML_FONTEDIT"));
  if (wf == NULL)
    return false;

  int UseCustomFonts_old = UseCustomFonts;
  UseCustomFonts=1;// global var
  InitializeOneFont (&NewFont,
                        FontRegKey,
                        autoLogFont,
                        &OriginalLogFont);
  UseCustomFonts=UseCustomFonts_old;


  OriginalFontRegKey=FontRegKey;
  NewLogFont=OriginalLogFont;
  resetLogFont = autoLogFont;

  InitGUI(FontDescription);
  LoadGUI();

  if (wf->ShowModal()==mrOK) {
    SaveValues(FontRegKey);
    bRetVal=true;
  }

  delete wf;

  return bRetVal;
}


