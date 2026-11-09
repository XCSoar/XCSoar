// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/Dialogs.h"
#include "Widget/ArrowPagerWidget.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "ui/event/KeyCode.hpp"
#include "Form/CheckBox.hpp"
#include "Form/Button.hpp"
#include "Renderer/ButtonRenderer.hpp"
#include "Renderer/TextButtonRenderer.hpp"
#include "Screen/Layout.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "DataGlobals.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "Weather/Settings.hpp"
#include "util/StaticArray.hxx"
#include "util/StaticString.hxx"
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
#include "UtilsSettings.hpp"
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
#include "net/client/SoaringScoring/Account.hpp"
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

/**
 * A setting which is a switch in the list of its group, where a
 * page would be too much for it.
 */
struct ConfigToggle {
  /** an explanation of the setting */
  const char *help;

  bool (*get)() noexcept;

  /** Store the new state in the settings and in the profile. */
  void (*set)(bool value) noexcept;
};

/**
 * One page of the configuration: a panel which edits some settings,
 * or a #ConfigToggle in its place.
 */
struct ConfigPage {
  const char *caption;

  /** nullptr for a #ConfigToggle */
  std::unique_ptr<Widget> (*create)();

  /** An unavailable page is left out of the menu and the pager. */
  bool (*IsAvailable)() = nullptr;

  /** A switch in the group list; nullptr for a page. */
  const ConfigToggle *toggle = nullptr;

  /**
   * Is the service this page sets up in use?  The list of the group
   * says so next to the page; nullptr for a page which is no service.
   */
  bool (*is_active)() noexcept = nullptr;
};

/** The pages which one item of the menu leads to. */
struct ConfigGroup {
  const char *caption;

  /** Terminated by a page without a create function. */
  const ConfigPage *pages;
};

static constexpr ConfigPage files_pages[] = {
  { N_("Site Files"), CreateSiteConfigPanel },
  { nullptr, nullptr }
};

