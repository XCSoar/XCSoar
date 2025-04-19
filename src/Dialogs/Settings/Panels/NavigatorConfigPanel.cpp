// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NavigatorConfigPanel.hpp"
#include "Gauge/NavigatorSettings.hpp"
#include "Gauge/NavigatorWidget.hpp"
#include "Profile/Keys.hpp"
#include "Interface.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "MainWindow.hpp"

enum ControlIndex {
  NavigatorWidgetLite1LHeight,
  NavigatorWidgetLite2LHeight,
  NavigatorWidgetHeight,
  NavigatorWidgetDetailedHeight
};


class NavigatorConfigPanel final : public RowFormWidget {
public:
  NavigatorConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};


void
NavigatorConfigPanel::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  const UISettings &ui_settings = CommonInterface::GetUISettings();

  RowFormWidget::Prepare(parent, rc);

  AddInteger(
    _("Navigator lite one line Height"),
    _("Select the height of the navigator lite topwidget (percentage of the main window).\n"
      "This widget is set under the settings menu: Look / Pages / Top Area\n"
      "Warning: an unsuitable height of the navigator widget could lead to a bad presentation of its included flight informations (e.g. name of waypoint, times, ...).\n"
      "to be setted accordingly to the size of the screen device.\n"
      "default value: 6%"),
    _T("%u %%"), _T("%u"), 1, 40, 1,
    (unsigned)ui_settings.navigator.navigator_lite_1_line_height); 


  AddInteger(
    _("Navigator lite two lines Height"),
    _("Select the height of the navigator lite topwidget (percentage of the main window).\n"
      "This widget is set under the settings menu: Look / Pages / Top Area\n"
      "Warning: an unsuitable height of the navigator widget could lead to a bad presentation of its included flight informations (e.g. name of waypoint, times, ...).\n"
      "to be setted accordingly to the size of the screen device.\n"
      "default value: 8%"),
    _T("%u %%"), _T("%u"), 1, 40, 1,
    (unsigned)ui_settings.navigator.navigator_lite_2_lines_height); 

  AddInteger(
    _("Navigator Height"),
    _("Select the height of the navigator topwidget (percentage of the main window).\n"
      "This widget is set under the settings menu: Look / Pages / Top Area\n"
      "Warning: an unsuitable height of the navigator widget could lead to a bad presentation of its included flight informations (e.g. name of waypoint, times, ...).\n"
      "to be setted accordingly to the size of the screen device.\n"
      "default value: 9%"),
    _T("%u %%"), _T("%u"), 1, 40, 1,
    (unsigned)ui_settings.navigator.navigator_height);

  AddInteger(
    _("Navigator detailled Height"),
    _("Select the height of the navigator detailled topwidget (percentage of the main window).\n"
      "This widget is set under the settings menu: Look / Pages / Top Area\n"
      "Warning: an unsuitable height of the navigator widget could lead to a bad presentation of its included flight informations (e.g. name of waypoint, times, ...).\n"
      "to be setted accordingly to the size of the screen device.\n"
      "default value: 11%"),
    _T("%u %%"), _T("%u"), 1, 40, 1,
    (unsigned)ui_settings.navigator.navigator_detailed_height);

}

bool
NavigatorConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  UISettings &ui_settings = CommonInterface::SetUISettings();

  if ((changed |= SaveValueInteger(NavigatorWidgetHeight, ProfileKeys::NavigatorHeight,
                                   ui_settings.navigator.navigator_height)))
    CommonInterface::main_window->ReinitialiseLayout();
  if ((changed |= SaveValueInteger(NavigatorWidgetLite1LHeight, ProfileKeys::NavigatorLite1LHeight,
                                   ui_settings.navigator.navigator_lite_1_line_height)))
    CommonInterface::main_window->ReinitialiseLayout();
  if ((changed |= SaveValueInteger(NavigatorWidgetLite2LHeight, ProfileKeys::NavigatorLite2LHeight,
                                   ui_settings.navigator.navigator_lite_2_lines_height)))
    CommonInterface::main_window->ReinitialiseLayout();
  if ((changed |= SaveValueInteger(NavigatorWidgetDetailedHeight, ProfileKeys::NavigatorDetailedHeight,
                                   ui_settings.navigator.navigator_detailed_height)))
    CommonInterface::main_window->ReinitialiseLayout();

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateNavigatorConfigPanel()
{
  return std::make_unique<NavigatorConfigPanel>();
}
