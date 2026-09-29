// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/Dialogs.h"
#include "Dialogs/Message.hpp"
#include "Widget/ArrowPagerWidget.hpp"
#include "Widget/CreateWindowWidget.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "ui/event/KeyCode.hpp"
#include "Form/TabMenuDisplay.hpp"
#include "Form/TabMenuData.hpp"
#include "Form/CheckBox.hpp"
#include "Form/Button.hpp"
#include "Screen/Layout.hpp"
#include "Profile/Profile.hpp"
#include "util/Macros.hpp"
#include "Panels/ConfigPanel.hpp"
#include "ConfigTileMenu.hpp"
#include "Panels/PagesConfigPanel.hpp"
#include "Panels/UnitsConfigPanel.hpp"
#include "Panels/TimeConfigPanel.hpp"
#include "Panels/LoggerConfigPanel.hpp"
#include "Panels/AirspaceConfigPanel.hpp"
#include "Panels/SiteConfigPanel.hpp"
#include "Panels/MapDisplayConfigPanel.hpp"
#include "Panels/WaypointDisplayConfigPanel.hpp"
#include "Panels/SymbolsConfigPanel.hpp"
#include "Panels/TerrainDisplayConfigPanel.hpp"
#include "Panels/GlideComputerConfigPanel.hpp"
#include "Panels/WindConfigPanel.hpp"
#include "Panels/SafetyFactorsConfigPanel.hpp"
#include "Panels/RouteConfigPanel.hpp"
#include "Panels/InterfaceConfigPanel.hpp"
#include "Panels/DisplayConfigPanel.hpp"
#include "Panels/LayoutConfigPanel.hpp"
#include "Panels/GaugesConfigPanel.hpp"
#include "Panels/VarioConfigPanel.hpp"
#include "Panels/TaskRulesConfigPanel.hpp"
#include "Panels/TaskDefaultsConfigPanel.hpp"
#include "Panels/ScoringConfigPanel.hpp"
#include "Panels/InfoBoxesConfigPanel.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Audio/Features.hpp"
#include "UtilsSettings.hpp"
#include "net/http/Features.hpp"

#ifdef HAVE_HTTP
#include "Panels/NOTAMConfigPanel.hpp"
#endif

#ifdef HAVE_PCM_PLAYER
#include "Panels/AudioVarioConfigPanel.hpp"
#endif

#ifdef HAVE_VOLUME_CONTROLLER
#include "Panels/AudioConfigPanel.hpp"
#endif

#ifdef HAVE_TRACKING
#include "Panels/TrackingConfigPanel.hpp"
#include "Panels/CloudConfigPanel.hpp"
#endif

#ifdef HAVE_HTTP
#include "Panels/WeatherConfigPanel.hpp"
#endif
#include "Panels/RaspConfigPanel.hpp"
#ifdef HAVE_PCMET
#include "Panels/PCMetConfigPanel.hpp"
#endif
#ifdef HAVE_HTTP
#include "Panels/XCThermConfigPanel.hpp"
#endif
#ifdef HAVE_HTTP
#include "Panels/SkySightConfigPanel.hpp"
#endif

#include "Panels/WeGlideConfigPanel.hpp"
#include "Panels/NetworkConfigPanel.hpp"

#if defined(__linux__) && !defined(__ANDROID__) && !defined(KOBO)
#include "Panels/SystemdConfigPanel.hpp"
#endif

#include <cassert>

static unsigned current_page;

// TODO: eliminate global variables
static ArrowPagerWidget *pager;
static ConfigTileMenu *tile_menu;

static constexpr TabMenuPage files_pages[] = {
  { N_("Site Files"), CreateSiteConfigPanel },
  { nullptr, nullptr }
};

static constexpr TabMenuPage map_pages[] = {
  { N_("Orientation"), CreateMapDisplayConfigPanel },
  { N_("Elements"), CreateSymbolsConfigPanel },
  { N_("Waypoints"), CreateWaypointDisplayConfigPanel },
  { N_("Terrain"), CreateTerrainDisplayConfigPanel },
  { N_("Airspace"), CreateAirspaceConfigPanel },
#ifdef HAVE_HTTP
  { NC_("Setting", "NOTAM"), CreateNOTAMConfigPanel },
#endif
  { nullptr, nullptr }
};

