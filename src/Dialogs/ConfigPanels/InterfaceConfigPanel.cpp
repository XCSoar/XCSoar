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

#include "InterfaceConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "Form/Button.hpp"
#include "Form/XMLWidget.hpp"
#include "DataField/FileReader.hpp"
#include "DataField/Enum.hpp"
#include "Dialogs/Dialogs.h"
#include "Util/StringUtil.hpp"
#include "Interface.hpp"
#include "Language/LanguageGlue.hpp"
#include "Asset.hpp"
#include "LocalPath.hpp"
#include "OS/PathName.hpp"
#include "Protection.hpp"
#include "ConfigPanel.hpp"
#include "Language/Language.hpp"

#include <windef.h> /* for MAX_PATH */

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

  const UISettings &settings = CommonInterface::GetUISettings();

  WndProperty *wp;

  buttonFonts = ((WndButton *)ConfigPanel::GetForm().FindByName(_T("cmdFonts")));
  if (buttonFonts)
    buttonFonts->SetOnClickNotify(OnFonts);

#ifdef HAVE_BLANK
  LoadFormProperty(form, _T("prpAutoBlank"),
                   settings.map.EnableAutoBlank);
#else
  ShowFormControl(form, _T("prpAutoBlank"), false);
#endif

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

  if (has_pointer()) {
    static gcc_constexpr_data StaticEnumChoice text_input_list[] = {
      { tiDefault, N_("Default") },
      { tiKeyboard, N_("Keyboard") },
      { tiHighScore, N_("HighScore Style") },
      { 0 }
    };

    LoadFormProperty(form, _T("prpTextInput"), text_input_list,
                     settings.dialog.text_input_style);
  } else {
    /* on-screen keyboard doesn't work without a pointing device
       (mouse or touch screen), hide the option on Altair */
    ShowFormControl(form, _T("prpTextInput"), false);
  }
}

bool
InterfaceConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  UISettings &settings = CommonInterface::SetUISettings();
  bool changed = false, requirerestart = false;;

#ifdef HAVE_BLANK
  changed |= SaveFormProperty(form, _T("prpAutoBlank"),
                              szProfileAutoBlank,
                              settings.map.EnableAutoBlank);
#endif

  if (FinishFileField(form, _T("prpInputFile"), szProfileInputFile)) {
    changed = true;
    requirerestart = true;
  }

#ifndef HAVE_NATIVE_GETTEXT
  WndProperty *wp = (WndProperty *)form.FindByName(_T("prpLanguageFile"));
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

  unsigned menu_timeout = GetFormValueInteger(form, _T("prpMenuTimeout")) * 2;
  if (XCSoarInterface::menu_timeout_max != menu_timeout) {
    XCSoarInterface::menu_timeout_max = menu_timeout;
    Profile::Set(szProfileMenuTimeout, XCSoarInterface::menu_timeout_max);
    changed = true;
  }

  if (has_pointer())
    changed |= SaveFormPropertyEnum(form, _T("prpTextInput"),
                                    szProfileAppTextInputStyle,
                                    settings.dialog.text_input_style);

  _changed |= changed;
  _require_restart |= requirerestart;
  return true;
}

Widget *
CreateInterfaceConfigPanel()
{
  return new InterfaceConfigPanel();
}
