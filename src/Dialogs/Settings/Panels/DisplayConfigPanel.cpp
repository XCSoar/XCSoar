// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DisplayConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "ui/canvas/Features.hpp" // for DRAW_MOUSE_CURSOR
#include "ui/window/Features.hpp" // for HAVE_FULL_SCREEN_SETTING
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Form/DataField/Enum.hpp"
#include "Hardware/DisplayBrightness.hpp"
#include "Hardware/RotateDisplay.hpp"
#include "Interface.hpp"
#include "DisplaySettings.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/HelpDialog.hpp"
#include "Dialogs/Settings/InfoBoxAreaStretchWidget.hpp"
#include "Widget/StaticHelpTextWidget.hpp"
#include "MainWindow.hpp"
#include "LogFile.hpp"
#include "Language/Language.hpp"
#include "UtilsSettings.hpp"
#include "UIGlobals.hpp"
#include "Asset.hpp"
#include "util/Macros.hpp"
#include "util/StaticString.hxx"

#ifdef USE_POLL_EVENT
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"
#endif

#include <memory>
#include <string>

#if defined(KOBO) || (defined(__linux__) && !defined(ANDROID))
#define HAVE_DISPLAY_BRIGHTNESS_CONTROL
#endif

#ifdef HAVE_FULL_SCREEN_SETTING
/**
 * The screen edges the InfoBox area can be stretched out to.
 */
static constexpr struct {
  const char *label;
  DisplaySettings::InfoBoxAreaStretchEdge bit;
} infobox_area_stretch_edges[] = {
  { NC_("Screen edge", "Top"),
    DisplaySettings::INFOBOX_AREA_STRETCH_TOP },
  { NC_("Screen edge", "Right"),
    DisplaySettings::INFOBOX_AREA_STRETCH_RIGHT },
  { NC_("Screen edge", "Bottom"),
    DisplaySettings::INFOBOX_AREA_STRETCH_BOTTOM },
  { NC_("Screen edge", "Left"),
    DisplaySettings::INFOBOX_AREA_STRETCH_LEFT },
};

/**
 * A one-line hint for the picker dialog; #TwoWidgets would cut a
 * longer paragraph off in landscape.  The full explanation is the
 * help text of the settings row.
 */
static constexpr const char *infobox_area_stretch_hint =
  N_("Tap an edge to stretch the InfoBox area out to the screen border "
     "there.");

static constexpr const char *infobox_area_stretch_help =
  N_("The InfoBox area is the part of the screen available to the "
     "InfoBoxes, gauges, map overlays and the menu. Pick the edges on "
     "which it is stretched out to the screen border. Unstretched "
     "edges stay clear of the system bars and rounded corners. The "
     "map always uses the whole screen, "
     "and dialogs always stay inside the safe area. Only relevant "
     "while full screen mode is enabled.");

/**
 * Describe the enabled edges for the settings list, e.g. "Top, Bottom".
 */
static std::string
FormatInfoBoxAreaStretch(uint8_t edges) noexcept
{
  if (edges == DisplaySettings::INFOBOX_AREA_STRETCH_NONE)
    return _("None");

  std::string result;

  for (const auto &i : infobox_area_stretch_edges) {
    if ((edges & i.bit) == 0)
      continue;

    if (!result.empty())
      result += ", ";
    result += gettext_context("Screen edge", i.label);
  }

  return result;
}
#endif

#ifdef HAVE_STATUS_BAR_SETTING
static constexpr StaticEnumChoice status_bar_list[] = {
  { DisplaySettings::StatusBar::AUTO, NC_("Setting", "Auto"),
    N_("Show the status bar unless the InfoBox area is stretched to the "
       "top screen edge, where it would cover the InfoBoxes.") },
  { DisplaySettings::StatusBar::VISIBLE, N_("Visible"),
    N_("Always show the status bar, even in full screen mode. The "
       "InfoBoxes cannot be drawn behind it, so they keep clear of the "
       "top screen edge.") },
  { DisplaySettings::StatusBar::HIDDEN, N_("Hidden"),
    N_("Never show the status bar.") },
  nullptr
};
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
#ifdef HAVE_FULL_SCREEN_SETTING
  bool full_screen;
  uint8_t infobox_area_stretch;
#endif
#ifdef HAVE_STATUS_BAR_SETTING
  DisplaySettings::StatusBar status_bar;
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

#ifdef HAVE_FULL_SCREEN_SETTING
  void EditInfoBoxAreaStretch() noexcept;
#endif

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

#ifdef HAVE_FULL_SCREEN_SETTING