static constexpr TabMenuPage computer_pages[] = {
  { N_("Safety Factors"), CreateSafetyFactorsConfigPanel },
  { N_("Glide Computer"), CreateGlideComputerConfigPanel },
  { N_("Wind"), CreateWindConfigPanel },
  { N_("Route"), CreateRouteConfigPanel },
  { N_("Scoring"), CreateScoringConfigPanel },
  { nullptr, nullptr }
};

static constexpr TabMenuPage gauge_pages[] = {
  { N_("FLARM, Other"), CreateGaugesConfigPanel },
  { N_("Vario"), CreateVarioConfigPanel },
#ifdef HAVE_PCM_PLAYER
  { N_("Audio Vario"), CreateAudioVarioConfigPanel },
#endif
  { nullptr, nullptr }
};

static constexpr TabMenuPage task_pages[] = {
  { N_("Task Rules"), CreateTaskRulesConfigPanel },
  { N_("Turnpoint Types"), CreateTaskDefaultsConfigPanel },
  { nullptr, nullptr }
};

static constexpr TabMenuPage look_pages[] = {
  { N_("Language, Input"), CreateInterfaceConfigPanel },
  { N_("Display"), CreateDisplayConfigPanel },
  { N_("Layout"), CreateLayoutConfigPanel },
  { N_("Pages"), CreatePagesConfigPanel },
  { N_("InfoBox Sets"), CreateInfoBoxesConfigPanel },
  { nullptr, nullptr }
};

static constexpr TabMenuPage weather_pages[] = {
#ifdef HAVE_HTTP
  { N_("Thermal Information Map"), CreateWeatherConfigPanel },
#endif
  { "RASP", CreateRaspConfigPanel },
#ifdef HAVE_HTTP
  { "SkySight", CreateSkySightConfigPanel },
#endif
#ifdef HAVE_PCMET
  { "Flugwetter (pc_met)", CreatePCMetConfigPanel },
#endif
#ifdef HAVE_HTTP
  { "XC Therm", CreateXCThermConfigPanel },
#endif
  { nullptr, nullptr }
};

static constexpr TabMenuPage setup_pages[] = {
  { N_("Logger"), CreateLoggerConfigPanel },
  { N_("Units"), CreateUnitsConfigPanel },
  // Important: all pages after Units in this list must not have data fields that are
  // unit-dependent because they will be saved after their units may have changed.
  // ToDo: implement API that controls order in which pages are saved
  { NC_("Setting", "Time"), CreateTimeConfigPanel },
#ifdef HAVE_TRACKING
  { N_("Tracking"), CreateTrackingConfigPanel },
  { "XCSoar Cloud", CreateCloudConfigPanel },
#endif
  { "WeGlide", CreateWeGlideConfigPanel },
#ifdef HAVE_VOLUME_CONTROLLER
  { N_("Audio"), CreateAudioConfigPanel },
#endif
  { N_("Network"), CreateNetworkConfigPanel },
#if defined(__linux__) && !defined(__ANDROID__) && !defined(KOBO)
  { N_("Services"), CreateSystemdConfigPanel },
#endif
  { nullptr, nullptr }
};

static constexpr TabMenuGroup main_menu_captions[] = {
  { N_("Site Files"), files_pages },
  { N_("Map Display"), map_pages },
  { N_("Glide Computer"), computer_pages },
  { N_("Gauges"), gauge_pages },
  { N_("Task Defaults"), task_pages },
  { N_("Look"), look_pages },
  { N_("Weather"), weather_pages },
  { NC_("Menu", "Setup"), setup_pages },
};

static void
OnUserLevel(bool expert) noexcept;

