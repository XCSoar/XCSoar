/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "WeatherDialog.hpp"
#include "NOAAList.hpp"
#include "RASPDialog.hpp"
#include "PCMetDialog.hpp"
#include "SkysightDialog.hpp"
#include "MapOverlayWidget.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/TabWidget.hpp"
#include "Widget/ButtonWidget.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Language/Language.hpp"
#include "Weather/Features.hpp"
#include "Util/StaticString.hxx"

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
  WidgetDialog dialog(look);

  auto *close_button = new ButtonWidget(look.button, _("Close"),
                                        dialog, mrOK);

  TabWidget widget(TabWidget::Orientation::AUTO, close_button);
  widget.SetPageFlippedCallback([&dialog, &widget]() {
      SetTitle(dialog, widget);
    });

  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Status"), &widget);
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

#ifdef ENABLE_OPENGL
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
  dialog.StealWidget();

  /* save page number for next time this dialog is opened */
  weather_page = widget.GetCurrentIndex();
}
