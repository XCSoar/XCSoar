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

#include "Dialogs/Dialogs.h"
#include "Dialogs/Internal.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Profile/FontConfig.hpp"
#include "Screen/Layout.hpp"
#include "MainWindow.hpp"
#include "DataField/Boolean.hpp"
#include "Util/StringUtil.hpp"
#include "Compiler.h"

#include <assert.h>

static Font TempInfoWindowFont;
static Font TempTitleWindowFont;
static Font TempMapWindowFont;
static Font TempTitleSmallWindowFont;
static Font TempMapWindowBoldFont;
static Font TempCDIWindowFont;
static Font TempMapLabelFont;
static Font TempMapLabelImportantFont;

extern LOGFONT LogInfoBox;
extern LOGFONT LogTitle;
extern LOGFONT LogMap;
extern LOGFONT LogInfoBoxSmall;
extern LOGFONT LogMapBold;
extern LOGFONT LogCDI;
extern LOGFONT LogMapLabel;
extern LOGFONT LogMapLabelImportant;

static bool changed = false;
static bool FontRegistryChanged = false;
static WndForm *wf = NULL;

static void
ResetFont(Font &font, bool custom, const TCHAR *profile_key,
          const LOGFONT &default_log_font)
{
  LOGFONT log_font;
  if (!custom || !Profile::GetFont(profile_key, &log_font))
    log_font = default_log_font;

  font.Set(log_font);
}

static void
ResetFonts(bool bUseCustom)
{
  ResetFont(TempInfoWindowFont, bUseCustom,
            szProfileFontInfoWindowFont, LogInfoBox);
  ResetFont(TempTitleWindowFont, bUseCustom,
            szProfileFontTitleWindowFont, LogTitle);
  ResetFont(TempMapWindowFont, bUseCustom,
            szProfileFontMapWindowFont, LogMap);
  ResetFont(TempTitleSmallWindowFont, bUseCustom,
            szProfileFontTitleSmallWindowFont, LogInfoBoxSmall);
  ResetFont(TempMapWindowBoldFont, bUseCustom,
            szProfileFontMapWindowBoldFont, LogMapBold);
  ResetFont(TempCDIWindowFont, bUseCustom,
            szProfileFontCDIWindowFont, LogCDI);
  ResetFont(TempMapLabelFont, bUseCustom,
            szProfileFontMapLabelFont, LogMapLabel);
  ResetFont(TempMapLabelImportantFont, bUseCustom,
            szProfileFontMapLabelImportantFont, LogMapLabelImportant);
}

static void
ShowFontEditButtons(bool bVisible)
{
  ShowFormControl(*wf, _T("cmdInfoWindowFont"), bVisible);
  ShowFormControl(*wf, _T("cmdTitleWindowFont"), bVisible);
  ShowFormControl(*wf, _T("cmdMapWindowFont"), bVisible);
  ShowFormControl(*wf, _T("cmdTitleSmallWindowFont"), bVisible);
  ShowFormControl(*wf, _T("cmdMapWindowBoldFont"), bVisible);
  ShowFormControl(*wf, _T("cmdCDIWindowFont"), bVisible);
  ShowFormControl(*wf, _T("cmdMapLabelFont"), bVisible);
  ShowFormControl(*wf, _T("cmdMapLabelImportantFont"), bVisible);
}

static void
RefreshFonts()
{
  WndProperty * wp;

  wp = (WndProperty*)wf->FindByName(_T("prpUseCustomFonts"));
  if (wp) {
    bool bUseCustomFonts =
        ((DataFieldBoolean*)(wp->GetDataField()))->GetAsBoolean();
    ResetFonts(bUseCustomFonts);
    ShowFontEditButtons(bUseCustomFonts);
  }

  // now set SampleTexts on the Fonts frame
  WndFrame *sample;

  sample = (WndFrame *)wf->FindByName(_T("prpInfoWindowFont"));
  if (sample)
    sample->SetFont(TempInfoWindowFont);

  sample = (WndFrame *)wf->FindByName(_T("prpTitleWindowFont"));
  if (sample)
    sample->SetFont(TempTitleWindowFont);

  sample = (WndFrame *)wf->FindByName(_T("prpMapWindowFont"));
  if (sample)
    sample->SetFont(TempMapWindowFont);

  sample = (WndFrame *)wf->FindByName(_T("prpTitleSmallWindowFont"));
  if (sample)
    sample->SetFont(TempTitleSmallWindowFont);

  sample = (WndFrame *)wf->FindByName(_T("prpMapWindowBoldFont"));
  if (sample)
    sample->SetFont(TempMapWindowBoldFont);

  sample = (WndFrame *)wf->FindByName(_T("prpCDIWindowFont"));
  if (sample)
    sample->SetFont(TempCDIWindowFont);

  sample = (WndFrame *)wf->FindByName(_T("prpMapLabelFont"));
  if (sample)
    sample->SetFont(TempMapLabelFont);

  sample = (WndFrame *)wf->FindByName(_T("prpMapLabelImportantFont"));
  if (sample)
    sample->SetFont(TempMapLabelImportantFont);
}