class ConfigurationExtraButtons final
  : public NullWidget {
  struct Layout {
    PixelRect expert, button2, button1;

    Layout(const PixelRect &rc):expert(rc), button2(rc), button1(rc) {
      const unsigned height = rc.GetHeight();
      const unsigned max_control_height = ::Layout::GetMaximumControlHeight();

      if (height >= 3 * max_control_height) {
        expert.bottom = expert.top + max_control_height;

        button1.top = button2.bottom = rc.bottom - max_control_height;
        button2.top = button2.bottom - max_control_height;
      } else {
        expert.right = button2.left = unsigned(rc.left * 2 + rc.right) / 3;
        button2.right = button1.left = unsigned(rc.left + rc.right * 2) / 3;
      }
    }
  };

  const DialogLook &look;

  CheckBoxControl expert;
  Button button2, button1;
  bool borrowed2, borrowed1;

public:
  ConfigurationExtraButtons(const DialogLook &_look)
    :look(_look),
     borrowed2(false), borrowed1(false) {}

  Button &GetButton(unsigned number) {
    switch (number) {
    case 1:
      return button1;

    case 2:
      return button2;

    default:
      assert(false);
      gcc_unreachable();
    }
  }

protected:
  /* virtual methods from Widget */
  PixelSize GetMinimumSize() const noexcept override {
    return {
      CheckBoxControl::GetMinimumWidth(look,
                                       ::Layout::GetMaximumControlHeight(),
                                       _("Expert")),
      ::Layout::GetMaximumControlHeight() * 3,
    };
  }

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    Layout layout(rc);

    expert.CreateInDialogForm(parent, look, _("Expert"), layout.expert,
                              [](bool value){ OnUserLevel(value); });

    WindowStyle style;
    style.Hide();
    style.TabStop();

    button2.Create(parent, look.button, "", layout.button2, style);
    button1.Create(parent, look.button, "", layout.button1, style);
  }

  void Show(const PixelRect &rc) noexcept override {
    Layout layout(rc);

    expert.SetState(CommonInterface::GetUISettings().dialog.expert);
    expert.MoveAndShow(layout.expert);

    if (borrowed2)
      button2.MoveAndShow(layout.button2);
    else
      button2.Move(layout.button2);

    if (borrowed1)
      button1.MoveAndShow(layout.button1);
    else
      button1.Move(layout.button1);
  }

  void Hide() noexcept override {
    expert.FastHide();
    button2.FastHide();
    button1.FastHide();
  }

  void Move(const PixelRect &rc) noexcept override {
    Layout layout(rc);
    expert.Move(layout.expert);
    button2.Move(layout.button2);
    button1.Move(layout.button1);
  }
};

void
ConfigPanel::BorrowExtraButton(unsigned i, const char *caption,
                               std::function<void()> callback) noexcept
{
  ConfigurationExtraButtons &extra =
    (ConfigurationExtraButtons &)pager->GetExtra();
  Button &button = extra.GetButton(i);
  button.SetCaption(caption);
  button.SetCallback(std::move(callback));
  button.Show();
}

void
ConfigPanel::ReturnExtraButton(unsigned i)
{
  ConfigurationExtraButtons &extra =
    (ConfigurationExtraButtons &)pager->GetExtra();
  Button &button = extra.GetButton(i);
  button.Hide();
}

static void
OnUserLevel(bool expert) noexcept
{
  CommonInterface::SetUISettings().dialog.expert = expert;

  /* Keep Profile I/O out of this checkbox callback (pager is mid-
     relayout). Persist UserLevel when the dialog closes instead. */

  /* force layout update */
  pager->PagerWidget::Move(pager->GetPosition());
}

/**
 * Close on the menu page commits (mrOK).  On a settings page, return
 * to the menu (Back).  From a tiled submenu, return to the group
 * tiles first.
 */
static void
OnCloseClicked(WidgetDialog &dialog)
{
  if (pager->GetCurrentIndex() == 0) {
    if (tile_menu != nullptr && tile_menu->GoBackToMain()) {
      dialog.SetCaption(_("Configuration"));
      pager->SetCloseButtonCaption(_("Close"));
      return;
    }
    dialog.SetModalResult(mrOK);
  } else
    pager->ClickPage(0);
}

static void
SetConfigurationCaption(WidgetDialog &dialog, const char *caption) noexcept
{
  if (caption == nullptr)
    caption = _("Configuration");
  dialog.SetCaption(caption);

  const bool on_menu = pager->GetCurrentIndex() == 0;
  const bool tiled_submenu =
    on_menu && tile_menu != nullptr && !tile_menu->IsShowingMain();
  pager->SetCloseButtonCaption(on_menu && !tiled_submenu
                               ? _("Close")
                               : _("Back"));
}

static void
OnPageFlippedList(WidgetDialog &dialog, TabMenuDisplay &menu)
{
  menu.OnPageFlipped();

  char buffer[128];
  SetConfigurationCaption(dialog, menu.GetCaption(buffer, ARRAY_SIZE(buffer)));
}

