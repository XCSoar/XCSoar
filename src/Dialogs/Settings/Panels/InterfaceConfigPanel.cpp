// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InterfaceConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Profile/Profile.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/File.hpp"
#include "util/StringCompare.hxx"
#include "Interface.hpp"
#include "Language/Table.hpp"
#include "Asset.hpp"
#include "LocalPath.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "UtilsSettings.hpp"
#include "Language/Language.hpp"
#include "Hardware/Vibrator.hpp"
#include "Repository/FileType.hpp"
#include "Version.hpp"

#include <string>
#include <vector>

using namespace std::chrono;

/**
 * How XCSoar talks to the user: the language, the menus and the
 * views it shows at startup.
 */
class InterfaceConfigPanel final : public ConfigListPanel {
  FileDataField events_file;

#ifdef HAVE_NLS
  /** One language the user may choose. */
  struct Language {
    /** what the profile stores: "auto", "none" or the file name */
    std::string value;

    std::string display;
  };

  std::vector<Language> languages;

  /** the index of the chosen language in #languages */
  unsigned language;
#endif

  /** the time until a menu closes, in seconds */
  duration<unsigned> menu_timeout;

  DialogSettings::TextInputStyle text_input_style;

#ifdef HAVE_VIBRATOR
  UISettings::HapticFeedback haptic_feedback;
#endif

