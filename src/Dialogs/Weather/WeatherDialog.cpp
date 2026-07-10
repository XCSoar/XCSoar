// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeatherDialog.hpp"
#include "NOAAList.hpp"
#include "RASPDialog.hpp"
#include "PCMetDialog.hpp"
#include "Dialogs/Settings/Panels/PCMetConfigPanel.hpp"
#ifdef HAVE_HTTP
#include "XCThermDialog.hpp"
#include "WeatherCredentialGateWidget.hpp"
#include "Dialogs/Settings/Panels/XCThermConfigPanel.hpp"
#endif
#include "Dialogs/Settings/Panels/WeatherConfigPanel.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_HTTP
#include "SkysightDialog.hpp"
#endif
#if 0
#include "MapOverlayWidget.hpp"
#endif
#include "Widget/TextWidget.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/TabWidget.hpp"
#include "Widget/ButtonWidget.hpp"
#ifdef HAVE_EDL
#include "EdlSettingsWidget.hpp"
#endif
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "util/StaticString.hxx"

static int weather_page = 0;

#ifdef HAVE_HTTP
static std::unique_ptr<Widget>
CreateXCThermTabWidget() noexcept
{
  return CreateWeatherCredentialGateWidget(
    []() {
      return CommonInterface::GetComputerSettings()
        .weather.xctherm.credentials.IsDefined();
    },
    CreateXCThermConfigPanel,
    CreateXCThermMainWidget);
}
#endif

#ifdef HAVE_PCMET
static std::unique_ptr<Widget>
CreatePCMetTabWidget() noexcept
{
  return CreateWeatherCredentialGateWidget(
    []() {
      return CommonInterface::GetComputerSettings()
        .weather.pcmet.www_credentials.IsDefined();
    },
    CreatePCMetConfigPanel,
    CreatePCMetMainWidget);
}
#endif

#ifndef HAVE_EDL
class EDLUnavailableWidget final : public TextWidget {
  const char *text;

public:
  explicit EDLUnavailableWidget(const char *_text) noexcept
    :text(_text) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    TextWidget::Prepare(parent, rc);
    SetText(text);
  }
};

static std::unique_ptr<Widget>
CreateEDLUnavailableWidget() noexcept
{
  return std::make_unique<EDLUnavailableWidget>(
    _("EDL weather is not available because this build has no OpenGL renderer."));
}
#endif

static void
SetTitle(WndForm &form, const TabWidget &pager)
{
  StaticString<128> title;
  title.Format("%s: %s", _("Weather"),
               pager.GetButtonCaption(pager.GetCurrentIndex()));
  form.SetCaption(title);
}

void
ShowWeatherDialog(const char *page)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  TWidgetDialog<TabWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           look, _("Status"));

  dialog.SetWidget(TabWidget::Orientation::AUTO,
                   std::make_unique<ButtonWidget>(look.button, _("Close"),
                                                  dialog.MakeModalResultCallback(mrOK)));

  auto &widget = dialog.GetWidget();
  widget.SetPageFlippedCallback([&dialog, &widget]() {
      SetTitle(dialog, widget);
    });

  dialog.PrepareWidget();

  int start_page = -1;

  /* setup tabs */

#ifdef HAVE_HTTP
  if (page != nullptr && StringIsEqual(page, "skysight"))
    start_page = widget.GetSize();

  widget.AddTab(CreateSkysightWidget(), "SkySight");
#endif

#ifdef HAVE_NOAA
  if (page != nullptr && StringIsEqual(page, "list"))
    start_page = widget.GetSize();

  widget.AddTab(CreateNOAAListWidget(), _("METAR and TAF"));
#endif

#ifdef HAVE_HTTP
  if (page != nullptr && StringIsEqual(page, "xctherm"))
    start_page = widget.GetSize();

  widget.AddTab(CreateXCThermTabWidget(), "XCTherm");
#endif

  if (page != nullptr && StringIsEqual(page, "rasp"))
    start_page = widget.GetSize();

  widget.AddTab(CreateRaspWidget(), "RASP");

  if (page != nullptr && StringIsEqual(page, "edl"))
    start_page = widget.GetSize();

#ifdef HAVE_EDL
  widget.AddTab(CreateEdlSettingsWidget(), "EDL");
#else
  widget.AddTab(CreateEDLUnavailableWidget(), "EDL");
#endif

#ifdef HAVE_PCMET
  if (page != nullptr && StringIsEqual(page, "pc_met"))
    start_page = widget.GetSize();

  widget.AddTab(CreatePCMetTabWidget(), "Flugwetter");
#endif

#if 0
  /* The German DWD has terminated our access to georeferenced images,
     so this code is disabled for now, but will remain here;
     eventually, we should refactor the code to be generic, allowing
     arbitrary georeferenced images */

  if (page != nullptr && StringIsEqual(page, "overlay"))
    start_page = widget.GetSize();

  // TODO: better and translatable title?
  widget.AddTab(CreateWeatherMapOverlayWidget(), "Overlay");
#endif

  /* restore previous page */

  if (start_page != -1)
    weather_page = start_page;

  widget.SetCurrent(weather_page);

  SetTitle(dialog, widget);

  dialog.ShowModal();

  /* save page number for next time this dialog is opened */
  weather_page = widget.GetCurrentIndex();
}
