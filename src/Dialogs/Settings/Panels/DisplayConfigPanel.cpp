// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DisplayConfigPanel.hpp"
#include "ui/canvas/Features.hpp" // for DRAW_MOUSE_CURSOR
#include "Profile/Keys.hpp"
#include "Form/DataField/Enum.hpp"
#include "Hardware/DisplayBrightness.hpp"
#include "Hardware/RotateDisplay.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "LogFile.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "UtilsSettings.hpp"
#include "Asset.hpp"
#include "util/Macros.hpp"
#include "util/StaticString.hxx"

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

enum ControlIndex {
  AppDisplayType,
  CustomDPI,
  Orientation,
#if defined(HAVE_DISPLAY_BRIGHTNESS_CONTROL)
  ScreenBrightness,
#endif
#ifdef ANDROID
  FullScreen,
#endif
#ifdef DRAW_MOUSE_CURSOR
  CursorSize,
  CursorInverted,
#endif
};

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

#ifdef ANDROID
  AddBoolean(_("Full screen"), _("Run XCSoar in full screen mode"),
             ui_settings.display.full_screen);
#endif

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

#ifdef ANDROID
  changed |= SaveValue(FullScreen, ProfileKeys::FullScreen,
                       ui_settings.display.full_screen);
  native_view->SetFullScreen(Java::GetEnv(), ui_settings.display.full_screen);
#endif

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