  bool show_quick_guide, show_release_notes, disclaimer_accepted;

private:
#ifdef HAVE_NLS
  void LoadLanguages() noexcept;
  bool SaveLanguage() noexcept;
#endif

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

/** Is the What's New page shown on the next startup? */
static bool
IsNewsSeen() noexcept
{
  const char *last_seen_news =
    Profile::Get(ProfileKeys::LastSeenNewsVersion);
  return last_seen_news != nullptr &&
    StringIsEqual(last_seen_news, XCSoar_Version);
}

/** Has the safety disclaimer been accepted for this version? */
static bool
IsDisclaimerAcknowledged() noexcept
{
  const char *version =
    Profile::Get(ProfileKeys::DisclaimerAcknowledgedVersion);
  return version != nullptr && StringIsEqual(version, XCSoar_Version);
}

#ifdef HAVE_NLS

void
InterfaceConfigPanel::LoadLanguages() noexcept
{
  languages.clear();
  languages.push_back({"auto", _("Automatic")});
  languages.push_back({"none", "English"});

  for (const BuiltinLanguage *l = language_table;
       l->resource != nullptr; ++l) {
    StaticString<100> display;
    display.Format("%s (%s)", l->name, l->resource);
    languages.push_back({l->resource, display.c_str()});
  }

#ifdef HAVE_BUILTIN_LANGUAGES
  /* the translations the user has put into the data directory */
  class LanguageFileVisitor final : public File::Visitor {
    std::vector<Language> &languages;

  public:
    explicit LanguageFileVisitor(std::vector<Language> &_languages) noexcept
      :languages(_languages) {}

    void Visit([[maybe_unused]] Path path, Path filename) override {
      for (const auto &l : languages)
        if (l.value == filename.c_str())
          return;

      languages.push_back({filename.c_str(), filename.c_str()});
    }
  };

  LanguageFileVisitor visitor(languages);
  VisitDataFiles("*.mo", visitor);
#endif

  /* the languages by name, after the two fixed choices */
  std::sort(languages.begin() + 2, languages.end(),
            [](const Language &a, const Language &b){
              return a.display < b.display;
            });

  language = 0;

  const auto value = Profile::GetPath(ProfileKeys::LanguageFile);
  if (value == nullptr || value.empty() || value == Path("auto"))
    return;

  if (value == Path("none")) {
    language = 1;
    return;
  }

  const Path base = value.GetBase();
  if (base == nullptr)
    return;

  for (unsigned i = 2; i < languages.size(); ++i)
    if (languages[i].value == base.c_str())
      language = i;
}

bool
InterfaceConfigPanel::SaveLanguage() noexcept
{
  /* Use AllocatedPath here: Path::empty() null-dereferences, while
     AllocatedPath::empty() is safe. Missing / empty LanguageFile means
     automatic - same as ReadLanguageFile(); do not persist "auto" just
     because the key was absent (#1793). */
  const auto old_value = Profile::GetPath(ProfileKeys::LanguageFile);
  const bool old_is_auto =
    old_value == nullptr || old_value.empty() || old_value == Path("auto");

  Path old_base = old_is_auto ? Path("auto") : Path(old_value).GetBase();
  if (old_base == nullptr)
    old_base = old_value;

  const char *new_value = languages[language].value.c_str();
  if (old_base == Path(new_value))
    return false;

  Profile::Set(ProfileKeys::LanguageFile, new_value);
  LanguageChanged = true;
  return true;
}

#endif // HAVE_NLS

void
InterfaceConfigPanel::LoadSettings() noexcept
{
  const UISettings &settings = CommonInterface::GetUISettings();

  events_file.SetFileType(FileType::XCI);
  events_file.AddNull();
  events_file.ScanMultiplePatterns(GetFileTypePatterns(FileType::XCI));
  events_file.Sort(FileDataField::SortOrder::ASCENDING, true);

  if (const auto path = Profile::GetPath(ProfileKeys::InputFile);
      path != nullptr)
    events_file.SetValue(path);

#ifdef HAVE_NLS
  LoadLanguages();
#endif

  menu_timeout = settings.menu_timeout / 2;
  text_input_style = settings.dialog.text_input_style;

#ifdef HAVE_VIBRATOR
  haptic_feedback = settings.haptic_feedback;
#endif

  bool hide_quick_guide = false;
  Profile::Get(ProfileKeys::HideQuickGuideDialogOnStartup,
               hide_quick_guide);
  show_quick_guide = !hide_quick_guide;

  show_release_notes = !IsNewsSeen();
  disclaimer_accepted = IsDisclaimerAcknowledged();
}

void
InterfaceConfigPanel::Fill() noexcept
{
  AddGroup();

#ifdef HAVE_NLS
  AddItem(_("Language"), [this](){
    std::vector<PickerChoice> choices;
    for (const auto &l : languages)
      choices.push_back({l.display.c_str()});

    const int picked =
      PickChoice(_("Language"),
                 _("The language options selects translations for English texts to other "
                   "languages. Select English for a native interface or Automatic to localise "
                   "XCSoar according to the system settings."),
                 choices, language);
    if (picked >= 0 && unsigned(picked) != language) {
      language = picked;
      Refresh();
    }
  }, {.value = languages[language].display.c_str(), .chevron = true});
#endif

  if (IsExpert()) {
    const char *caption = _("Events");
    const char *help =
      _("The Input Events file defines the menu system and how XCSoar responds to "
        "button presses and events from external devices.");

    /* the file as a small value, with every line of it */
    ItemOptions options{.value_size = TextSize::SMALL,
                        .value_all_lines = true,
                        .chevron = true};

    const char *name = events_file.GetAsDisplayString();
    if (*name != '\0')
      options.value = name;
    else
      options.badge = C_("Badge", "none");

    AddItem(caption, [this, caption, help](){
      PickFile(caption, help, events_file);
      Refresh();
    }, options);

    AddDurationItem(_("Menu timeout"),
                    _("This determines how long menus will appear on screen if the user does not make any button "
                      "presses or interacts with the computer."),
                    1, 60, 1, menu_timeout);

    /* on-screen keyboard doesn't work without a pointing device
       (mouse or touch screen) */
    if (HasPointer()) {
      static constexpr StaticEnumChoice text_input_list[] = {
        { DialogSettings::TextInputStyle::Default, N_("Default") },
        { DialogSettings::TextInputStyle::Keyboard, N_("Keyboard") },
        { DialogSettings::TextInputStyle::HighScore,
          N_("HighScore Style") },
        nullptr
      };

      AddEnumItem(_("Text input style"),
                  _("Determines how the user is prompted for text input (filename, teamcode etc.)"),
                  text_input_list, text_input_style);
    }

#ifdef HAVE_VIBRATOR
    static constexpr StaticEnumChoice haptic_feedback_list[] = {
      { UISettings::HapticFeedback::DEFAULT, N_("OS settings") },
      { UISettings::HapticFeedback::OFF, N_("Off") },
      { UISettings::HapticFeedback::ON, N_("On") },
      nullptr
    };

    AddEnumItem(_("Haptic feedback"),
                _("Determines if haptic feedback like vibration is used."),
                haptic_feedback_list, haptic_feedback);
#endif
  }

  /* what XCSoar shows when it starts */
  AddGroup();

  AddToggleItem(C_("Setting", "Show Quick Guide"),
                _("If enabled, the Quick Guide is shown when XCSoar starts."),
                show_quick_guide);

  AddToggleItem(C_("Setting", "Show release notes"),
                _("If enabled, the What's New page is shown on the next "
                  "startup."),
                show_release_notes);

  if (IsExpert())
    AddToggleItem(_("Safety disclaimer accepted"),
                  _("Whether the safety disclaimer has been accepted for this "
                    "version."),
                  disclaimer_accepted);
}

bool
InterfaceConfigPanel::Save(bool &_changed) noexcept
{
  UISettings &settings = CommonInterface::SetUISettings();
  bool changed = false;

  if (Profile::SetPath(ProfileKeys::InputFile, events_file.GetValue()))
    require_restart = changed = true;

#ifdef HAVE_NLS
  changed |= SaveLanguage();
#endif

  changed |= Profile::Update(ProfileKeys::MenuTimeout, settings.menu_timeout,
                             duration<unsigned>{menu_timeout.count() * 2});

  if (HasPointer())
    changed |= Profile::Update(ProfileKeys::AppTextInputStyle,
                               settings.dialog.text_input_style,
                               text_input_style);

#ifdef HAVE_VIBRATOR
  changed |= Profile::Update(ProfileKeys::HapticFeedback,
                             settings.haptic_feedback, haptic_feedback);
#endif

  bool hide_quick_guide = false;
  Profile::Get(ProfileKeys::HideQuickGuideDialogOnStartup, hide_quick_guide);
  if (hide_quick_guide != !show_quick_guide) {
    Profile::Set(ProfileKeys::HideQuickGuideDialogOnStartup,
                 !show_quick_guide);
    changed = true;
  }

  if (show_release_notes != !IsNewsSeen()) {
    Profile::Set(ProfileKeys::LastSeenNewsVersion,
                 show_release_notes ? "" : XCSoar_Version);
    changed = true;
  }

  if (disclaimer_accepted != IsDisclaimerAcknowledged()) {
    Profile::Set(ProfileKeys::DisclaimerAcknowledgedVersion,
                 disclaimer_accepted ? XCSoar_Version : "");
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
