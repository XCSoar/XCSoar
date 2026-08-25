// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DisplayConfigPanel.hpp"
#include "ui/canvas/Features.hpp" // for DRAW_MOUSE_CURSOR
#include "ui/window/Features.hpp" // for HAVE_FULL_SCREEN_SETTING
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/String.hpp"
#include "Form/Edit.hpp"
#include "Hardware/DisplayBrightness.hpp"
#include "Hardware/RotateDisplay.hpp"
#include "Interface.hpp"
#include "DisplaySettings.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/HelpDialog.hpp"
#include "Dialogs/Settings/SafeAreaStretchWidget.hpp"
#include "Widget/StaticHelpTextWidget.hpp"
#include "MainWindow.hpp"
#include "LogFile.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "UtilsSettings.hpp"
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

enum ControlIndex {
  AppDisplayType,
  CustomDPI,
  Orientation,
#if defined(HAVE_DISPLAY_BRIGHTNESS_CONTROL)
  ScreenBrightness,
#endif
#ifdef HAVE_FULL_SCREEN_SETTING
  FullScreen,
  SafeAreaStretch,
#endif
#ifdef HAVE_STATUS_BAR_SETTING
  StatusBar,
#endif
  DarkMode,
  UIScale,
#ifdef DRAW_MOUSE_CURSOR
  CursorSize,
  CursorInverted,
#endif
};

#ifdef HAVE_FULL_SCREEN_SETTING
/**
 * The screen edges the safe area can be stretched out to.
 */
static constexpr struct {
  const char *label;
  DisplaySettings::SafeAreaStretchEdge bit;
} safe_area_stretch_edges[] = {
  { NC_("Screen edge", "Top"), DisplaySettings::SAFE_AREA_STRETCH_TOP },
  { NC_("Screen edge", "Right"), DisplaySettings::SAFE_AREA_STRETCH_RIGHT },
  { NC_("Screen edge", "Bottom"), DisplaySettings::SAFE_AREA_STRETCH_BOTTOM },
  { NC_("Screen edge", "Left"), DisplaySettings::SAFE_AREA_STRETCH_LEFT },
};

/**
 * A one-line hint for the picker dialog; #TwoWidgets would cut a
 * longer paragraph off in landscape.  The full explanation is the
 * help text of the settings row.
 */
static constexpr const char *safe_area_stretch_hint =
  N_("Tap an edge to stretch the safe area out to the screen border "
     "there.");

static constexpr const char *safe_area_stretch_help =
  N_("The safe area is the part of the screen available to the "
     "InfoBoxes, gauges and map overlays. Pick the edges on which it "
     "is stretched out to the screen border. The map always uses the "
     "whole screen, and dialogs and the menu always stay inside the "
     "safe area. Only relevant while full screen mode is enabled.");

/**
 * Describe the enabled edges for the settings list, e.g. "Top, Bottom".
 */
static std::string
FormatSafeAreaStretch(uint8_t edges) noexcept
{
  if (edges == DisplaySettings::SAFE_AREA_STRETCH_NONE)
    return _("None");

  std::string result;

  for (const auto &i : safe_area_stretch_edges) {
    if ((edges & i.bit) == 0)
      continue;

    if (!result.empty())
      result += ", ";
    result += gettext_context("Screen edge", i.label);
  }

  return result;
}

