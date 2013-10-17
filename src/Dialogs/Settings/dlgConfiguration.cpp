/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Dialogs/Dialogs.h"
#include "Dialogs/Message.hpp"
#include "Widget/DockWindow.hpp"
#include "Widget/ArrowPagerWidget.hpp"
#include "Widget/CreateWindowWidget.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "Form/TabMenuDisplay.hpp"
#include "Form/TabMenuData.hpp"
#include "Form/CheckBox.hpp"
#include "Form/Button.hpp"
#include "Form/LambdaActionListener.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Profile/Profile.hpp"
#include "LogFile.hpp"
#include "Util/Macros.hpp"
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

#ifdef HAVE_PCM_PLAYER
#include "Panels/AudioVarioConfigPanel.hpp"
#endif

#ifdef HAVE_TRACKING
#include "Panels/TrackingConfigPanel.hpp"
#endif

#ifdef HAVE_MODEL_TYPE
#include "Panels/ExperimentalConfigPanel.hpp"
#endif

#include <assert.h>

static unsigned current_page;

// TODO: eliminate global variables
static ArrowPagerWidget *pager;

static constexpr TabMenuGroup main_menu_captions[] = {
  { N_("Site Files"), },
  { N_("Map Display"), },
  { N_("Glide Computer"), },
  { N_("Gauges"), },
  { N_("Task Defaults"), },
  { N_("Look"), },
  { N_("Setup"), },
};

static constexpr TabMenuPage pages[] = {
  {N_("Site Files"), 0, CreateSiteConfigPanel },
  {N_("Orientation"), 1, CreateMapDisplayConfigPanel },
  {N_("Elements"), 1, CreateSymbolsConfigPanel },
  {N_("Waypoints"), 1, CreateWaypointDisplayConfigPanel },
  {N_("Terrain"), 1, CreateTerrainDisplayConfigPanel },
  {N_("Airspace"), 1, CreateAirspaceConfigPanel },
  {N_("Safety Factors"), 2, CreateSafetyFactorsConfigPanel },
  {N_("Glide Computer"), 2, CreateGlideComputerConfigPanel },
  {N_("Wind"), 2, CreateWindConfigPanel },
  {N_("Route"), 2, CreateRouteConfigPanel },
  {N_("Scoring"), 2, CreateScoringConfigPanel },
  {N_("FLARM, Other"), 3, CreateGaugesConfigPanel },
  {N_("Vario"), 3, CreateVarioConfigPanel },
#ifdef HAVE_PCM_PLAYER
  {N_("Audio Vario"), 3, CreateAudioVarioConfigPanel },
#endif
  {N_("Task Rules"), 4, CreateTaskRulesConfigPanel },
  {N_("Turnpoint Types"), 4, CreateTaskDefaultsConfigPanel },
  {N_("Language, Input"), 5, CreateInterfaceConfigPanel },
  {N_("Screen Layout"), 5, CreateLayoutConfigPanel },
  {N_("Pages"), 5, CreatePagesConfigPanel },
  {N_("InfoBox Sets"), 5, CreateInfoBoxesConfigPanel },
  {N_("Logger"), 6, CreateLoggerConfigPanel },
  {N_("Units"), 6, CreateUnitsConfigPanel },
  // Important: all pages after Units in this list must not have data fields that are
  // unit-dependent because they will be saved after their units may have changed.
  // ToDo: implement API that controls order in which pages are saved
  {N_("Time"), 6, CreateTimeConfigPanel },
#ifdef HAVE_TRACKING
  {N_("Tracking"), 6, CreateTrackingConfigPanel },
#endif
#ifdef HAVE_MODEL_TYPE
  {N_("Experimental Features"), 6, CreateExperimentalConfigPanel, },
#endif
};

