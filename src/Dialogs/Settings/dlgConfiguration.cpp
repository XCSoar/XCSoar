// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/Dialogs.h"
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
#include "Renderer/TextButtonRenderer.hpp"
#include "Screen/Layout.hpp"
#include "Profile/Profile.hpp"
#include "util/Macros.hpp"
#include "util/ScopeExit.hxx"
#include "Panels/ConfigPanel.hpp"
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
#include "Panels/MapOverlaysConfigPanel.hpp"
#include "Panels/TrafficConfigPanel.hpp"
#include "Panels/VarioConfigPanel.hpp"
#include "Panels/TaskRulesConfigPanel.hpp"
#include "Panels/TaskDefaultsConfigPanel.hpp"
#include "Panels/ScoringConfigPanel.hpp"
#include "Panels/InfoBoxesConfigPanel.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Audio/Features.hpp"
#include "net/http/Features.hpp"

#ifdef HAVE_HTTP
#include "Panels/NOTAMConfigPanel.hpp"
#endif

#ifdef HAVE_PCM_PLAYER
#include "Panels/AudioConfigPanel.hpp"
#endif

#include "Tracking/Features.hpp"
#ifdef HAVE_SKYLINES_TRACKING
#include "Panels/SkyLinesConfigPanel.hpp"
#endif
#ifdef HAVE_LIVETRACK24
#include "Panels/LiveTrack24ConfigPanel.hpp"
#endif
#ifdef HAVE_TRACKING
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
#ifdef HAVE_HTTP
#include "Panels/SoaringScoringConfigPanel.hpp"
#endif
#include "Panels/NetworkConfigPanel.hpp"

#if defined(__linux__) && !defined(__ANDROID__) && !defined(KOBO)
#include "Panels/SystemdConfigPanel.hpp"
#ifdef USE_POLL_EVENT
#include "Panels/SystemToolsPanel.hpp"
#endif
#endif

#include <algorithm>
#include <cassert>

static unsigned current_page;

// TODO: eliminate global variables
static ArrowPagerWidget *pager;

static constexpr TabMenuPage files_pages[] = {
  { N_("Site Files"), CreateSiteConfigPanel },
  { nullptr, nullptr }
};

static constexpr TabMenuPage map_pages[] = {
  { N_("Airspace"), CreateAirspaceConfigPanel },
  { N_("Elements"), CreateSymbolsConfigPanel },
#ifdef HAVE_HTTP
  { NC_("Setting", "NOTAM"), CreateNOTAMConfigPanel },
#endif
  { N_("Orientation"), CreateMapDisplayConfigPanel },
  { N_("Overlays"), CreateMapOverlaysConfigPanel },
  { N_("Terrain"), CreateTerrainDisplayConfigPanel },
  { N_("Waypoints"), CreateWaypointDisplayConfigPanel },
  { nullptr, nullptr }
};

static constexpr TabMenuPage computer_pages[] = {
  { N_("Glide Computer"), CreateGlideComputerConfigPanel },
  { N_("Route"), CreateRouteConfigPanel },
  { N_("Safety Factors"), CreateSafetyFactorsConfigPanel },
  { N_("Wind"), CreateWindConfigPanel },
  { nullptr, nullptr }
};

static constexpr TabMenuPage gauge_pages[] = {
  { N_("Thermal Assistant"), CreateGaugesConfigPanel },
  { N_("Traffic"), CreateTrafficConfigPanel },
  { nullptr, nullptr }
};

static constexpr TabMenuPage task_pages[] = {
  { N_("Scoring"), CreateScoringConfigPanel },
  { N_("Task Rules"), CreateTaskRulesConfigPanel },
  { N_("Turnpoint Types"), CreateTaskDefaultsConfigPanel },
  { nullptr, nullptr }
};

static constexpr TabMenuPage look_pages[] = {
  { N_("InfoBox Sets"), CreateInfoBoxesConfigPanel },
  { N_("Layout"), CreateLayoutConfigPanel },
  { N_("Pages"), CreatePagesConfigPanel },
  { N_("Vario"), CreateVarioConfigPanel },
  { nullptr, nullptr }
};

static constexpr TabMenuPage weather_pages[] = {
#ifdef HAVE_PCMET
  { "Flugwetter (pc_met)", CreatePCMetConfigPanel },
#endif
  { "RASP", CreateRaspConfigPanel },
#ifdef HAVE_HTTP
  { "SkySight", CreateSkySightConfigPanel },
  { N_("Thermal Information Map"), CreateWeatherConfigPanel },
  { "XC Therm", CreateXCThermConfigPanel },
#endif
  { nullptr, nullptr }
};

static constexpr TabMenuPage online_pages[] = {
#ifdef HAVE_LIVETRACK24
  { "LiveTrack24", CreateLiveTrack24ConfigPanel },
#endif
#ifdef HAVE_SKYLINES_TRACKING
  { "SkyLines", CreateSkyLinesConfigPanel },
#endif
  { "WeGlide", CreateWeGlideConfigPanel },
#ifdef HAVE_HTTP
  { "SoaringScoring", CreateSoaringScoringConfigPanel },
#endif
#ifdef HAVE_TRACKING
  { "XCSoar Cloud", CreateCloudConfigPanel },
#endif
  { nullptr, nullptr }
};