static void
OnPageFlippedTiles(WidgetDialog &dialog, ConfigTileMenu &menu)
{
  menu.OnPageFlipped();

  char buffer[128];
  SetConfigurationCaption(dialog, menu.GetCaption(buffer, ARRAY_SIZE(buffer)));
}

static void
PersistExpertAndSave(WidgetDialog &dialog, int result) noexcept
{
  bool expert_changed = false;
  if (result == mrOK) {
    const bool expert = CommonInterface::GetUISettings().dialog.expert;
    if (expert) {
      bool profile_expert = false;
      Profile::Get(ProfileKeys::UserLevel, profile_expert);
      if (!profile_expert) {
        Profile::Set(ProfileKeys::UserLevel, true);
        expert_changed = true;
      }
    } else if (Profile::Exists(ProfileKeys::UserLevel)) {
      Profile::Remove(ProfileKeys::UserLevel);
      expert_changed = true;
    }
  }

  if (dialog.GetChanged() || expert_changed) {
    Profile::Save();
    if (require_restart)
      ShowMessageBox(_("Changes to configuration saved. Restart XCSoar to apply changes."),
                  "", MB_OK);
  }
}

void dlgConfigurationShowModal()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  const bool use_tiles =
    CommonInterface::GetUISettings().dialog.tiled_menu;

  tile_menu = nullptr;

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Configuration"));

  pager = new ArrowPagerWidget(look.button,
                               [&dialog](){ OnCloseClicked(dialog); },
                               std::make_unique<ConfigurationExtraButtons>(look));

  if (use_tiles) {
    auto _menu = std::make_unique<ConfigTileMenu>(*pager, look);
    auto &menu = *_menu;
    tile_menu = &menu;
    pager->Add(std::make_unique<CreateWindowWidget>(
                 [&_menu](ContainerWindow &parent, const PixelRect &rc,
                          WindowStyle style) {
      style.TabStop();
      _menu->Create(parent, rc, style);
      return std::move(_menu);
    }));

    menu.InitMenu(main_menu_captions, ARRAY_SIZE(main_menu_captions));
    menu.SetCursor(current_page);

    pager->SetPageFlippedCallback([&dialog, &menu](){
      OnPageFlippedTiles(dialog, menu);
    });

    dialog.FinishPreliminary(pager);

    dialog.SetKeyDownFunction([&dialog](unsigned key_code) {
      if (key_code != KEY_ESCAPE)
        return false;

      if (pager->GetCurrentIndex() == 0) {
        if (tile_menu != nullptr && tile_menu->GoBackToMain()) {
          dialog.SetCaption(_("Configuration"));
          pager->SetCloseButtonCaption(_("Close"));
          return true;
        }
        return false;
      }

      OnCloseClicked(dialog);
      return true;
    });

    const int result = dialog.ShowModal();
    current_page = menu.GetCursor();
    tile_menu = nullptr;
    PersistExpertAndSave(dialog, result);
    return;
  }

  auto _menu = std::make_unique<TabMenuDisplay>(*pager, look);
  auto &menu = *_menu;
  pager->Add(std::make_unique<CreateWindowWidget>([&_menu](ContainerWindow &parent,
                                                           const PixelRect &rc,
                                                           WindowStyle style) {
    style.TabStop();
    _menu->Create(parent, rc, style);
    return std::move(_menu);
  }));

  menu.InitMenu(main_menu_captions, ARRAY_SIZE(main_menu_captions));

  /* restore last selected menu item */
  menu.SetCursor(current_page);

  pager->SetPageFlippedCallback([&dialog, &menu](){
    OnPageFlippedList(dialog, menu);
  });

  dialog.FinishPreliminary(pager);

  /* Esc on a settings panel returns to the menu (same as Back);
     on the menu itself, leave Esc to cancel the dialog. */
  dialog.SetKeyDownFunction([&dialog](unsigned key_code) {
    if (key_code != KEY_ESCAPE || pager->GetCurrentIndex() == 0)
      return false;

    OnCloseClicked(dialog);
    return true;
  });

  const int result = dialog.ShowModal();

  /* save page number for next time this dialog is opened */
  current_page = menu.GetCursor();

  PersistExpertAndSave(dialog, result);
}
