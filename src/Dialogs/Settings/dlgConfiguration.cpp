// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/Dialogs.h"
#include "Dialogs/Message.hpp"
#include "Widget/ArrowPagerWidget.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "ui/event/KeyCode.hpp"
#include "Form/Button.hpp"
#include "Renderer/ButtonRenderer.hpp"
#include "Screen/Layout.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Weather/Settings.hpp"
#include "util/StaticArray.hxx"
#include "util/StaticString.hxx"
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

  const ConfigToggle *toggle = nullptr;
};

/** The pages which one item of the menu leads to. */
struct ConfigGroup {
  const char *caption;

  /** terminated by a page without a caption */
  const ConfigPage *pages;
};

static constexpr ConfigPage files_pages[] = {
  { N_("Site Files"), CreateSiteConfigPanel },
  { nullptr, nullptr }
};

static constexpr ConfigPage map_pages[] = {
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

static constexpr ConfigPage computer_pages[] = {
  { N_("Safety Factors"), CreateSafetyFactorsConfigPanel },
  { N_("Glide Computer"), CreateGlideComputerConfigPanel },
  { N_("Wind"), CreateWindConfigPanel },
  { N_("Route"), CreateRouteConfigPanel },
  { N_("Scoring"), CreateScoringConfigPanel },
  { nullptr, nullptr }
};

static constexpr ConfigPage gauge_pages[] = {
  { N_("FLARM, Other"), CreateGaugesConfigPanel },
  { N_("Vario"), CreateVarioConfigPanel },
#ifdef HAVE_PCM_PLAYER
  { N_("Audio Vario"), CreateAudioVarioConfigPanel },
#endif
  { nullptr, nullptr }
};

static constexpr ConfigPage task_pages[] = {
  { N_("Task Rules"), CreateTaskRulesConfigPanel },
  { N_("Turnpoint Types"), CreateTaskDefaultsConfigPanel },
  { nullptr, nullptr }
};

static constexpr ConfigPage look_pages[] = {
  { N_("Language, Input"), CreateInterfaceConfigPanel },
  { N_("Display"), CreateDisplayConfigPanel },
  { N_("Layout"), CreateLayoutConfigPanel },
  { N_("Pages"), CreatePagesConfigPanel },
  { N_("InfoBox Sets"), CreateInfoBoxesConfigPanel },
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

static constexpr ConfigPage weather_pages[] = {
#ifdef HAVE_HTTP
  { N_("Thermal Information Map"), nullptr,
    &thermal_information_map_toggle },
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

static constexpr ConfigPage setup_pages[] = {
  { N_("Logger"), CreateLoggerConfigPanel },
  { N_("Units"), CreateUnitsConfigPanel },
  /* Important: all pages after Units in this list must not have data
     fields that are unit-dependent because they will be saved after
     their units may have changed.  ToDo: implement API that controls
     order in which pages are saved */
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

static constexpr ConfigGroup groups[] = {
  { N_("Site Files"), files_pages },
  { N_("Map Display"), map_pages },
  { N_("Glide Computer"), computer_pages },
  { N_("Gauges"), gauge_pages },
  { N_("Task Defaults"), task_pages },
  { N_("Look"), look_pages },
  { N_("Weather"), weather_pages },
  { NC_("Menu", "Setup"), setup_pages },
};

/**
 * What one page of the pager shows.  The pager holds the menu and
 * the pages of the configuration, nothing else: the arrows walk
 * through the pages, the list of a group is a dialog above the menu.
 */
struct PagerPage {
  /** the group of this page; nullptr on the menu */
  const ConfigGroup *group;

  /** the page of the configuration; nullptr on the menu */
  const ConfigPage *page;

  /** the index of the first page of #group in the pager */
  unsigned first;
};

/** does the group have a list of its pages, or only one page? */
static constexpr bool
HasList(const ConfigGroup &group) noexcept
{
  return group.pages[1].caption != nullptr;
}

/** the item of the menu the cursor rests on, kept for the next time */
static unsigned current_item;

// TODO: eliminate global variables
static ArrowPagerWidget *pager;

static StaticArray<PagerPage, 48u> pager_pages;

class ConfigurationExtraButtons final
  : public NullWidget {
  struct Layout {
    PixelRect button2, button1;

    Layout(const PixelRect &_rc) {
      /* the controls keep the gap to the edges that the buttons have
         between each other */
      PixelRect rc = _rc;
      rc.Grow(-(int)ButtonFrameRenderer::GetEdgeMargin(rc));
      button2 = button1 = rc;

      const unsigned height = rc.GetHeight();
      const unsigned max_control_height = ::Layout::GetMaximumControlHeight();

      if (height >= 2 * max_control_height) {
        button1.top = button2.bottom = rc.bottom - max_control_height;
        button2.top = button2.bottom - max_control_height;
      } else {
        button2.right = button1.left = unsigned(rc.left + rc.right) / 2;
      }
    }
  };

  const DialogLook &look;

  Button button2, button1;
  bool borrowed2, borrowed1;

public:
  ConfigurationExtraButtons(const DialogLook &_look)
    :look(_look),
     borrowed2(false), borrowed1(false) {}

  /** does a page show one of the buttons at the moment? */
  bool HasButtons() const noexcept {
    return borrowed2 || borrowed1;
  }

  void Borrow(unsigned i, const char *caption,
              std::function<void()> callback) noexcept {
    Button &button = GetButton(i);
    button.SetCaption(caption);
    button.SetCallback(std::move(callback));
    button.Show();
    GetBorrowed(i) = true;
  }

  void Return(unsigned i) noexcept {
    GetButton(i).Hide();
    GetBorrowed(i) = false;
  }

private:
  Button &GetButton(unsigned number) noexcept {
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

  bool &GetBorrowed(unsigned number) noexcept {
    switch (number) {
    case 1:
      return borrowed1;

    case 2:
      return borrowed2;

    default:
      assert(false);
      gcc_unreachable();
    }
  }

protected:
  /* virtual methods from Widget */
  PixelSize GetMinimumSize() const noexcept override {
    /* no room while no page borrows a button: the pager leaves the
       row out then */
    return {
      ::Layout::GetMaximumControlHeight() * 2,
      HasButtons() ? ::Layout::GetMaximumControlHeight() : 0u,
    };
  }

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    Layout layout(rc);

    WindowStyle style;
    style.Hide();
    style.TabStop();

    button2.Create(parent, look.button, "", layout.button2, style);
    button1.Create(parent, look.button, "", layout.button1, style);
  }

  void Show(const PixelRect &rc) noexcept override {
    /* the pager gives no room while no button is borrowed, and a
       window cannot be moved into none */
    if (rc.GetHeight() == 0)
      return;

    Layout layout(rc);

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
    button2.FastHide();
    button1.FastHide();
  }

  void Move(const PixelRect &rc) noexcept override {
    if (rc.GetHeight() == 0)
      return;

    Layout layout(rc);
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
  extra.Borrow(i, caption, std::move(callback));
}

void
ConfigPanel::ReturnExtraButton(unsigned i)
{
  ConfigurationExtraButtons &extra =
    (ConfigurationExtraButtons &)pager->GetExtra();
  extra.Return(i);
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

    list.AddItem(gettext(page->caption), [&dialog, &picked, i](){
      picked = i;
      dialog.SetModalResult(mrOK);
    }, {.chevron = true});
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
  if (pager->ClickPage(0) && HasList(*current.group))
    ShowGroupList(*current.group, current.first,
                  current.page - current.group->pages);
}

/**
 * @param extra_row does the layout of the pager hold the row of the
 * extra buttons?
 */
static void
OnPageFlipped(WidgetDialog &dialog, GroupedListWidget &menu,
              bool &extra_row)
{
  const unsigned i = pager->GetCurrentIndex();
  const PagerPage &current = pager_pages[i];

  /* the row of the extra buttons comes and goes with the page which
     borrows them */
  const bool need_extra_row =
    ((const ConfigurationExtraButtons &)pager->GetExtra()).HasButtons();
  if (need_extra_row != extra_row) {
    extra_row = need_extra_row;

    /* the dialog has no buttons of its own: the whole client area is
       the pager's.  Its GetPosition() is only the page inside it */
    pager->Move(dialog.GetClientAreaWindow().GetClientRect());
  }

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

  for (const ConfigPage *page = group.pages; page->caption != nullptr; ++page) {
    /* a switch lives in the list of the group, not in the pager */
    if (page->toggle != nullptr)
      continue;

    pager_pages.append({&group, page, first});
    pager->Add(page->create());
  }
}

void dlgConfigurationShowModal()
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Configuration"));

  pager = new ArrowPagerWidget(look.button,
                               [&dialog](){ OnCloseClicked(dialog); },
                               std::make_unique<ConfigurationExtraButtons>(look));

  auto _menu = std::make_unique<GroupedListWidget>(look);
  auto &menu = *_menu;
  pager_pages.clear();
  pager_pages.append({nullptr, nullptr, 0});
  pager->Add(std::move(_menu));

  menu.AddGroup();
  for (const ConfigGroup &group : groups)
    AddGroup(menu, group);

  /* a page lays itself out again whenever it is shown, and reads the
     user level then */
  menu.AddGroup();
  const unsigned expert_item = menu.GetItemCount();
  menu.AddItem(_("Expert"), [&menu, expert_item](){
    CommonInterface::SetUISettings().dialog.expert =
      menu.IsItemChecked(expert_item);
  }, {.toggle = true,
      .toggle_hit_area = GroupedListWidget::ToggleHitArea::ROW,
      .checked = CommonInterface::GetUISettings().dialog.expert,
      .help = _("Show the advanced settings, which are hidden otherwise.")});

  /* restore last selected menu item */
  menu.SetCursorIndex(current_item);

  bool extra_row = false;
  pager->SetPageFlippedCallback([&dialog, &menu, &extra_row](){
    OnPageFlipped(dialog, menu, extra_row);
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

  /* save the menu item for next time this dialog is opened */
  if (menu.GetCursorIndex() >= 0)
    current_item = menu.GetCursorIndex();

  /* Persist Expert only on OK. Missing UserLevel means beginner:
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

  if (dialog.GetChanged() || expert_changed) {
    Profile::Save();
    if (require_restart)
      ShowMessageBox(_("Changes to configuration saved. Restart XCSoar to apply changes."),
                  "", MB_OK);
  }
}
