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

#include "Dialogs/Dialogs.h"
#include "Dialogs/FontEdit.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Form.hpp"
#include "Form/Util.hpp"
#include "Form/Frame.hpp"
#include "Form/Button.hpp"
#include "Form/Edit.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Profile/FontConfig.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Font.hpp"
#include "UIGlobals.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Util/StringUtil.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Compiler.h"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Cache.hpp"
#endif

#include <assert.h>

static Font TempInfoWindowFont;
static Font TempTitleWindowFont;
static Font TempMapWindowFont;
static Font TempTitleSmallWindowFont;
static Font TempMapWindowBoldFont;
static Font TempCDIWindowFont;
static Font TempMapLabelFont;
static Font TempMapLabelImportantFont;

extern LOGFONT log_infobox;
extern LOGFONT log_title;
extern LOGFONT log_map;
extern LOGFONT log_infobox_small;
extern LOGFONT log_map_bold;
extern LOGFONT log_cdi;
extern LOGFONT log_map_label;
extern LOGFONT log_map_label_important;

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

  font.Load(log_font);
}

static void
ResetFonts(bool bUseCustom)
{
  ResetFont(TempInfoWindowFont, bUseCustom,
            ProfileKeys::FontInfoWindowFont, log_infobox);
  ResetFont(TempTitleWindowFont, bUseCustom,
            ProfileKeys::FontTitleWindowFont, log_title);
  ResetFont(TempMapWindowFont, bUseCustom,
            ProfileKeys::FontMapWindowFont, log_map);
  ResetFont(TempTitleSmallWindowFont, bUseCustom,
            ProfileKeys::FontTitleSmallWindowFont, log_infobox_small);
  ResetFont(TempMapWindowBoldFont, bUseCustom,
            ProfileKeys::FontMapWindowBoldFont, log_map_bold);
  ResetFont(TempCDIWindowFont, bUseCustom,
            ProfileKeys::FontCDIWindowFont, log_cdi);
  ResetFont(TempMapLabelFont, bUseCustom,
            ProfileKeys::FontMapLabelFont, log_map_label);
  ResetFont(TempMapLabelImportantFont, bUseCustom,
            ProfileKeys::FontMapLabelImportantFont, log_map_label_important);
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
RefreshPreview(const TCHAR *name, Font &font)
{
  WndFrame *sample = (WndFrame *)wf->FindByName(name);
  assert(sample != nullptr);

  sample->SetFont(font);
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

#ifdef ENABLE_OPENGL
  TextCache::Flush();
#endif

  // now set SampleTexts on the Fonts frame
  RefreshPreview(_T("prpInfoWindowFont"), TempInfoWindowFont);
  RefreshPreview(_T("prpTitleWindowFont"), TempTitleWindowFont);
  RefreshPreview(_T("prpMapWindowFont"), TempMapWindowFont);
  RefreshPreview(_T("prpTitleSmallWindowFont"), TempTitleSmallWindowFont);
  RefreshPreview(_T("prpMapWindowBoldFont"), TempMapWindowBoldFont);
  RefreshPreview(_T("prpCDIWindowFont"), TempCDIWindowFont);
  RefreshPreview(_T("prpMapLabelFont"), TempMapLabelFont);
  RefreshPreview(_T("prpMapLabelImportantFont"), TempMapLabelImportantFont);
}

static void
OnUseCustomFontData(DataField *Sender, DataField::DataAccessMode Mode)
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
  EditFont(_T("prpInfoWindowFont"), ProfileKeys::FontInfoWindowFont, log_infobox);
}

static void
OnEditTitleWindowFontClicked(gcc_unused WndButton &button)
{
  EditFont(_T("prpTitleWindowFont"), ProfileKeys::FontTitleWindowFont, log_title);
}

static void
OnEditMapWindowFontClicked(gcc_unused WndButton &button)
{
  EditFont(_T("prpMapWindowFont"), ProfileKeys::FontMapWindowFont, log_map);
}

static void
OnEditTitleSmallWindowFontClicked(gcc_unused WndButton &button)
{
  EditFont(_T("prpTitleSmallWindowFont"), ProfileKeys::FontTitleSmallWindowFont,
           log_infobox_small);
}

static void
OnEditMapWindowBoldFontClicked(gcc_unused WndButton &button)
{
  EditFont(_T("prpMapWindowBoldFont"), ProfileKeys::FontMapWindowBoldFont,
           log_map_bold);
}

static void
OnEditCDIWindowFontClicked(gcc_unused WndButton &button)
{
  EditFont(_T("prpCDIWindowFont"), ProfileKeys::FontCDIWindowFont, log_cdi);
}

static void
OnEditMapLabelFontClicked(gcc_unused WndButton &button)
{
  EditFont(_T("prpMapLabelFont"), ProfileKeys::FontMapLabelFont, log_map_label);
}

static void
OnEditMapLabelImportantFontClicked(gcc_unused WndButton &button)
{
  EditFont(_T("prpMapLabelImportantFont"), ProfileKeys::FontMapLabelImportantFont,
           log_map_label_important);
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static constexpr CallBackTableEntry CallBackTable[] = {
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
  wf = LoadDialog(CallBackTable, UIGlobals::GetMainWindow(),
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
                              ProfileKeys::UseCustomFonts,
                              ui_settings.custom_fonts);

  delete wf;

  if (changed) {
    Profile::Save();

    ShowMessageBox(_("Changes to configuration saved.  Restart XCSoar to apply changes."),
                _T(""), MB_OK);
  }

  TempInfoWindowFont.Destroy();
  TempTitleWindowFont.Destroy();
  TempMapWindowFont.Destroy();
  TempTitleSmallWindowFont.Destroy();
  TempMapWindowBoldFont.Destroy();
  TempCDIWindowFont.Destroy();
  TempMapLabelFont.Destroy();
  TempMapLabelImportantFont.Destroy();
}
