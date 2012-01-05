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
#include "Profile/Profile.hpp"
#include "Form/Button.hpp"
#include "Form/Util.hpp"
#include "Form/Form.hpp"
#include "Screen/Layout.hpp"
#include "Form/RowFormWidget.hpp"
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
#include "UIGlobals.hpp"

#include <windef.h> /* for MAX_PATH */

enum ControlIndex {
#ifdef HAVE_BLANK
  AutoBlank,
#endif
  InputFile,
#ifndef HAVE_NATIVE_GETTEXT
  LanguageFile,
#endif
  StatusFile,
  MenuTimeout,
  TextInput
};

class InterfaceConfigPanel : public RowFormWidget {
public:
  InterfaceConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook(), Layout::Scale(150)), buttonFonts(0) {}

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
  buttonFonts->show();
  RowFormWidget::Show(rc);
}

void
InterfaceConfigPanel::Hide()
{
  buttonFonts->hide();
  RowFormWidget::Hide();
}

static void
OnFonts(gcc_unused WndButton &button)
{
  dlgConfigFontsShowModal();
}

void
InterfaceConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const UISettings &settings = CommonInterface::GetUISettings();
  WndProperty *wp;

  RowFormWidget::Prepare(parent, rc);

  buttonFonts = ((WndButton *)ConfigPanel::GetForm().FindByName(_T("cmdFonts")));
  assert(buttonFonts);
  buttonFonts->SetOnClickNotify(OnFonts);

#ifdef HAVE_BLANK
  AddBoolean(_("Auto. blank"),
             _("This determines whether to blank the display after a long period of inactivity "
                 "when operating on internal battery power."),
             settings.enable_auto_blank);
#endif

  // Expert item (TODO)
  AddFileReader(_("Events"),
                _("The Input Events file defines the menu system and how XCSoar responds to "
                    "button presses and events from external devices."),
                szProfileInputFile, _T("*.xci\0"));

#ifndef HAVE_NATIVE_GETTEXT
  wp = AddEnum(_("Language"),
               _("The language options selects translations for English texts to other "
                   "languages. Select English for a native interface or Automatic to localise "
                   "XCSoar according to the system settings."));
  if (wp != NULL) {
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
  }
#endif /* !HAVE_NATIVE_GETTEXT */

  // Expert item
  AddFileReader(_("Status message"),
                _("The status file can be used to define sounds to be played when certain "
                    "events occur, and how long various status messages will appear on screen."),
                szProfileStatusFile, _T("*.xcs\0"));

  // Expert item
  AddInteger(_("Menu timeout"),
             _("This determines how long menus will appear on screen if the user does not make any button "
                 "presses or interacts with the computer."),
             _T("%u s"), _T("%u"), 1, 60, 1, settings.menu_timeout / 2);


  static gcc_constexpr_data StaticEnumChoice text_input_list[] = {
    { tiDefault, N_("Default") },
    { tiKeyboard, N_("Keyboard") },
    { tiHighScore, N_("HighScore Style") },
    { 0 }
  };

  // Expert item
  wp = AddEnum(_("Text input style"),
               _("Determines how the user is prompted for text input (filename, teamcode etc.)"),
               text_input_list, settings.dialog.text_input_style);

  /* on-screen keyboard doesn't work without a pointing device
       (mouse or touch screen), hide the option on Altair */
  wp->set_visible(HasPointer());
}

bool
InterfaceConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  UISettings &settings = CommonInterface::SetUISettings();
  bool changed = false, require_restart = false;;

#ifdef HAVE_BLANK
  changed |= SaveValue(AutoBlank, szProfileAutoBlank, settings.enable_auto_blank);
#endif

  require_restart |= changed |= SaveValueFileReader(InputFile, szProfileInputFile);

#ifndef HAVE_NATIVE_GETTEXT
  WndProperty *wp = (WndProperty *)&GetControl(LanguageFile);
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

  require_restart |= changed |= SaveValueFileReader(StatusFile, szProfileStatusFile);

  unsigned menu_timeout;
  SaveValue(MenuTimeout, menu_timeout);
  menu_timeout *= 2;
  if (settings.menu_timeout != menu_timeout) {
    settings.menu_timeout = menu_timeout;
    Profile::Set(szProfileMenuTimeout, menu_timeout);
    changed = true;
  }

  if (HasPointer())
    changed |= SaveValueEnum(TextInput, szProfileAppTextInputStyle, settings.dialog.text_input_style);

  _changed |= changed;
  _require_restart |= require_restart;
  return true;
}

Widget *
CreateInterfaceConfigPanel()
{
  return new InterfaceConfigPanel();
}