static void
OnUseCustomFontData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch (Mode) {
  case DataField::daChange:
    RefreshFonts();

    break;

  case DataField::daSpecial:
    return;
  }
}

static void
GetFontDescription(TCHAR Description[], const TCHAR * prpName, int iMaxLen)
{
  const WndFrame *wp = (WndFrame *)wf->FindByName(prpName);
  if (wp)
    CopyString(Description, wp->GetCaption(), iMaxLen);
}

static void
EditFont(const TCHAR *prp_name, const TCHAR *profile_key,
         const LOGFONT &log_font)
{
  // updates registry for font info and updates LogFont values
#define MAX_EDITFONT_DESC_LEN 100
  TCHAR FontDesc[MAX_EDITFONT_DESC_LEN];

  LOGFONT custom_log_font;
  if (!Profile::GetFont(profile_key, &custom_log_font))
    custom_log_font = log_font;

  GetFontDescription(FontDesc, prp_name, MAX_EDITFONT_DESC_LEN);
  if (dlgFontEditShowModal(FontDesc, custom_log_font, log_font)) {
    Profile::SetFont(profile_key, custom_log_font);
    FontRegistryChanged = true;
    RefreshFonts();
  }
}

static void
OnEditInfoWindowFontClicked(gcc_unused WndButton &button)
{
  EditFont(_T("prpInfoWindowFont"), szProfileFontInfoWindowFont, LogInfoBox);
}

static void
OnEditTitleWindowFontClicked(gcc_unused WndButton &button)
{
  EditFont(_T("prpTitleWindowFont"), szProfileFontTitleWindowFont, LogTitle);
}

static void
OnEditMapWindowFontClicked(gcc_unused WndButton &button)
{
  EditFont(_T("prpMapWindowFont"), szProfileFontMapWindowFont, LogMap);
}

static void
OnEditTitleSmallWindowFontClicked(gcc_unused WndButton &button)
{
  EditFont(_T("prpTitleSmallWindowFont"), szProfileFontTitleSmallWindowFont,
           LogInfoBoxSmall);
}

static void
OnEditMapWindowBoldFontClicked(gcc_unused WndButton &button)
{
  EditFont(_T("prpMapWindowBoldFont"), szProfileFontMapWindowBoldFont,
           LogMapBold);
}

static void
OnEditCDIWindowFontClicked(gcc_unused WndButton &button)
{
  EditFont(_T("prpCDIWindowFont"), szProfileFontCDIWindowFont, LogCDI);
}

static void
OnEditMapLabelFontClicked(gcc_unused WndButton &button)
{
  EditFont(_T("prpMapLabelFont"), szProfileFontMapLabelFont, LogMapLabel);
}

static void
OnEditMapLabelImportantFontClicked(gcc_unused WndButton &button)
{
  EditFont(_T("prpMapLabelImportantFont"), szProfileFontMapLabelImportantFont,
           LogMapLabelImportant);
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnUseCustomFontData),
  DeclareCallBackEntry(OnEditInfoWindowFontClicked),
  DeclareCallBackEntry(OnEditTitleWindowFontClicked),
  DeclareCallBackEntry(OnEditMapWindowFontClicked),
  DeclareCallBackEntry(OnEditTitleSmallWindowFontClicked),
  DeclareCallBackEntry(OnEditMapWindowBoldFontClicked),
  DeclareCallBackEntry(OnEditCDIWindowFontClicked),
  DeclareCallBackEntry(OnEditMapLabelFontClicked),
  DeclareCallBackEntry(OnEditMapLabelImportantFontClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

void dlgConfigFontsShowModal()
{
  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  Layout::landscape ?
                  _T("IDR_XML_CONFIG_FONTS_L") : _T("IDR_XML_CONFIG_FONTS"));

  if (wf == NULL)
    return;

  UISettings &ui_settings = CommonInterface::SetUISettings();

  LoadFormProperty(*wf, _T("prpUseCustomFonts"), ui_settings.custom_fonts);

  ShowFontEditButtons(ui_settings.custom_fonts);
  RefreshFonts();

  FontRegistryChanged = false;
  changed = false;

  wf->ShowModal();

  changed |= SaveFormProperty(*wf, _T("prpUseCustomFonts"),
                              szProfileUseCustomFonts,
                              ui_settings.custom_fonts);

  TempInfoWindowFont.Reset();
  TempTitleWindowFont.Reset();
  TempMapWindowFont.Reset();
  TempTitleSmallWindowFont.Reset();
  TempMapWindowBoldFont.Reset();
  TempCDIWindowFont.Reset();
  TempMapLabelFont.Reset();
  TempMapLabelImportantFont.Reset();

  if (changed) {
    Profile::Save();

    MessageBoxX(_("Changes to configuration saved.  Restart XCSoar to apply changes."),
                _T(""), MB_OK);
  }

  delete wf;
}
