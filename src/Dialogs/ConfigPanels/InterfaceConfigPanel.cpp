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

#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "Form/Button.hpp"
#include "Form/XMLWidget.hpp"
#include "DataField/Enum.hpp"
#include "DataField/Boolean.hpp"
#include "DataField/FileReader.hpp"
#include "Dialogs/Dialogs.h"
#include "Util/StringUtil.hpp"
#include "Interface.hpp"
#include "Language/LanguageGlue.hpp"
#include "Asset.hpp"
#include "LocalPath.hpp"
#include "OS/PathName.hpp"
#include "Protection.hpp"
#include "ConfigPanel.hpp"
#include "InterfaceConfigPanel.hpp"
#include "Language/Language.hpp"

using namespace ConfigPanel;

class InterfaceConfigPanel : public XMLWidget {
  WndButton *buttonFonts;

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
};

void
InterfaceConfigPanel::Show(const PixelRect &rc)
{
  XMLWidget::Show(rc);
  buttonFonts->show();
}

void
InterfaceConfigPanel::Hide()
{
  XMLWidget::Hide();
  buttonFonts->hide();
}

static void
OnFonts(gcc_unused WndButton &button)
{
  dlgConfigFontsShowModal();
}

void
InterfaceConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent, N_("IDR_XML_INTERFACECONFIGPANEL"));

  WndProperty *wp;

  buttonFonts = ((WndButton *)ConfigPanel::GetForm().FindByName(_T("cmdFonts")));
  if (buttonFonts)
    buttonFonts->SetOnClickNotify(OnFonts);

  wp = (WndProperty*)form.FindByName(_T("prpAutoBlank"));
  if (wp) {
#ifdef HAVE_BLANK
    DataFieldBoolean *df = (DataFieldBoolean *)wp->GetDataField();
    df->Set(XCSoarInterface::SettingsMap().EnableAutoBlank);
    wp->RefreshDisplay();
#else
    wp->hide();
#endif
  }

  InitFileField(form, _T("prpInputFile"), szProfileInputFile, _T("*.xci\0"));

  wp = (WndProperty *)form.FindByName(_T("prpLanguageFile"));
  if (wp != NULL) {
#ifdef HAVE_NATIVE_GETTEXT
    wp->hide();
#else /* !HAVE_NATIVE_GETTEXT */
    DataFieldEnum &df = *(DataFieldEnum *)wp->GetDataField();
    df.addEnumText(_("Automatic"));
    df.addEnumText(_("English"));

#ifdef HAVE_BUILTIN_LANGUAGES
    for (const struct builtin_language *l = language_table;
         l->resource != NULL; ++l)
      df.addEnumText(l->resource);
#endif

    DataFieldFileReader files(NULL);
    files.ScanDirectoryTop(_T("*.mo"));
    for (unsigned i = 0; i < files.size(); ++i) {
      const TCHAR *path = files.getItem(i);
      if (path == NULL)
        continue;

      path = BaseName(path);
      if (path != NULL && !df.Exists(path))
        df.addEnumText(path);
    }

    df.Sort(2);

    TCHAR value[MAX_PATH];
    if (!Profile::GetPath(szProfileLanguageFile, value))
      value[0] = _T('\0');

    if (_tcscmp(value, _T("none")) == 0)
      df.Set(1);
    else if (!string_is_empty(value) && _tcscmp(value, _T("auto")) != 0) {
      const TCHAR *base = BaseName(value);
      if (base != NULL)
        df.SetAsString(base);
    }

    wp->RefreshDisplay();
#endif /* !HAVE_NATIVE_GETTEXT */
  }

  InitFileField(form, _T("prpStatusFile"), szProfileStatusFile, _T("*.xcs\0"));

  LoadFormProperty(form, _T("prpMenuTimeout"),
                   XCSoarInterface::menu_timeout_max / 2);

  LoadFormProperty(form, _T("prpDebounceTimeout"),
                   XCSoarInterface::debounce_timeout);

  wp = (WndProperty*)form.FindByName(_T("prpTextInput"));
  assert(wp != NULL);
  if (has_pointer()) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Default"));
    dfe->addEnumText(_("Keyboard"));
    dfe->addEnumText(_("HighScore Style"));
    dfe->Set(CommonInterface::GetUISettings().dialog.text_input_style);
    wp->RefreshDisplay();
  } else {
    /* on-screen keyboard doesn't work without a pointing device
       (mouse or touch screen), hide the option on Altair */
    wp->hide();
  }

}