static bool
EditSafeAreaStretch(const char *caption, DataField &df,
                    [[maybe_unused]] const char *help_text)
{
  DisplaySettings &settings = CommonInterface::SetUISettings().display;

  auto picker =
    std::make_unique<SafeAreaStretchWidget>(UIGlobals::GetDialogLook(),
                                            settings.safe_area_stretch);
  auto &picker_ref = *picker;

  TWidgetDialog<StaticHelpTextWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(), _("Stretch safe area"));
  dialog.SetWidget(std::move(picker), gettext(safe_area_stretch_hint));
  dialog.AddButton(_("OK"), mrOK);

  /* the row's help text is not reachable from here: WndProperty only
     falls back to OnHelp() when there is no edit callback */
  dialog.AddButton(_("Help"), [caption](){
    HelpDialog(caption, gettext(safe_area_stretch_help));
  });

  dialog.AddButton(_("Cancel"), mrCancel);

  if (dialog.ShowModal() != mrOK)
    return false;

  const uint8_t edges = picker_ref.GetEdges();
  if (edges == settings.safe_area_stretch)
    return false;

  settings.safe_area_stretch = edges;
  Profile::Set(ProfileKeys::SafeAreaStretch, unsigned(edges));
  Profile::Save();

  /* in "Auto" mode the status bar follows the top edge, and the area
     the InfoBoxes and gauges may use changed */
  CommonInterface::main_window->ApplyFullScreenSettings();
  CommonInterface::main_window->ReinitialiseLayout();

  ((DataFieldString &)df).ModifyValue(FormatSafeAreaStretch(edges).c_str());
  return true;
}

#endif