static constexpr TabMenuPage setup_pages[] = {
#ifdef HAVE_PCM_PLAYER
  /* Before Units: audio vario deadband uses vertical-speed units. */
  { N_("Audio"), CreateAudioConfigPanel },
#endif
  { N_("Language, Input"), CreateInterfaceConfigPanel },
  { N_("Logger"), CreateLoggerConfigPanel },
  { N_("Units"), CreateUnitsConfigPanel },
  // Important: all pages after Units in this list must not have data fields that are
  // unit-dependent because they will be saved after their units may have changed.
  // ToDo: implement API that controls order in which pages are saved
  { N_("Network"), CreateNetworkConfigPanel },
  { N_("Screen"), CreateDisplayConfigPanel },
#if defined(__linux__) && !defined(__ANDROID__) && !defined(KOBO)
  { N_("System Services"), CreateSystemdConfigPanel },
#ifdef USE_POLL_EVENT
  { N_("System Tools"), CreateSystemToolsPanel, HasSystemTools },
#endif
#endif
  { NC_("Setting", "Time"), CreateTimeConfigPanel },
  { nullptr, nullptr }
};

static constexpr TabMenuGroup main_menu_captions[] = {
  { N_("Site Files"), files_pages },
  { N_("Map Display"), map_pages },
  { N_("Glide Computer"), computer_pages },
  { N_("Gauges"), gauge_pages },
  { N_("Task"), task_pages },
  { N_("Look"), look_pages },
  { N_("Weather"), weather_pages },
  { NC_("Menu", "Services"), online_pages },
  { NC_("Menu", "Setup"), setup_pages },
};

static void
OnUserLevel(bool expert) noexcept;

class ConfigurationExtraButtons final
  : public NullWidget {
  /** Landscape stacks up to this many page actions above Previous. */
  static constexpr unsigned MAX_EXTRA_BUTTONS = 4;

  const DialogLook &look;

  CheckBoxControl expert;
  Button buttons[MAX_EXTRA_BUTTONS];
  bool borrowed[MAX_EXTRA_BUTTONS] = {};

  PixelRect position{};

  /** Taller than the single portrait row: the landscape column. */
  bool IsStack(const PixelRect &rc) const noexcept {
    return rc.GetHeight() > ::Layout::GetMaximumControlHeight();
  }

  void Apply(const PixelRect &rc) noexcept;

public:
  explicit ConfigurationExtraButtons(const DialogLook &_look) noexcept
    :look(_look) {}

  Button &GetButton(unsigned number) noexcept {
    assert(number >= 1 && number <= MAX_EXTRA_BUTTONS);
    return buttons[number - 1];
  }

  CheckBoxControl &GetExpert() noexcept {
    return expert;
  }

  void Borrow(unsigned number, const char *caption,
              std::function<void()> callback) noexcept {
    assert(number >= 1 && number <= MAX_EXTRA_BUTTONS);
    const unsigned i = number - 1;
    borrowed[i] = true;
    buttons[i].SetMenuCaption(look.button, caption);
    buttons[i].SetCallback(std::move(callback));
    if (expert.IsDefined())
      Apply(position);
  }

  void Return(unsigned number) noexcept {
    assert(number >= 1 && number <= MAX_EXTRA_BUTTONS);
    const unsigned i = number - 1;
    borrowed[i] = false;
    if (buttons[i].IsDefined())
      buttons[i].Hide();
  }

protected:
  /* virtual methods from Widget */
  PixelSize GetMinimumSize() const noexcept override {
    const unsigned height = ::Layout::GetMaximumControlHeight();
    unsigned width = CheckBoxControl::GetMinimumWidth(look, height,
                                                      _("Expert"));
    const ButtonLook &button_look = look.button;
    const char *const captions[] = {
      C_("Button", "Add"),
      C_("Button", "Delete"),
      _("Turn off"),
      _("Restart"),
    };
    for (const char *caption : captions)
      width = std::max(width,
                       TextButtonRenderer::GetMinimumButtonWidth(
                         button_look, caption));
    return { width, height * 3 };
  }

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    position = rc;

    expert.CreateInDialogForm(parent, look, _("Expert"), rc,
                              [](bool value){ OnUserLevel(value); });

    WindowStyle style;
    style.Hide();
    style.TabStop();

    /* High index first so child order matches the stack, top to
       bottom.  Up then walks v → ^ → Delete → Add. */
    for (unsigned n = MAX_EXTRA_BUTTONS; n > 0; --n)
      buttons[n - 1].Create(parent, look.button, "", rc, style);

    Apply(rc);
  }

  void Show(const PixelRect &rc) noexcept override {
    expert.SetState(CommonInterface::GetUISettings().dialog.expert);
    expert.Show();
    Apply(rc);
  }

  void Hide() noexcept override {
    expert.FastHide();
    for (auto &button : buttons)
      button.FastHide();
  }

  void Move(const PixelRect &rc) noexcept override {
    Apply(rc);
  }
};

