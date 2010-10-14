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
#include "DataField/Boolean.hpp"
#include "DataField/Enum.hpp"
#include "DataField/Integer.hpp"
#include "MainWindow.hpp"

#include <stdio.h>

static WndForm *wf = NULL;
static LOGFONT NewLogFont;
static LOGFONT resetLogFont;
static Font NewFont;

static bool locked;

static void LoadGUI();

static void
OnCloseClicked(WindowControl * Sender)
{
  (void)Sender;
  wf->SetModalResult(mrOK);
}

static void
OnCancelClicked(WindowControl * Sender)
{
  (void)Sender;
  wf->SetModalResult(mrCancel);
}

static void
OnResetClicked(WindowControl * Sender)
{
  (void)Sender;

  NewLogFont = resetLogFont;
  LoadGUI();
}

static void
GetLogFont(LOGFONT &logfont)
{
  WndProperty* wp;
  wp = (WndProperty*)wf->FindByName(_T("prpFontName"));
  if (wp)
    _tcsncpy(logfont.lfFaceName,
             wp->GetDataField()->GetAsString(), LF_FACESIZE - 1);

  wp = (WndProperty*)wf->FindByName(_T("prpFontHeight"));
  if (wp)
    logfont.lfHeight = wp->GetDataField()->GetAsInteger();

  wp = (WndProperty*)wf->FindByName(_T("prpFontWeight"));
  if (wp)
    logfont.lfWeight = wp->GetDataField()->GetAsBoolean() ? 700 : 500;

  wp = (WndProperty*)wf->FindByName(_T("prpFontItalic"));
  if (wp)
    logfont.lfItalic = wp->GetDataField()->GetAsBoolean();
}

static void RedrawSampleFont(void)
{
  GetLogFont(NewLogFont);

#ifdef ENABLE_SDL
  // XXX
#else /* !ENABLE_SDL */
  NewFont.set(NewLogFont);
#endif /* !ENABLE_SDL */

  WndFrame *wp = (WndFrame *)wf->FindByName(_T("prpFontSample"));
  if (wp) {
    if (NewFont.defined()) {
      wp->SetFont(NewFont);
      wp->SetCaption(_T("Sample Text 123"));
    } else {
      wp->SetCaption(_T("Error Creating Font!"));
    }
  }
}

static void
OnData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  if (!locked && Mode == DataField::daChange)
    RedrawSampleFont();
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnData),
  DeclareCallBackEntry(OnResetClicked),
  DeclareCallBackEntry(OnCancelClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

static void
InitGUI(const TCHAR * FontDescription)
{
  #define FONTEDIT_GUI_MAX_TITLE 128

  WndProperty* wp;

  TCHAR sTitle[FONTEDIT_GUI_MAX_TITLE] = _T("Edit Font: ");
  _tcsncat(sTitle, FontDescription, FONTEDIT_GUI_MAX_TITLE);
  wf->SetCaption(sTitle);

  wp = (WndProperty*)wf->FindByName(_T("prpFontName"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_T("Tahoma"));
    dfe->addEnumText(_T("TahomaBD"));
    dfe->addEnumText(_T("DejaVu Sans Condensed"));
    // RLD ToDo code: add more font faces, and validate their availabiliy
  }
}

static void
LoadGUI()
{
  assert(!locked);
  locked = true;

  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpFontName"));
  if (wp) {
    DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
    if (dfe) {
      dfe->Set(0);

      unsigned i;
      for (i = 0; i < dfe->Count(); i++) {
        if (_tcsncmp(dfe->GetAsString(), NewLogFont.lfFaceName, LF_FACESIZE)
            == 0)
          break;

        dfe->Inc();
      }

      if (i == dfe->Count())
        dfe->Set(dfe->addEnumText(NewLogFont.lfFaceName));
    }
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFontHeight"));
  if (wp) {
    DataFieldInteger* dfi = (DataFieldInteger*)wp->GetDataField();
    if (dfi)
      dfi->Set(NewLogFont.lfHeight);

    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(_T("prpFontWeight"));
  if (wp) {
    DataFieldBoolean* dfi = (DataFieldBoolean*)wp->GetDataField();
    if (dfi)
      dfi->Set(NewLogFont.lfWeight > 500);

    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(_T("prpFontItalic"));
  if (wp) {
    DataFieldBoolean* dfb = (DataFieldBoolean*)wp->GetDataField();
    if (dfb)
      dfb->Set(NewLogFont.lfItalic);

    wp->RefreshDisplay();
  }

  locked = false;

  RedrawSampleFont();
}


bool
dlgFontEditShowModal(const TCHAR * FontDescription,
                     LOGFONT &log_font,
                     LOGFONT autoLogFont)
{
  bool bRetVal = false;

  wf = LoadDialog(CallBackTable,
                      XCSoarInterface::main_window, _T("IDR_XML_FONTEDIT"));
  if (wf == NULL)
    return false;

  NewLogFont = log_font;
  resetLogFont = autoLogFont;

  InitGUI(FontDescription);
  LoadGUI();

  if (wf->ShowModal() == mrOK) {
    log_font = NewLogFont;
    bRetVal = true;
  }

  delete wf;

  return bRetVal;
}