#ifdef HAVE_STATUS_BAR_SETTING
static constexpr StaticEnumChoice status_bar_list[] = {
  { DisplaySettings::StatusBar::AUTO, NC_("Setting", "Auto"),
    N_("Show the status bar unless the safe area is stretched to the top "
       "screen edge, where it would cover the InfoBoxes.") },
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

static void
FillDpiChoices(DataFieldEnum &df, unsigned value) noexcept
{
  static constexpr unsigned dpi_choices[] = {
    120, 160, 240, 260, 280, 300, 340, 360, 400, 420, 520,
  };

  df.AddChoice(0, _("Automatic"));
  for (unsigned dpi : dpi_choices) {
    StaticString<20> buffer;
    buffer.Format(_("%u dpi"), dpi);
    df.AddChoice(dpi, buffer);
  }
  df.SetValue(value);
}

class DisplayConfigPanel final : public RowFormWidget {
  std::unique_ptr<DisplayBrightness> brightness;

public:
  DisplayConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()),
     brightness(DisplayBrightness::Detect()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
DisplayConfigPanel::Prepare(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
  const UISettings &ui_settings = CommonInterface::GetUISettings();

  RowFormWidget::Prepare(parent, rc);

  AddEnum(C_("Setting", "Display type"),
          _("Select the display technology. E-ink modes disable kinetic "
            "and smooth scrolling for slow refresh screens."),
          display_type_list,
          (unsigned)ui_settings.display.display_type);
  SetExpertRow(AppDisplayType);

  WndProperty *wp_dpi = AddEnum(_("Display resolution"),
                                _("The display resolution is used to adapt line widths, "
                                  "font size, landable size and more."));
  FillDpiChoices(*(DataFieldEnum *)wp_dpi->GetDataField(),
                 ui_settings.custom_dpi);
  wp_dpi->RefreshDisplay();
  SetExpertRow(CustomDPI);

  if (Display::RotateSupported())
    AddEnum(_("Display orientation"),
            _("Rotate the display on devices that support it."),
            display_orientation_list,
            (unsigned)ui_settings.display.orientation);
  else
    AddDummy();

#ifdef HAVE_DISPLAY_BRIGHTNESS_CONTROL
  if (brightness != nullptr) {
    AddInteger(_("Screen brightness"),
               brightness->IsWritable()
               ? _("Adjust the screen brightness.")
               : _("Screen brightness is read-only because writing requires additional permissions."),
               "%d %%", "%d", 0, 100, 5,
               brightness->GetBrightnessPercent());

    if (!brightness->IsWritable())
      SetReadOnly(ScreenBrightness);
  } else
    AddDummy();
#endif

#ifdef HAVE_FULL_SCREEN_SETTING
  AddBoolean(_("Full screen"),
             _("Let XCSoar use the whole screen, including the areas behind "
               "the system bars and the display cutout. The map fills it "
               "completely, while \"Stretch safe area\" decides which edges "
               "the InfoBoxes, gauges and map overlays may reach."),
             ui_settings.display.full_screen);

  auto *edges_row =
    Add(_("Stretch safe area"), gettext(safe_area_stretch_help),
        new DataFieldString(FormatSafeAreaStretch(ui_settings.display
                                                  .safe_area_stretch).c_str()));
  edges_row->SetEditCallback(EditSafeAreaStretch);
  SetExpertRow(SafeAreaStretch);
#endif

#ifdef HAVE_STATUS_BAR_SETTING
  AddEnum(_("System status bar"),
          _("Whether the system status bar with the clock and the battery "
            "level stays visible."),
          status_bar_list,
          unsigned(ui_settings.display.status_bar));
#endif

  AddEnum(_("Dark mode"), nullptr, dark_mode_list,
          (unsigned)ui_settings.dark_mode);

  AddInteger(_("Text size"),
             nullptr,
             "%d %%", "%d",
             UISettings::SCALE_MIN, UISettings::SCALE_MAX,
             UISettings::SCALE_STEP,
             ui_settings.scale);

#ifdef DRAW_MOUSE_CURSOR
  AddInteger(_("Cursor zoom"), _("Cursor zoom factor"), "%d x", "%d x",
             1, 10, 1, (unsigned)ui_settings.display.cursor_size);
  AddBoolean(_("Invert cursor color"), _("Enable black cursor"),
             ui_settings.display.invert_cursor_colors);
#endif
}

bool
DisplayConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  UISettings &ui_settings = CommonInterface::SetUISettings();

  if (SaveValueEnum(AppDisplayType, ProfileKeys::DisplayType,
                    ui_settings.display.display_type)) {
    changed = true;
    SetDisplayType(ui_settings.display.display_type);
  }

  if (SaveValueEnum(CustomDPI, ProfileKeys::CustomDPI,
                    ui_settings.custom_dpi))
    require_restart = changed = true;

  if (Display::RotateSupported() &&
      SaveValueEnum(Orientation, ProfileKeys::MapOrientation,
                    ui_settings.display.orientation)) {
    changed = true;

    if (!Display::Rotate(ui_settings.display.orientation))
      LogString("Display rotation failed");

#ifdef USE_POLL_EVENT
    UI::event_queue->SetDisplayOrientation(ui_settings.display.orientation);
#endif

    CommonInterface::main_window->CheckResize();
  }

#ifdef HAVE_DISPLAY_BRIGHTNESS_CONTROL
  if (brightness != nullptr && brightness->IsWritable()) {
    const unsigned old_percent = brightness->GetBrightnessPercent();
    const unsigned new_percent = GetValueInteger(ScreenBrightness);
    if (new_percent != old_percent)
      brightness->SetBrightnessPercent(new_percent);
  }
#endif

#ifdef HAVE_FULL_SCREEN_SETTING
  bool full_screen_changed =
    SaveValue(FullScreen, ProfileKeys::FullScreen,
              ui_settings.display.full_screen);
  /* the per-edge settings are applied by their own dialog */
  changed |= full_screen_changed;

#ifdef HAVE_STATUS_BAR_SETTING
  if (SaveValueEnum(StatusBar, ProfileKeys::StatusBar,
                    ui_settings.display.status_bar)) {
    changed = true;
    full_screen_changed = true;
  }
#endif

  /* this may change the usable screen area, so do it before the
     remaining settings are applied */
  if (full_screen_changed)
    CommonInterface::main_window->ApplyFullScreenSettings();
#endif

  changed |= SaveValueEnum(DarkMode, ProfileKeys::DarkMode,
                           ui_settings.dark_mode);

  if (SaveValueInteger(UIScale, ProfileKeys::UIScale,
                       ui_settings.scale))
    require_restart = changed = true;

#ifdef DRAW_MOUSE_CURSOR
  changed |= SaveValueInteger(CursorSize, ProfileKeys::CursorSize,
                              ui_settings.display.cursor_size);
  CommonInterface::main_window->SetCursorSize(ui_settings.display.cursor_size);

  changed |= SaveValue(CursorInverted, ProfileKeys::CursorColorsInverted,
                       ui_settings.display.invert_cursor_colors);
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
