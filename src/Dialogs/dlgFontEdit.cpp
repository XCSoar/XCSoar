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

#include "Dialogs/Internal.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Screen/SingleWindow.hpp"
#include "DataField/Boolean.hpp"
#include "DataField/Enum.hpp"
#include "DataField/Integer.hpp"
#include "Util/StringUtil.hpp"
#include "UIGlobals.hpp"
#include "Compiler.h"

#include <stdio.h>

static WndForm *wf = NULL;
static LOGFONT NewLogFont;
static LOGFONT resetLogFont;
static Font NewFont;

static void LoadGUI();

static void
OnCloseClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static void
OnCancelClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrCancel);
}

static void
OnResetClicked(gcc_unused WndButton &Sender)
{
  NewLogFont = resetLogFont;
  LoadGUI();
}

static void
GetLogFont(LOGFONT &logfont)
{
#ifdef USE_GDI
  WndProperty* wp;
  wp = (WndProperty*)wf->FindByName(_T("prpFontName"));
  if (wp)
    CopyString(logfont.lfFaceName,
               wp->GetDataField()->GetAsString(), LF_FACESIZE);
#endif

  logfont.lfHeight = GetFormValueInteger(*wf, _T("prpFontHeight"));
  logfont.lfWeight = GetFormValueBoolean(*wf, _T("prpFontWeight")) ? 700 : 500;
  logfont.lfItalic = GetFormValueBoolean(*wf, _T("prpFontItalic"));
}

static void RedrawSampleFont(void)
{
  GetLogFont(NewLogFont);
  NewFont.Set(NewLogFont);

  WndFrame *wp = (WndFrame *)wf->FindByName(_T("prpFontSample"));
  if (wp) {
    if (NewFont.IsDefined()) {
      wp->SetFont(NewFont);
      wp->SetCaption(_("Sample Text\n123"));
    } else {
      wp->SetCaption(_("Font not found."));
    }
  }
}

static void
OnData(gcc_unused DataField *Sender, DataField::DataAccessKind_t Mode)
{
  if (Mode == DataField::daChange)
    RedrawSampleFont();
}

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnData),
  DeclareCallBackEntry(OnResetClicked),
  DeclareCallBackEntry(OnCancelClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

static void
InitGUI(const TCHAR * FontDescription)
{
  StaticString<128> title;
  title.Format(_T("%s: %s"), _("Edit Font"), FontDescription);
  wf->SetCaption(title);

  WndProperty *wp = (WndProperty*)wf->FindByName(_T("prpFontName"));
  if (wp) {
#ifndef USE_GDI
    /* we cannot obtain a list of fonts on SDL/OpenGL currently */
    wp->hide();
#else
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_T("Tahoma"));
    dfe->addEnumText(_T("TahomaBD"));
    dfe->addEnumText(_T("DejaVu Sans Condensed"));
    // RLD ToDo code: add more font faces, and validate their availabiliy
#endif
  }
}

static void
LoadGUI()
{
#ifdef USE_GDI
  WndProperty *wp = (WndProperty*)wf->FindByName(_T("prpFontName"));
  if (wp) {
    DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->SetStringAutoAdd(NewLogFont.lfFaceName);
    wp->RefreshDisplay();
  }
#endif

  LoadFormProperty(*wf, _T("prpFontHeight"), (unsigned)NewLogFont.lfHeight);
  LoadFormProperty(*wf, _T("prpFontWeight"), NewLogFont.lfWeight > 500);
  LoadFormProperty(*wf, _T("prpFontItalic"), !!NewLogFont.lfItalic);

  RedrawSampleFont();
}


bool
dlgFontEditShowModal(const TCHAR * FontDescription,
                     LOGFONT &log_font,
                     LOGFONT autoLogFont)
{
  bool bRetVal = false;

  wf = LoadDialog(CallBackTable, UIGlobals::GetMainWindow(),
                  _T("IDR_XML_FONTEDIT"));
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

  NewFont.Reset();

  return bRetVal;
}
