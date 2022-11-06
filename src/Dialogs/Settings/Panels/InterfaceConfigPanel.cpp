/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Dialogs/Dialogs.h"
#include "util/StringCompare.hxx"
#include "Interface.hpp"
#include "Language/Table.hpp"
#include "Asset.hpp"
#include "LocalPath.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "UtilsSettings.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Hardware/Vibrator.hpp"

using namespace std::chrono;

enum ControlIndex {
  UIScale,
  CustomDPI,
  InputFile,
#ifdef HAVE_NLS
  LanguageFile,
#endif
  MenuTimeout,
  TextInput,
  HapticFeedback
};

class InterfaceConfigPanel final : public RowFormWidget {
public:
  InterfaceConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

#ifdef HAVE_BUILTIN_LANGUAGES

class LanguageFileVisitor: public File::Visitor
{
private:
  DataFieldEnum &df;

public:
  LanguageFileVisitor(DataFieldEnum &_df): df(_df) {}

  void Visit([[maybe_unused]] Path path, Path filename) override {
    if (!df.Exists(filename.c_str()))
      df.addEnumText(filename.c_str());
  }
};

#endif // HAVE_BUILTIN_LANGUAGES

void
InterfaceConfigPanel::Prepare(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  const UISettings &settings = CommonInterface::GetUISettings();

  RowFormWidget::Prepare(parent, rc);

  AddInteger(_("Text size"),
             nullptr,
             _T("%d %%"), _T("%d"), 75, 200, 5,
             settings.scale);

  WndProperty *wp_dpi = AddEnum(_("Display Resolution"),
                                _("The display resolution is used to adapt line widths, "
                                  "font size, landable size and more."));
  if (wp_dpi != nullptr) {
    static constexpr unsigned dpi_choices[] = {
      120, 160, 240, 260, 280, 300, 340, 360, 400, 420, 520,
    };
    const unsigned *dpi_choices_end =
      dpi_choices + sizeof(dpi_choices) / sizeof(dpi_choices[0]);

    DataFieldEnum &df = *(DataFieldEnum *)wp_dpi->GetDataField();
    df.AddChoice(0, _("Automatic"));
    for (const unsigned *dpi = dpi_choices; dpi != dpi_choices_end; ++dpi) {
      TCHAR buffer[20];
      _stprintf(buffer, _("%d dpi"), *dpi);
      df.AddChoice(*dpi, buffer);
    }
    df.SetValue(settings.custom_dpi);
    wp_dpi->RefreshDisplay();
  }
  SetExpertRow(CustomDPI);

  AddFile(_("Events"),
          _("The Input Events file defines the menu system and how XCSoar responds to "
            "button presses and events from external devices."),
          ProfileKeys::InputFile, _T("*.xci\0"));
  SetExpertRow(InputFile);

#ifdef HAVE_NLS
  WndProperty *wp;
  wp = AddEnum(_("Language"),
               _("The language options selects translations for English texts to other "
                   "languages. Select English for a native interface or Automatic to localise "
                   "XCSoar according to the system settings."));
  if (wp != nullptr) {
    DataFieldEnum &df = *(DataFieldEnum *)wp->GetDataField();
    df.addEnumText(_("Automatic"));
    df.addEnumText(_T("English"));

    for (const BuiltinLanguage *l = language_table;
         l->resource != nullptr; ++l) {
      StaticString<100> display_string;
      display_string.Format(_T("%s (%s)"), l->name, l->resource);
      df.addEnumText(l->resource, display_string);
    }

#ifdef HAVE_BUILTIN_LANGUAGES
    LanguageFileVisitor lfv(df);
    VisitDataFiles(_T("*.mo"), lfv);
#endif

    df.Sort(2);

    auto value_buffer = Profile::GetPath(ProfileKeys::LanguageFile);
    Path value = value_buffer;
    if (value == nullptr)
      value = Path(_T(""));

    if (value == Path(_T("none")))
      df.SetValue(1);
    else if (!value.empty() && value != Path(_T("auto"))) {
      const Path base = value.GetBase();
      if (base != nullptr)
        df.SetValue(base.c_str());
    }
    wp->RefreshDisplay();
  }
#endif // HAVE_NLS

  AddDuration(_("Menu timeout"),
              _("This determines how long menus will appear on screen if the user does not make any button "
                "presses or interacts with the computer."),
              seconds{1}, minutes{1}, seconds{1},
              settings.menu_timeout / 2);
  SetExpertRow(MenuTimeout);

  static constexpr StaticEnumChoice text_input_list[] = {
    { DialogSettings::TextInputStyle::Default, N_("Default") },
    { DialogSettings::TextInputStyle::Keyboard, N_("Keyboard") },
    { DialogSettings::TextInputStyle::HighScore,
      N_("HighScore Style") },
    nullptr
  };

  AddEnum(_("Text input style"),
          _("Determines how the user is prompted for text input (filename, teamcode etc.)"),
          text_input_list, (unsigned)settings.dialog.text_input_style);
  SetExpertRow(TextInput);

  /* on-screen keyboard doesn't work without a pointing device
     (mouse or touch screen) */
  SetRowVisible(TextInput, HasPointer());

#ifdef HAVE_VIBRATOR
  static constexpr StaticEnumChoice haptic_feedback_list[] = {
    { UISettings::HapticFeedback::DEFAULT, N_("OS settings") },
    { UISettings::HapticFeedback::OFF, N_("Off") },
    { UISettings::HapticFeedback::ON, N_("On") },
    nullptr
  };

  wp = AddEnum(_("Haptic feedback"),
               _("Determines if haptic feedback like vibration is used."),
               haptic_feedback_list, (unsigned)settings.haptic_feedback);
  SetExpertRow(HapticFeedback);
#endif /* HAVE_VIBRATOR */
}

bool
InterfaceConfigPanel::Save(bool &_changed) noexcept
{
  UISettings &settings = CommonInterface::SetUISettings();
  bool changed = false;

  if (SaveValueInteger(UIScale, ProfileKeys::UIScale,
                       settings.scale))
    require_restart = changed = true;

  if (SaveValueEnum(CustomDPI, ProfileKeys::CustomDPI,
                    settings.custom_dpi))
    require_restart = changed = true;

  if (SaveValueFileReader(InputFile, ProfileKeys::InputFile))
    require_restart = changed = true;

#ifdef HAVE_NLS
  WndProperty *wp = (WndProperty *)&GetControl(LanguageFile);
  if (wp != nullptr) {
    DataFieldEnum &df = *(DataFieldEnum *)wp->GetDataField();

    const auto old_value_buffer = Profile::GetPath(ProfileKeys::LanguageFile);
    Path old_value = old_value_buffer;
    if (old_value == nullptr)
      old_value = Path(_T(""));

    auto old_base = old_value.GetBase();
    if (old_base == nullptr)
      old_base = old_value;

    AllocatedPath buffer = nullptr;
    const TCHAR *new_value, *new_base;

    switch (df.GetValue()) {
    case 0:
      new_value = new_base = _T("auto");
      break;

    case 1:
      new_value = new_base = _T("none");
      break;

    default:
      new_value = df.GetAsString();
      buffer = ContractLocalPath(Path(new_value));
      if (buffer != nullptr)
        new_value = buffer.c_str();
      new_base = Path(new_value).GetBase().c_str();
      if (new_base == nullptr)
        new_base = new_value;
      break;
    }

    if (old_value != Path(new_value) &&
        old_base != Path(new_base)) {
      Profile::Set(ProfileKeys::LanguageFile, new_value);
      LanguageChanged = changed = true;
    }
  }
#endif // HAVE_NLS

  duration<unsigned> menu_timeout = GetValueTime(MenuTimeout) * 2;
  if (settings.menu_timeout != menu_timeout) {
    settings.menu_timeout = menu_timeout;
    Profile::Set(ProfileKeys::MenuTimeout, menu_timeout);
    changed = true;
  }

  if (HasPointer())
    changed |= SaveValueEnum(TextInput, ProfileKeys::AppTextInputStyle, settings.dialog.text_input_style);

#ifdef HAVE_VIBRATOR
  changed |= SaveValueEnum(HapticFeedback, ProfileKeys::HapticFeedback, settings.haptic_feedback);
#endif

  _changed |= changed;
  return true;
}

std::unique_ptr<Widget>
CreateInterfaceConfigPanel()
{
  return std::make_unique<InterfaceConfigPanel>();
}
