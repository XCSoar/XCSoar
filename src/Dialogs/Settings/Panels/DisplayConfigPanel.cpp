// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DisplayConfigPanel.hpp"
#include "ui/canvas/Features.hpp" // for DRAW_MOUSE_CURSOR
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Form/DataField/Enum.hpp"
#include "Hardware/RotateDisplay.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "LogFile.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "UtilsSettings.hpp"
#include "Asset.hpp"
#include "ActionInterface.hpp"
#include "util/Macros.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#endif

#ifdef USE_POLL_EVENT
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"
#endif

enum ControlIndex {
#ifdef ANDROID
  FullScreen,
#endif
  Orientation,
  DarkMode,
  AppDisplayType,
#ifdef DRAW_MOUSE_CURSOR
  CursorSize,
  CursorInverted,
#endif
};

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

static constexpr StaticEnumChoice display_type_list[] = {
  { ::DisplayType::LCD, N_("LCD"),
    N_("Conventional LCD or OLED. Full scrolling animations.") },
  { ::DisplayType::E_INK, N_("E-ink"),
    N_("Monochrome electronic paper. Disables kinetic and smooth "
       "scrolling.") },
  { ::DisplayType::COLOR_E_INK, N_("Color e-ink"),
    N_("Color electronic paper. Disables kinetic and smooth "
       "scrolling like monochrome e-ink.") },
  nullptr
};

static_assert(ARRAY_SIZE(display_type_list) ==
              unsigned(::DisplayType::COUNT) + 1,
              "display_type_list must match DisplayType::COUNT");

class DisplayConfigPanel final : public RowFormWidget {
public:
  DisplayConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
DisplayConfigPanel::Prepare(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
  const UISettings &ui_settings = CommonInterface::GetUISettings();

  RowFormWidget::Prepare(parent, rc);

#ifdef ANDROID
  AddBoolean(_("Full screen"), _("Run XCSoar in full screen mode"),
             ui_settings.display.full_screen);
#endif

  if (Display::RotateSupported())
    AddEnum(_("Display orientation"),
            _("Rotate the display on devices that support it."),
            display_orientation_list,
            (unsigned)ui_settings.display.orientation);
  else
    AddDummy();

#ifndef KOBO
  AddEnum(_("Dark mode"), nullptr, dark_mode_list,
          (unsigned)ui_settings.dark_mode);
  SetExpertRow(DarkMode);
#else
  AddDummy();
#endif

  AddEnum(_("Display type"),
          _("Select the display technology. E-ink modes disable kinetic "
            "and smooth scrolling for slow refresh screens."),
          display_type_list,
          (unsigned)ui_settings.display.display_type);
  SetExpertRow(AppDisplayType);

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

#ifdef ANDROID
  changed |= SaveValue(FullScreen, ProfileKeys::FullScreen,
                       ui_settings.display.full_screen);
  native_view->SetFullScreen(Java::GetEnv(), ui_settings.display.full_screen);
#endif

  bool orientation_changed = false;

  if (Display::RotateSupported()) {
    orientation_changed =
      SaveValueEnum(Orientation, ProfileKeys::MapOrientation,
                    ui_settings.display.orientation);
    changed |= orientation_changed;
  }

#ifndef KOBO
  changed |= SaveValueEnum(DarkMode, ProfileKeys::DarkMode,
                           ui_settings.dark_mode);
#else
  if (ui_settings.dark_mode != UISettings::DarkMode::OFF) {
    ui_settings.dark_mode = UISettings::DarkMode::OFF;
    changed = true;
  }
#endif

  if (SaveValueEnum(AppDisplayType, ProfileKeys::DisplayType,
                    ui_settings.display.display_type)) {
    changed = true;
    SetDisplayType(ui_settings.display.display_type);
  }

#ifdef DRAW_MOUSE_CURSOR
  changed |= SaveValueInteger(CursorSize, ProfileKeys::CursorSize,
                              ui_settings.display.cursor_size);
  CommonInterface::main_window->SetCursorSize(ui_settings.display.cursor_size);

  changed |= SaveValue(CursorInverted, ProfileKeys::CursorColorsInverted,
                       ui_settings.display.invert_cursor_colors);
  CommonInterface::main_window->SetCursorColorsInverted(
    ui_settings.display.invert_cursor_colors);
#endif

  if (orientation_changed) {
    assert(Display::RotateSupported());

    if (!Display::Rotate(ui_settings.display.orientation))
      LogString("Display rotation failed");

#ifdef USE_POLL_EVENT
    UI::event_queue->SetDisplayOrientation(ui_settings.display.orientation);
#endif

    CommonInterface::main_window->CheckResize();
  }

  _changed |= changed;
  return true;
}

std::unique_ptr<Widget>
CreateDisplayConfigPanel()
{
  return std::make_unique<DisplayConfigPanel>();
}