bool
InterfaceConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, requirerestart = false;;
  WndProperty *wp;

#ifdef HAVE_BLANK
  changed |= SaveFormProperty(form, _T("prpAutoBlank"),
                              szProfileAutoBlank,
                              XCSoarInterface::SetSettingsMap().EnableAutoBlank);
#endif

  if (FinishFileField(form, _T("prpInputFile"), szProfileInputFile)) {
    changed = true;
    requirerestart = true;
  }

#ifndef HAVE_NATIVE_GETTEXT
  wp = (WndProperty *)form.FindByName(_T("prpLanguageFile"));
  if (wp != NULL) {
    DataFieldEnum &df = *(DataFieldEnum *)wp->GetDataField();

    TCHAR old_value[MAX_PATH];
    if (!Profile::GetPath(szProfileLanguageFile, old_value))
      old_value[0] = _T('\0');

    const TCHAR *old_base = BaseName(old_value);
    if (old_base == NULL)
      old_base = old_value;

    TCHAR buffer[MAX_PATH];
    const TCHAR *new_value, *new_base;

    switch (df.GetAsInteger()) {
    case 0:
      new_value = new_base = _T("auto");
      break;

    case 1:
      new_value = new_base = _T("none");
      break;

    default:
      _tcscpy(buffer, df.GetAsString());
      ContractLocalPath(buffer);
      new_value = buffer;
      new_base = BaseName(new_value);
      if (new_base == NULL)
        new_base = new_value;
      break;
    }

    if (_tcscmp(old_value, new_value) != 0 &&
        _tcscmp(old_base, new_base) != 0) {
      Profile::Set(szProfileLanguageFile, new_value);
      LanguageChanged = changed = true;
    }
  }
#endif

  if (FinishFileField(form, _T("prpStatusFile"), szProfileStatusFile)) {
    changed = true;
    requirerestart = true;
  }

  wp = (WndProperty*)form.FindByName(_T("prpMenuTimeout"));
  if (wp) {
    if ((int)XCSoarInterface::menu_timeout_max != wp->GetDataField()->GetAsInteger()*2) {
      XCSoarInterface::menu_timeout_max = wp->GetDataField()->GetAsInteger()*2;
      Profile::Set(szProfileMenuTimeout,XCSoarInterface::menu_timeout_max);
      changed = true;
    }
  }

  changed |= SaveFormProperty(form, _T("prpDebounceTimeout"),
                              szProfileDebounceTimeout,
                              XCSoarInterface::debounce_timeout);


  DialogSettings &dialog_settings = CommonInterface::SetUISettings().dialog;
  if (has_pointer()) {
    wp = (WndProperty*)form.FindByName(_T("prpTextInput"));
    assert(wp != NULL);
    if (dialog_settings.text_input_style != (TextInputStyle_t)(wp->GetDataField()->GetAsInteger())) {
      dialog_settings.text_input_style =
        (TextInputStyle_t)(wp->GetDataField()->GetAsInteger());
      Profile::Set(szProfileAppTextInputStyle,
                   dialog_settings.text_input_style);
      changed = true;
    }
  }

  _changed |= changed;
  _require_restart |= requirerestart;
  return true;
}

Widget *
CreateInterfaceConfigPanel()
{
  return new InterfaceConfigPanel();
}
