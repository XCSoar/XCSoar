// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeatherDialog.hpp"
#include "NOAAList.hpp"
#include "RASPDialog.hpp"
#include "PCMetDialog.hpp"
#include "SkysightDialog.hpp"
#if 0
#include "MapOverlayWidget.hpp"
#endif
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/TabWidget.hpp"
#include "Widget/ButtonWidget.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Language/Language.hpp"
#include "Weather/Features.hpp"
#include "util/StaticString.hxx"

static int weather_page = 0;

static void
SetTitle(WndForm &form, const TabWidget &pager)
{
  StaticString<128> title;
  title.Format(_T("%s: %s"), _("Weather"),
               pager.GetButtonCaption(pager.GetCurrentIndex()));
  form.SetCaption(title);
}

void
ShowWeatherDialog(const TCHAR *page)
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

#ifdef HAVE_NOAA
  if (page != nullptr && StringIsEqual(page, _T("list")))
    start_page = widget.GetSize();

  widget.AddTab(CreateNOAAListWidget(), _("METAR and TAF"));
#endif

  if (page != nullptr && StringIsEqual(page, _T("rasp")))
    start_page = widget.GetSize();

  widget.AddTab(CreateRaspWidget(), _T("RASP"));

  if (page != nullptr && StringIsEqual(page, _T("Skysight")))
    start_page = widget.GetSize();

  widget.AddTab(CreateSkysightWidget(), _T("Skysight"));

#ifdef HAVE_PCMET
  if (page != nullptr && StringIsEqual(page, _T("pc_met")))
    start_page = widget.GetSize();

  widget.AddTab(CreatePCMetWidget(), _T("pc_met"));
#endif

#if 0
  /* The German DWD has terminated our access to georeferenced images,
     so this code is disabled for now, but will remain here;
     eventually, we should refactor the code to be generic, allowing
     arbitrary georeferenced images */

  if (page != nullptr && StringIsEqual(page, _T("overlay")))
    start_page = widget.GetSize();

  // TODO: better and translatable title?
  widget.AddTab(CreateWeatherMapOverlayWidget(), _T("Overlay"));
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
