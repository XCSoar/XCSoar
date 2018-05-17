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

#include "NOAADetails.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_NOAA

#include "Dialogs/WidgetDialog.hpp"
#include "Widget/LargeTextWidget.hpp"
#include "Weather/NOAAGlue.hpp"
#include "Weather/NOAAStore.hpp"
#include "Weather/NOAAUpdater.hpp"
#include "Weather/ParsedMETAR.hpp"
#include "Weather/NOAAFormatter.hpp"
#include "UIGlobals.hpp"

class NOAADetailsWidget final : public LargeTextWidget, ActionListener {
  enum Buttons {
    UPDATE,
    REMOVE,
  };

  WndForm &dialog;
  NOAAStore::iterator station_iterator;

public:
  NOAADetailsWidget(WndForm &_dialog, NOAAStore::iterator _station)
    :LargeTextWidget(_dialog.GetLook()), dialog(_dialog),
    station_iterator(_station) {}

  void CreateButtons(WidgetDialog &buttons);

private:
  void Update();
  void UpdateClicked();
  void RemoveClicked();

  /* virtual methods from class Widget */
  virtual void Show(const PixelRect &rc) override;

  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) override;
};

void
NOAADetailsWidget::CreateButtons(WidgetDialog &buttons)
{
  buttons.AddButton(_("Update"), *this, UPDATE);
  buttons.AddButton(_("Remove"), *this, REMOVE);
}

void
NOAADetailsWidget::Update()
{
  tstring metar_taf = _T("");

  NOAAFormatter::Format(*station_iterator, metar_taf);

  SetText(metar_taf.c_str());

  StaticString<100> caption;
  caption.Format(_T("%s: "), _("METAR and TAF"));

  if (!station_iterator->parsed_metar_available ||
      !station_iterator->parsed_metar.name_available)
    caption += station_iterator->GetCodeT();
  else
    caption.AppendFormat(_T("%s (%s)"),
                         station_iterator->parsed_metar.name.c_str(),
                         station_iterator->GetCodeT());

  dialog.SetCaption(caption);
}

inline void
NOAADetailsWidget::UpdateClicked()
{
  DialogJobRunner runner(dialog.GetMainWindow(), dialog.GetLook(),
                         _("Download"), true);
  NOAAUpdater::Update(*station_iterator, runner);
  Update();
}

inline void
NOAADetailsWidget::RemoveClicked()
{
  StaticString<256> tmp;
  tmp.Format(_("Do you want to remove station %s?"),
             station_iterator->GetCodeT());

  if (ShowMessageBox(tmp, _("Remove"), MB_YESNO) == IDNO)
    return;

  noaa_store->erase(station_iterator);
  noaa_store->SaveToProfile();

  dialog.SetModalResult(mrOK);
}

void
NOAADetailsWidget::Show(const PixelRect &rc)
{
  LargeTextWidget::Show(rc);
  Update();
}

void
NOAADetailsWidget::OnAction(int id)
{
  switch (id) {
  case UPDATE:
    UpdateClicked();
    break;

  case REMOVE:
    RemoveClicked();
    break;
  }
}

void
dlgNOAADetailsShowModal(NOAAStore::iterator iterator)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(look);
  NOAADetailsWidget widget(dialog, iterator);
  dialog.CreateFull(UIGlobals::GetMainWindow(), _("METAR and TAF"), &widget);
  widget.CreateButtons(dialog);
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
  dialog.StealWidget();
}

#else

#include "Dialogs/Message.hpp"

void
dlgNOAADetailsShowModal(unsigned station_index)
{
  ShowMessageBox(_("This function is not available on your platform yet."),
              _("Error"), MB_OK);
}
#endif
