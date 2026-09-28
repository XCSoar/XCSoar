// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DisplayConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "ui/canvas/Features.hpp" // for DRAW_MOUSE_CURSOR
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Form/DataField/Enum.hpp"
#include "Hardware/DisplayBrightness.hpp"
#include "Hardware/RotateDisplay.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "LogFile.hpp"
#include "Language/Language.hpp"
#include "UtilsSettings.hpp"
#include "Asset.hpp"
#include "util/Macros.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#endif

#ifdef USE_POLL_EVENT
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"
#endif

#include <memory>

#if defined(KOBO) || (defined(__linux__) && !defined(ANDROID))
#define HAVE_DISPLAY_BRIGHTNESS_CONTROL
#endif

static constexpr StaticEnumChoice display_type_list[] = {
  { DisplayType::LCD, NC_("Setting", "LCD"),
    N_("Conventional LCD or OLED. Full scrolling animations.") },
  { DisplayType::E_INK, NC_("Setting", "E-ink"),
    N_("Monochrome electronic paper. Disables kinetic and smooth "
       "scrolling.") },
  { DisplayType::COLOR_E_INK, NC_("Setting", "Color e-ink"),
    N_("Color electronic paper. Disables kinetic and smooth "
       "scrolling like monochrome e-ink.") },
  nullptr
};

static_assert(ARRAY_SIZE(display_type_list) ==
              unsigned(DisplayType::COUNT) + 1,
              "display_type_list must match DisplayType::COUNT");

static constexpr StaticEnumChoice display_orientation_list[] = {
  { DisplayOrientation::DEFAULT,
    N_("Default") },
  { DisplayOrientation::PORTRAIT,
    N_("Portrait") },
  { DisplayOrientation::LANDSCAPE,
    N_("Landscape") },
  { DisplayOrientation::REVERSE_PORTRAIT,
    N_("Reverse Portrait") },
  { DisplayOrientation::REVERSE_LANDSCAPE,
    N_("Reverse Landscape") },
  nullptr
};

static constexpr StaticEnumChoice dark_mode_list[] = {
  { UISettings::DarkMode::AUTO, N_("Auto"),
    N_("Use the system-wide setting") },
  { UISettings::DarkMode::OFF, N_("Off"),
    N_("Black text on white background") },
  { UISettings::DarkMode::ON, N_("On"),
    N_("White text on black background") },
  nullptr
};

/** the resolutions the user may choose instead of the detected one */
static constexpr unsigned dpi_choices[] = {
  120, 160, 240, 260, 280, 300, 340, 360, 400, 420, 520,
};

/** The caption of a resolution; 0 stands for the detected one. */
static void
FormatDpi(StaticString<32> &buffer, unsigned dpi) noexcept
{
  if (dpi == 0)
    buffer = _("Automatic");
  else
    buffer.Format(_("%u dpi"), dpi);
}

/**
 * Let the user pick the resolution from #dpi_choices or the detected
 * one.
 *
 * @return true if the value has changed
 */
static bool
PickDpi(const char *caption, const char *help, unsigned &value) noexcept
{
  constexpr unsigned n = std::size(dpi_choices) + 1;

  StaticString<32> captions[n];
  PickerChoice choices[n];
  int current = 0;

  FormatDpi(captions[0], 0);
  choices[0] = {captions[0].c_str()};

  for (unsigned i = 0; i < std::size(dpi_choices); ++i) {
    FormatDpi(captions[i + 1], dpi_choices[i]);
    choices[i + 1] = {captions[i + 1].c_str()};

    if (dpi_choices[i] == value)
      current = i + 1;
  }

  const int picked = PickChoice(caption, help, choices, current);
  if (picked < 0 || picked == current)
    return false;

  value = picked == 0 ? 0 : dpi_choices[picked - 1];
  return true;
}

/**
 * The screen: its technology, its resolution and orientation, its
 * brightness and the size of the text on it.
 */
class DisplayConfigPanel final : public ConfigListPanel {
  std::unique_ptr<DisplayBrightness> brightness;