static constexpr ConfigPage map_pages[] = {
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

static constexpr ConfigPage computer_pages[] = {
  { N_("Glide Computer"), CreateGlideComputerConfigPanel },
  { N_("Route"), CreateRouteConfigPanel },
  { N_("Safety Factors"), CreateSafetyFactorsConfigPanel },
  { N_("Wind"), CreateWindConfigPanel },
  { nullptr, nullptr }
};

static constexpr ConfigPage gauge_pages[] = {
  { N_("Thermal Assistant"), CreateGaugesConfigPanel },
  { N_("Traffic"), CreateTrafficConfigPanel },
  { nullptr, nullptr }
};

static constexpr ConfigPage task_pages[] = {
  { N_("Scoring"), CreateScoringConfigPanel },
  { N_("Task Rules"), CreateTaskRulesConfigPanel },
  { N_("Turnpoint Types"), CreateTaskDefaultsConfigPanel },
  { nullptr, nullptr }
};

static constexpr ConfigPage look_pages[] = {
  { N_("InfoBox Sets"), CreateInfoBoxesConfigPanel },
  { N_("Layout"), CreateLayoutConfigPanel },
  { N_("Pages"), CreatePagesConfigPanel },
  { N_("Vario"), CreateVarioConfigPanel },
  { nullptr, nullptr }
};

#ifdef HAVE_HTTP

static bool
GetThermalInformationMap() noexcept
{
  return CommonInterface::GetComputerSettings().weather.enable_tim;
}

static void
SetThermalInformationMap(bool value) noexcept
{
  auto &settings = CommonInterface::SetComputerSettings().weather;

  /* the list of the group closes without a Save() of a page: write
     the profile here */
  if (Profile::Update(ProfileKeys::EnableThermalInformationMap,
                      settings.enable_tim, value))
    Profile::Save();
}

static constexpr ConfigToggle thermal_information_map_toggle{
  N_("Show thermal locations downloaded from Thermal Information Map (thermalmap.info)."),
  GetThermalInformationMap,
  SetThermalInformationMap,
};

#endif

/* the services are in use as soon as they are set up: a RASP file
   with fields, or an account */

static bool
IsRaspActive() noexcept
{
  const auto rasp = DataGlobals::GetRasp();
  return rasp != nullptr && rasp->GetItemCount() > 0;
}

#ifdef HAVE_HTTP

static bool
IsSkySightActive() noexcept
{
  return CommonInterface::GetComputerSettings().weather.skysight.IsDefined();
}

static bool
IsXCThermActive() noexcept
{
  return CommonInterface::GetComputerSettings()
    .weather.xctherm.credentials.IsDefined();
}

#endif

#ifdef HAVE_PCMET

static bool
IsPCMetActive() noexcept
{
  return CommonInterface::GetComputerSettings()
    .weather.pcmet.www_credentials.IsDefined();
}

#endif

static constexpr ConfigPage weather_pages[] = {
#ifdef HAVE_PCMET
  { "Flugwetter (pc_met)", CreatePCMetConfigPanel, nullptr, nullptr,
    IsPCMetActive },
#endif
  { "RASP", CreateRaspConfigPanel, nullptr, nullptr, IsRaspActive },
#ifdef HAVE_HTTP
  { "SkySight", CreateSkySightConfigPanel, nullptr, nullptr,
    IsSkySightActive },
  { N_("Thermal Information Map"), nullptr, nullptr,
    &thermal_information_map_toggle },
  { "XC Therm", CreateXCThermConfigPanel, nullptr, nullptr,
    IsXCThermActive },
#endif
  { nullptr, nullptr }
};

#ifdef HAVE_LIVETRACK24

static bool
IsLiveTrack24Active() noexcept
{
  return CommonInterface::GetComputerSettings()
    .tracking.livetrack24.enabled;
}

#endif

#ifdef HAVE_SKYLINES_TRACKING

static bool
IsSkyLinesActive() noexcept
{
  return CommonInterface::GetComputerSettings()
    .tracking.skylines.enabled;
}

#endif

static bool
IsWeGlideActive() noexcept
{
  return CommonInterface::GetComputerSettings().weglide.IsConfigured();
}

#ifdef HAVE_HTTP

static bool
IsSoaringScoringActive() noexcept
{
  return SoaringScoring::AccountConnected();
}

#endif

#ifdef HAVE_TRACKING

static bool
IsCloudActive() noexcept
{
  return CommonInterface::GetComputerSettings().tracking.cloud.enabled ==
    TriState::TRUE;
}

#endif

static constexpr ConfigPage online_pages[] = {
#ifdef HAVE_LIVETRACK24
  { "LiveTrack24", CreateLiveTrack24ConfigPanel, nullptr, nullptr,
    IsLiveTrack24Active },
#endif
#ifdef HAVE_SKYLINES_TRACKING
  { "SkyLines", CreateSkyLinesConfigPanel, nullptr, nullptr,
    IsSkyLinesActive },
#endif
  { "WeGlide", CreateWeGlideConfigPanel, nullptr, nullptr,
    IsWeGlideActive },
#ifdef HAVE_HTTP
  { "SoaringScoring", CreateSoaringScoringConfigPanel, nullptr, nullptr,
    IsSoaringScoringActive },
#endif
#ifdef HAVE_TRACKING
  { "XCSoar Cloud", CreateCloudConfigPanel, nullptr, nullptr,
    IsCloudActive },
#endif
  { nullptr, nullptr }
};

static constexpr ConfigPage setup_pages[] = {
#ifdef HAVE_PCM_PLAYER
  /* Before Units: audio vario deadband uses vertical-speed units. */
  { N_("Audio"), CreateAudioConfigPanel },
#endif
  { N_("Language, Input"), CreateInterfaceConfigPanel },
  { N_("Logger"), CreateLoggerConfigPanel },
  { N_("Units"), CreateUnitsConfigPanel },
  /* Important: all pages after Units in this list must not have data
     fields that are unit-dependent because they will be saved after
     their units may have changed.
     ToDo: implement API that controls order in which pages are saved */
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

static constexpr ConfigGroup groups[] = {
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

/**
 * What one page of the pager shows.  The pager holds the menu and
 * the pages of the configuration, nothing else: the arrows walk
 * through the pages, the list of a group is a dialog above the menu.
 */
struct PagerPage {
  /** The group of this page; nullptr on the menu. */
  const ConfigGroup *group;

  /** The page of the configuration; nullptr on the menu. */
  const ConfigPage *page;

  /** The index of the first page of #group in the pager. */
  unsigned first;
};

static bool
PageAvailable(const ConfigPage &page) noexcept
{
  return page.IsAvailable == nullptr || page.IsAvailable();
}

/** Does the group have a list of its pages, or only one page? */
static constexpr bool
HasList(const ConfigGroup &group) noexcept
{
  return group.pages[1].caption != nullptr;
}

/** The item of the menu the cursor rests on, kept for the next time. */
static unsigned current_item;

/** Menu page to show when Configuration opens. */
static std::unique_ptr<Widget> (*initial_page)() = nullptr;

// TODO: eliminate global variables
static ArrowPagerWidget *pager;

static StaticArray<PagerPage, 48u> pager_pages;


static unsigned
FindConfigPage(std::unique_ptr<Widget> (*load)()) noexcept
{
  unsigned index = 1;
  for (const auto &group : groups) {
    for (auto page = group.pages; page->caption != nullptr; ++page) {
      if (page->toggle != nullptr || !PageAvailable(*page))
        continue;
      if (page->create == load)
        return index;
      ++index;
    }
  }

  assert(false);
  return 0;
}

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
ConfigurationExtraButtons::Apply(const PixelRect &outer) noexcept
{
  if (!expert.IsDefined())
    return;

  position = outer;

  /* the controls keep the gap to the edges that the buttons have
     between each other */
  PixelRect rc = outer;
  rc.Grow(-(int)ButtonFrameRenderer::GetEdgeMargin(rc));

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
 * Show the pages of one group as a list above the menu, and open the
 * page the user picks.
 *
 * @param first the index of the first page of the group in the pager
 * @param cursor the page the cursor starts on, counted in the group
 */
static void
ShowGroupList(const ConfigGroup &group, unsigned first, unsigned cursor)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, gettext(group.caption));

  auto _list = std::make_unique<GroupedListWidget>(look);
  GroupedListWidget &list = *_list;
  list.AddGroup();

  int picked = -1;
  unsigned i = first;
  for (const ConfigPage *page = group.pages; page->caption != nullptr;
       ++page) {
    if (!PageAvailable(*page))
      continue;

    if (page->toggle != nullptr) {
      /* a switch in the list, which acts right away */
      const ConfigToggle &toggle = *page->toggle;
      const unsigned item = list.GetItemCount();
      list.AddItem(gettext(page->caption), [&list, &toggle, item](){
        toggle.set(list.IsItemChecked(item));
      }, {.toggle = true,
          .checked = toggle.get(),
          .help = gettext(toggle.help)});
      continue;
    }

    GroupedListWidget::ItemOptions options{.chevron = true};

    /* a service says whether it is in use */
    if (page->is_active != nullptr) {
      if (page->is_active()) {
        options.badge = C_("Badge", "active");
        options.badge_style = GroupedListWidget::BadgeStyle::SUCCESS;
      } else
        options.badge = C_("Badge", "off");
    }

    list.AddItem(gettext(page->caption), [&dialog, &picked, i](){
      picked = i;
      dialog.SetModalResult(mrOK);
    }, options);
    ++i;
  }

  list.SetCursorIndex(cursor);

  dialog.FinishPreliminary(std::move(_list));
  dialog.AddButton(_("Back"), mrCancel);
  dialog.ShowModal();

  if (picked >= 0)
    pager->ClickPage(picked);
}

/**
 * Close on the menu page commits (mrOK).  On a page of the
 * configuration, return to the menu, and to the list of the group
 * the page belongs to (Back).
 */
static void
OnCloseClicked(WidgetDialog &dialog)
{
  const unsigned i = pager->GetCurrentIndex();
  if (i == 0) {
    dialog.SetModalResult(mrOK);
    return;
  }

  const PagerPage &current = pager_pages[i];
  unsigned cursor = 0;
  if (current.group != nullptr) {
    for (const ConfigPage *page = current.group->pages;
         page != current.page; ++page)
      if (PageAvailable(*page))
        ++cursor;
  }

  if (pager->ClickPage(0) && current.group != nullptr &&
      HasList(*current.group))
    ShowGroupList(*current.group, current.first, cursor);
}

static void
OnPageFlipped(WidgetDialog &dialog, GroupedListWidget &menu)
{
  const unsigned i = pager->GetCurrentIndex();
  const PagerPage &current = pager_pages[i];

  StaticString<128> caption;
  if (current.page == nullptr)
    caption = _("Configuration");
  else if (HasList(*current.group))
    caption.Format("%s > %s", gettext(current.group->caption),
                   gettext(current.page->caption));
  else
    /* the only page of its group: its name says it all */
    caption = gettext(current.page->caption);
  dialog.SetCaption(caption);

  pager->SetCloseButtonCaption(i == 0 ? _("Close") : _("Back"));

  /* the arrows reach a page without the menu: let its cursor follow,
     so that Back returns to the item of this page */
  if (current.group != nullptr)
    menu.SetCursorIndex(current.group - groups);
}

/**
 * Add the item of one group to the menu, and its pages to the pager.
 */
static void
AddGroup(GroupedListWidget &menu, const ConfigGroup &group) noexcept
{
  const unsigned first = pager->GetSize();

  menu.AddItem(gettext(group.caption), [&group, first](){
    if (HasList(group))
      ShowGroupList(group, first, 0);
    else
      pager->ClickPage(first);
  }, {.chevron = true});

  for (const ConfigPage *page = group.pages; page->caption != nullptr;
       ++page) {
    /* a switch lives in the list of the group, not in the pager */
    if (page->toggle != nullptr || !PageAvailable(*page))
      continue;

    pager_pages.append({&group, page, first});
    pager->Add(page->create());
  }
}

void
dlgConfigurationShowPage(std::unique_ptr<Widget> (*page)())
{
  assert(page != nullptr);

  /* Already inside Configuration: this is one of its pages. */
  if (pager != nullptr) {
    pager->SetCurrent(FindConfigPage(page), false);
    return;
  }

  initial_page = page;
  AtScopeExit() {
    initial_page = nullptr;
  };
  SystemConfiguration();
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
                               std::make_unique<ConfigurationExtraButtons>(
                                 look));

  auto _menu = std::make_unique<GroupedListWidget>(look);
  auto &menu = *_menu;
  pager_pages.clear();
  pager_pages.append({nullptr, nullptr, 0});
  pager->Add(std::move(_menu));

  menu.AddGroup();
  for (const ConfigGroup &group : groups)
    AddGroup(menu, group);

  /* restore last selected menu item */
  menu.SetCursorIndex(current_item);

  pager->SetPageFlippedCallback([&dialog, &menu](){
    OnPageFlipped(dialog, menu);
  });

  dialog.FinishPreliminary(pager);

  if (initial_page != nullptr) {
    pager->SetCurrent(FindConfigPage(initial_page), false);
    OnPageFlipped(dialog, menu);
  }

  /* Esc on a settings panel returns to the menu (same as Back);
     on the menu itself, leave Esc to cancel the dialog. */
  dialog.SetKeyDownFunction([&dialog](unsigned key_code) {
    if (key_code != KEY_ESCAPE || pager->GetCurrentIndex() == 0)
      return false;

    OnCloseClicked(dialog);
    return true;
  });

  const int result = dialog.ShowModal();

  /* save the menu item for next time this dialog is opened */
  if (menu.GetCursorIndex() >= 0)
    current_item = menu.GetCursorIndex();

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