void
DisplayConfigPanel::EditInfoBoxAreaStretch() noexcept
{
  auto picker =
    std::make_unique<InfoBoxAreaStretchWidget>(UIGlobals::GetDialogLook(),
                                               infobox_area_stretch);
  auto &picker_ref = *picker;

  TWidgetDialog<StaticHelpTextWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(), _("Stretch InfoBox area"));
  dialog.SetWidget(std::move(picker), gettext(infobox_area_stretch_hint));
  dialog.AddButton(_("OK"), mrOK);

  /* the row's help text is not reachable from here */
  dialog.AddButton(_("Help"), [](){
    HelpDialog(_("Stretch InfoBox area"),
               gettext(infobox_area_stretch_help));
  });

  dialog.AddButton(_("Cancel"), mrCancel);

  if (dialog.ShowModal() != mrOK)
    return;

  const uint8_t edges = picker_ref.GetEdges();
  if (edges == infobox_area_stretch)
    return;

  infobox_area_stretch = edges;

  auto &settings = CommonInterface::SetUISettings().display;
  settings.infobox_area_stretch = edges;
  Profile::Set(ProfileKeys::InfoBoxAreaStretch, unsigned(edges));
  Profile::Save();

  /* in "Auto" mode the status bar follows the top edge, and the area
     the InfoBoxes and gauges may use changed */
  CommonInterface::main_window->ApplyFullScreenSettings();
  CommonInterface::main_window->ReinitialiseLayout();
  Refresh();
}

#endif

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

#ifdef HAVE_FULL_SCREEN_SETTING
  full_screen = ui_settings.display.full_screen;
  infobox_area_stretch = ui_settings.display.infobox_area_stretch;
#endif

#ifdef HAVE_STATUS_BAR_SETTING
  status_bar = ui_settings.display.status_bar;
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

#ifdef HAVE_FULL_SCREEN_SETTING
  AddToggleItem(_("Full screen"),
                _("Let XCSoar use the whole screen, including the areas behind "
                  "the system bars and the display cutout. The map fills it "
                  "completely, while \"Stretch InfoBox area\" decides which "
                  "edges the InfoBoxes, gauges, map overlays and the menu "
                  "may reach."),
                full_screen);

  if (IsExpert() && full_screen) {
    const std::string edges =
      FormatInfoBoxAreaStretch(infobox_area_stretch);
    AddItem(_("Stretch InfoBox area"), [this](){
      EditInfoBoxAreaStretch();
    }, {.value = edges.c_str(),
        .chevron = true,
        .help = gettext(infobox_area_stretch_help)});
  }
#endif

#ifdef HAVE_STATUS_BAR_SETTING
  AddEnumItem(_("System status bar"),
              _("Whether the system status bar with the clock and the battery "
                "level stays visible."),
              status_bar_list, status_bar);
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
    /* Reload 1-bit vs anti-aliased glyphs (Kobo mono path). */
    CommonInterface::main_window->Initialise();
    CommonInterface::main_window->ReinitialiseLook();
  }

  if (Profile::Update(ProfileKeys::CustomDPI,
                      ui_settings.custom_dpi, custom_dpi)) {
    changed = true;
    if (CommonInterface::main_window != nullptr) {
      CommonInterface::main_window->Initialise();
      CommonInterface::main_window->ReinitialiseLook();
      CommonInterface::main_window->ReinitialiseLayout();
    }
  }

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

#ifdef HAVE_FULL_SCREEN_SETTING
  bool full_screen_changed =
    Profile::Update(ProfileKeys::FullScreen,
                    ui_settings.display.full_screen, full_screen);
  /* the per-edge settings are applied by their own dialog */
  changed |= full_screen_changed;

#ifdef HAVE_STATUS_BAR_SETTING
  if (Profile::Update(ProfileKeys::StatusBar,
                      ui_settings.display.status_bar, status_bar)) {
    changed = true;
    full_screen_changed = true;
  }
#endif

  /* this may change the usable screen area, so do it before the
     remaining settings are applied */
  if (full_screen_changed)
    CommonInterface::main_window->ApplyFullScreenSettings();
#endif

  changed |= Profile::Update(ProfileKeys::DarkMode,
                             ui_settings.dark_mode, dark_mode);

  if (Profile::Update(ProfileKeys::UIScale,
                      ui_settings.scale, unsigned(scale))) {
    changed = true;
    if (CommonInterface::main_window != nullptr) {
      CommonInterface::main_window->Initialise();
      CommonInterface::main_window->ReinitialiseLook();
      CommonInterface::main_window->ReinitialiseLayout();
    }
  }

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