class ConfigurationExtraButtons final
  : public NullWidget, ActionListener {
  enum Buttons {
    EXPERT,
  };

  struct Layout {
    PixelRect expert, button2, button1;

    Layout(const PixelRect &rc):expert(rc), button2(rc), button1(rc) {
      const unsigned height = rc.bottom - rc.top;
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
  WndButton button2, button1;
  bool borrowed2, borrowed1;

public:
  ConfigurationExtraButtons(const DialogLook &_look)
    :look(_look),
     button2(look.button), button1(look.button),
     borrowed2(false), borrowed1(false) {}

  WndButton &GetButton(unsigned number) {
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
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override {
    Layout layout(rc);

    WindowStyle style;
    style.Hide();
    style.TabStop();

    expert.Create(parent, look, _("Expert"),
                  layout.expert, style, *this, EXPERT);

    button2.Create(parent, _T(""), layout.button2, style);
    button1.Create(parent, _T(""), layout.button1, style);
  }

  virtual void Show(const PixelRect &rc) override {
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

  virtual void Hide() override {
    expert.FastHide();
    button2.FastHide();
    button1.FastHide();
  }

  virtual void Move(const PixelRect &rc) override {
    Layout layout(rc);
    expert.Move(layout.expert);
    button2.Move(layout.button2);
    button1.Move(layout.button1);
  }

private:
  void OnExpertClicked();

  /* virtual methods from ActionListener */
  virtual void OnAction(int id) override {
    switch (id) {
    case EXPERT:
      OnExpertClicked();
      break;
    }
  }
};

void
ConfigPanel::BorrowExtraButton(unsigned i, const TCHAR *caption,
                               void (*callback)())
{
  ConfigurationExtraButtons &extra =
    (ConfigurationExtraButtons &)pager->GetExtra();
  WndButton &button = extra.GetButton(i);
  button.SetCaption(caption);
  button.SetOnClickNotify(callback);
  button.Show();
}

void
ConfigPanel::ReturnExtraButton(unsigned i)
{
  ConfigurationExtraButtons &extra =
    (ConfigurationExtraButtons &)pager->GetExtra();
  WndButton &button = extra.GetButton(i);
  button.Hide();
}

static void
OnUserLevel(CheckBoxControl &control)
{
  const bool expert = control.GetState();
  CommonInterface::SetUISettings().dialog.expert = expert;
  Profile::Set(ProfileKeys::UserLevel, expert);

  /* force layout update */
  pager->PagerWidget::Move(pager->GetPosition());
}

inline void
ConfigurationExtraButtons::OnExpertClicked()
{
  OnUserLevel(expert);
}

/**
 * close dialog from menu page.  from content, goes to menu page
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

  TCHAR buffer[128];
  const TCHAR *caption = menu.GetCaption(buffer, ARRAY_SIZE(buffer));
  if (caption == nullptr)
    caption = _("Configuration");
  dialog.SetCaption(caption);
}

void dlgConfigurationShowModal()
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(look);

  auto on_close = MakeLambdaActionListener([&dialog](unsigned id) {
      OnCloseClicked(dialog);
    });

  pager = new ArrowPagerWidget(on_close, look.button,
                               new ConfigurationExtraButtons(look));

  TabMenuDisplay *menu = new TabMenuDisplay(*pager, look);
  pager->Add(new CreateWindowWidget([menu](ContainerWindow &parent,
                                           const PixelRect &rc,
                                           WindowStyle style) {
                                      style.TabStop();
                                      menu->Create(parent, rc, style);
                                      return menu;
                                    }));

  menu->InitMenu(pages, ARRAY_SIZE(pages),
                 main_menu_captions, ARRAY_SIZE(main_menu_captions));

  /* restore last selected menu item */
  menu->SetCursor(current_page);

  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Configuration"), pager);

  pager->SetPageFlippedCallback([&dialog, menu](){
      OnPageFlipped(dialog, *menu);
    });

  dialog.ShowModal();

  /* save page number for next time this dialog is opened */
  current_page = menu->GetCursor();

  bool changed = false;
  pager->Save(changed);
  if (changed) {
    Profile::Save();
    LogDebug(_T("Configuration: Changes saved"));
    if (require_restart)
      ShowMessageBox(_("Changes to configuration saved.  Restart XCSoar to apply changes."),
                  _T(""), MB_OK);
  }
}