void
ConfigurationExtraButtons::Apply(const PixelRect &rc) noexcept
{
  if (!expert.IsDefined())
    return;

  position = rc;
  if (rc.top >= rc.bottom || rc.left >= rc.right)
    return;

  const unsigned h = ::Layout::GetMaximumControlHeight();
  const bool stack = IsStack(rc);
  bool placed[MAX_EXTRA_BUTTONS] = {};

  auto place = [&](unsigned n, PixelRect br) {
    if (br.top >= br.bottom || br.left >= br.right)
      return;
    buttons[n].Move(br);
    placed[n] = true;
  };

  if (!stack) {
    /* Portrait: Expert | button 2 | button 1. */
    PixelRect expert_rc = rc, middle = rc, right = rc;
    expert_rc.right = middle.left =
      (unsigned)(rc.left * 2 + rc.right) / 3;
    middle.right = right.left =
      (unsigned)(rc.left + rc.right * 2) / 3;
    if (expert_rc.left < expert_rc.right)
      expert.Move(expert_rc);
    place(1, middle);
    place(0, right);
  } else {
    PixelRect expert_rc = rc;
    expert_rc.bottom = expert_rc.top + (int)h;
    if (expert_rc.bottom > rc.bottom)
      expert_rc.bottom = rc.bottom;
    if (expert_rc.top < expert_rc.bottom)
      expert.Move(expert_rc);

    unsigned count = 0;
    for (unsigned n = 0; n < MAX_EXTRA_BUTTONS; ++n)
      if (borrowed[n])
        ++count;

    const int room = rc.bottom - expert_rc.bottom;
    unsigned slot = h;
    if (count > 0 && room > 0 && room < (int)(count * h))
      slot = std::max(1u, unsigned(room) / count);

    int bottom = rc.bottom;
    for (unsigned n = 0; n < MAX_EXTRA_BUTTONS; ++n) {
      if (!borrowed[n] || bottom <= expert_rc.bottom)
        continue;

      PixelRect br = rc;
      br.bottom = bottom;
      br.top = bottom - (int)slot;
      if (br.top < expert_rc.bottom)
        br.top = expert_rc.bottom;
      bottom = br.top;
      place(n, br);
    }
  }

  if (!expert.IsVisible())
    return;

  for (unsigned n = 0; n < MAX_EXTRA_BUTTONS; ++n) {
    if (!buttons[n].IsDefined())
      continue;

    /* Portrait has room for the original two buttons only. */
    if (borrowed[n] && placed[n] && (stack || n < 2))
      buttons[n].Show();
    else
      buttons[n].Hide();
  }
}

bool
ConfigPanel::HasExtraButtons() noexcept
{
  return pager != nullptr;
}

Button &
ConfigPanel::BorrowExtraButton(unsigned i, const char *caption,
                               std::function<void()> callback) noexcept
{
  assert(pager != nullptr);

  auto &extra = (ConfigurationExtraButtons &)pager->GetExtra();
  extra.Borrow(i, caption, std::move(callback));
  return extra.GetButton(i);
}

void
ConfigPanel::ReturnExtraButton(unsigned i)
{
  assert(pager != nullptr);

  auto &extra = (ConfigurationExtraButtons &)pager->GetExtra();
  extra.Return(i);
}

CheckBoxControl &
ConfigPanel::GetExpertCheckbox() noexcept
{
  assert(pager != nullptr);

  auto &extra = (ConfigurationExtraButtons &)pager->GetExtra();
  return extra.GetExpert();
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
 * to the menu (Back).
 */
static void
OnCloseClicked(WidgetDialog &dialog)
{
  if (pager->GetCurrentIndex() == 0)
    dialog.SetModalResult(mrOK);
  else
    pager->ClickPage(0);
}

static void
OnPageFlipped(WidgetDialog &dialog, TabMenuDisplay &menu)
{
  menu.OnPageFlipped();

  char buffer[128];
  const char *caption = menu.GetCaption(buffer, ARRAY_SIZE(buffer));
  if (caption == nullptr)
    caption = _("Configuration");
  dialog.SetCaption(caption);

  pager->SetCloseButtonCaption(pager->GetCurrentIndex() == 0
                               ? _("Close")
                               : _("Back"));
}

void dlgConfigurationShowModal()
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  /* Destroyed after the dialog, so Hide() can still return borrowed
     buttons. Left set, the pointer dangles and a later standalone
     panel crashes in BorrowExtraButton(). */
  AtScopeExit() {
    pager = nullptr;
  };

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Configuration"));

  pager = new ArrowPagerWidget(look.button,
                               [&dialog](){ OnCloseClicked(dialog); },
                               std::make_unique<ConfigurationExtraButtons>(look));

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
    OnPageFlipped(dialog, menu);
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

  /* Persist Expert only on OK. Missing UserLevel means beginner —
     write "1" when enabling Expert; remove the key when returning to
     beginner (do not leave UserLevel=0 cruft) (#1793). */
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

  if (dialog.GetChanged() || expert_changed)
    Profile::Save();
}