  DisplayType display_type;
  unsigned custom_dpi;
  DisplayOrientation orientation;
#ifdef HAVE_DISPLAY_BRIGHTNESS_CONTROL
  int brightness_percent;
#endif
#ifdef ANDROID
  bool full_screen;
#endif
  UISettings::DarkMode dark_mode;
  int scale;
#ifdef DRAW_MOUSE_CURSOR
  int cursor_size;
  bool invert_cursor_colors;
#endif

public:
  DisplayConfigPanel() noexcept
    :brightness(DisplayBrightness::Detect()) {}

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
DisplayConfigPanel::LoadSettings() noexcept
{
  const UISettings &ui_settings = CommonInterface::GetUISettings();

  display_type = ui_settings.display.display_type;
  custom_dpi = ui_settings.custom_dpi;
  orientation = ui_settings.display.orientation;

#ifdef HAVE_DISPLAY_BRIGHTNESS_CONTROL
  brightness_percent = brightness != nullptr
    ? brightness->GetBrightnessPercent()
    : 0;
#endif

#ifdef ANDROID
  full_screen = ui_settings.display.full_screen;
#endif

  dark_mode = ui_settings.dark_mode;
  scale = ui_settings.scale;

#ifdef DRAW_MOUSE_CURSOR
  cursor_size = ui_settings.display.cursor_size;
  invert_cursor_colors = ui_settings.display.invert_cursor_colors;
#endif
}

void
DisplayConfigPanel::Fill() noexcept
{
  /* the hardware */
  AddGroup();

  if (IsExpert()) {
    AddEnumItem(C_("Setting", "Display type"),
                _("Select the display technology. E-ink modes disable kinetic "
                  "and smooth scrolling for slow refresh screens."),
                display_type_list, display_type);

    const char *dpi_caption = _("Display resolution");
    const char *dpi_help =
      _("The display resolution is used to adapt line widths, "
        "font size, landable size and more.");

    StaticString<32> dpi;
    FormatDpi(dpi, custom_dpi);

    AddItem(dpi_caption, [this, dpi_caption, dpi_help](){
      if (PickDpi(dpi_caption, dpi_help, custom_dpi))
        Refresh();
    }, {.value = dpi.c_str(), .chevron = true});
  }

  if (Display::RotateSupported())
    AddEnumItem(_("Display orientation"),
                _("Rotate the display on devices that support it."),
                display_orientation_list, orientation);

#ifdef HAVE_DISPLAY_BRIGHTNESS_CONTROL
  if (brightness != nullptr) {
    const char *caption = _("Screen brightness");

    if (brightness->IsWritable())
      AddPercentItem(caption, _("Adjust the screen brightness."),
                     0, 100, 5, brightness_percent);
    else {
      StaticString<8> percent;
      percent.Format("%d %%", brightness_percent);

      AddItem(caption, {
        .value = percent.c_str(),
        .help = _("Screen brightness is read-only because writing requires additional permissions."),
        .disabled = true,
        .selectable_when_disabled = true,
      });
    }
  }
#endif

#ifdef ANDROID
  AddToggleItem(_("Full screen"), _("Run XCSoar in full screen mode"),
                full_screen);
#endif

  /* the look of everything on it */
  AddGroup();

  AddEnumItem(_("Dark mode"), nullptr, dark_mode_list, dark_mode);

  AddPercentItem(_("Text size"), nullptr,
                 UISettings::SCALE_MIN, UISettings::SCALE_MAX,
                 UISettings::SCALE_STEP, scale);

#ifdef DRAW_MOUSE_CURSOR
  AddGroup();

  StaticString<8> zoom;
  zoom.Format("%d x", cursor_size);

  AddItem(_("Cursor zoom"), [this](){
    if (PickNumber(_("Cursor zoom"), _("Cursor zoom factor"),
                   1, 10, 1, cursor_size,
                   [](StaticString<32> &s, int v){ s.Format("%d x", v); }))
      Refresh();
  }, {.value = zoom.c_str(), .chevron = true});

  AddToggleItem(_("Invert cursor color"), _("Enable black cursor"),
                invert_cursor_colors);
#endif
}

bool
DisplayConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  UISettings &ui_settings = CommonInterface::SetUISettings();

  if (Profile::Update(ProfileKeys::DisplayType,
                      ui_settings.display.display_type, display_type)) {
    changed = true;
    SetDisplayType(ui_settings.display.display_type);
  }

  if (Profile::Update(ProfileKeys::CustomDPI,
                      ui_settings.custom_dpi, custom_dpi))
    require_restart = changed = true;

  if (Display::RotateSupported() &&
      Profile::Update(ProfileKeys::MapOrientation,
                      ui_settings.display.orientation, orientation)) {
    changed = true;

    if (!Display::Rotate(ui_settings.display.orientation))
      LogString("Display rotation failed");

#ifdef SOFTWARE_ROTATE_DISPLAY
    CommonInterface::main_window->SetDisplayOrientation(
        ui_settings.display.orientation);
#endif

#ifdef USE_POLL_EVENT
    UI::event_queue->SetDisplayOrientation(ui_settings.display.orientation);
#endif

    CommonInterface::main_window->CheckResize();
  }

#ifdef HAVE_DISPLAY_BRIGHTNESS_CONTROL
  if (brightness != nullptr && brightness->IsWritable() &&
      unsigned(brightness_percent) != brightness->GetBrightnessPercent())
    brightness->SetBrightnessPercent(brightness_percent);
#endif

#ifdef ANDROID
  changed |= Profile::Update(ProfileKeys::FullScreen,
                             ui_settings.display.full_screen, full_screen);
  native_view->SetFullScreen(Java::GetEnv(), ui_settings.display.full_screen);
#endif

  changed |= Profile::Update(ProfileKeys::DarkMode,
                             ui_settings.dark_mode, dark_mode);

  if (Profile::Update(ProfileKeys::UIScale,
                      ui_settings.scale, unsigned(scale)))
    require_restart = changed = true;

#ifdef DRAW_MOUSE_CURSOR
  changed |= Profile::Update(ProfileKeys::CursorSize,
                             ui_settings.display.cursor_size,
                             uint8_t(cursor_size));
  CommonInterface::main_window->SetCursorSize(ui_settings.display.cursor_size);

  changed |= Profile::Update(ProfileKeys::CursorColorsInverted,
                             ui_settings.display.invert_cursor_colors,
                             invert_cursor_colors);
  CommonInterface::main_window->SetCursorColorsInverted(
    ui_settings.display.invert_cursor_colors);
#endif

  _changed |= changed;
  return true;
}

std::unique_ptr<Widget>
CreateDisplayConfigPanel()
{
  return std::make_unique<DisplayConfigPanel>();
}
