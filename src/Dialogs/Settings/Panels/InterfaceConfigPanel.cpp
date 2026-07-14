// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InterfaceConfigPanel.hpp"
#include "Profile/Profile.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Dialogs/Dialogs.h"
#include "util/StringCompare.hxx"
#include "util/StaticString.hxx"
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
#include "Repository/FileType.hpp"
#include "Version.hpp"

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
#ifdef HAVE_VIBRATOR
  HapticFeedback,
#endif
  ShowQuickGuideOnStartup,
  ShowReleaseNotesOnStartup,
  DisclaimerAccepted,
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
             "%d %%", "%d", 75, 200, 5,
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
      StaticString<20> buffer;
      buffer.Format(_("%u dpi"), *dpi);
      df.AddChoice(*dpi, buffer);
    }
    df.SetValue(settings.custom_dpi);
    wp_dpi->RefreshDisplay();
  }
  SetExpertRow(CustomDPI);

  AddFile(_("Events"),
          _("The Input Events file defines the menu system and how XCSoar responds to "
            "button presses and events from external devices."),
          ProfileKeys::InputFile,
          GetFileTypePatterns(FileType::XCI),
          FileType::XCI);
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
    df.addEnumText("English");

    for (const BuiltinLanguage *l = language_table;
         l->resource != nullptr; ++l) {
      StaticString<100> display_string;
      display_string.Format("%s (%s)", l->name, l->resource);
      df.addEnumText(l->resource, display_string);
    }

#ifdef HAVE_BUILTIN_LANGUAGES
    LanguageFileVisitor lfv(df);
    VisitDataFiles("*.mo", lfv);
#endif

    df.Sort(2);

    auto value_buffer = Profile::GetPath(ProfileKeys::LanguageFile);
    Path value = value_buffer;
    if (value == nullptr)
      value = Path("");

    if (value == Path("none"))
      df.SetValue(1);
    else if (!value.empty() && value != Path("auto")) {
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

  bool hide_quick_guide = false;
  Profile::Get(ProfileKeys::HideQuickGuideDialogOnStartup,
               hide_quick_guide);
  AddBoolean(_("Show Quick Guide"),
             _("If enabled, the Quick Guide is shown when XCSoar starts."),
             !hide_quick_guide);

  const char *last_seen_news =
    Profile::Get(ProfileKeys::LastSeenNewsVersion);
  const bool news_seen = last_seen_news != nullptr &&
    StringIsEqual(last_seen_news, XCSoar_Version);
  AddBoolean(_("Show release notes"),
             _("If enabled, the What's New page is shown on the next "
               "startup."),
             !news_seen);

  const char *disclaimer_acknowledged_version =
    Profile::Get(ProfileKeys::DisclaimerAcknowledgedVersion);
  const bool disclaimer_acknowledged =
    disclaimer_acknowledged_version != nullptr &&
    StringIsEqual(disclaimer_acknowledged_version, XCSoar_Version);

  static constexpr StaticEnumChoice disclaimer_accepted_list[] = {
    { 0, N_("No") },
    { 1, N_("Yes") },
    nullptr
  };

  AddEnum(_("Safety disclaimer accepted"),
          _("Whether the safety disclaimer has been accepted for this "
            "version."),
          disclaimer_accepted_list,
          disclaimer_acknowledged ? 1u : 0u);
  SetExpertRow(DisclaimerAccepted);
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
      old_value = Path("");

    auto old_base = old_value.GetBase();
    if (old_base == nullptr)
      old_base = old_value;

    AllocatedPath buffer = nullptr;
    const char *new_value, *new_base;

    switch (df.GetValue()) {
    case 0:
      new_value = new_base = "auto";
      break;

    case 1:
      new_value = new_base = "none";
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

  bool hide_quick_guide = false;
  Profile::Get(ProfileKeys::HideQuickGuideDialogOnStartup, hide_quick_guide);
  if (SaveValue(ShowQuickGuideOnStartup,
                ProfileKeys::HideQuickGuideDialogOnStartup,
                hide_quick_guide, true))
    changed = true;

  const bool show_release_notes = GetValueBoolean(ShowReleaseNotesOnStartup);
  const char *last_seen_news =
    Profile::Get(ProfileKeys::LastSeenNewsVersion);
  const bool news_seen = last_seen_news != nullptr &&
    StringIsEqual(last_seen_news, XCSoar_Version);
  if (show_release_notes != !news_seen) {
    if (show_release_notes)
      Profile::Set(ProfileKeys::LastSeenNewsVersion, "");
    else
      Profile::Set(ProfileKeys::LastSeenNewsVersion, XCSoar_Version);
    changed = true;
  }

  const bool disclaimer_accepted =
    GetValueEnum(DisclaimerAccepted) != 0;
  const char *disclaimer_acknowledged_version =
    Profile::Get(ProfileKeys::DisclaimerAcknowledgedVersion);
  const bool disclaimer_acknowledged =
    disclaimer_acknowledged_version != nullptr &&
    StringIsEqual(disclaimer_acknowledged_version, XCSoar_Version);
  if (disclaimer_accepted != disclaimer_acknowledged) {
    if (disclaimer_accepted)
      Profile::Set(ProfileKeys::DisclaimerAcknowledgedVersion,
                   XCSoar_Version);
    else
      Profile::Set(ProfileKeys::DisclaimerAcknowledgedVersion, "");
    changed = true;
  }

  _changed |= changed;
  return true;
}

std::unique_ptr<Widget>
CreateInterfaceConfigPanel()
{
  return std::make_unique<InterfaceConfigPanel>();
}
