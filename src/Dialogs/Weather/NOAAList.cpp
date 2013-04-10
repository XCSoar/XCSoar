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

#include "WeatherDialogs.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Language/Language.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_NOAA

#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Widget/ListWidget.hpp"
#include "Weather/NOAAGlue.hpp"
#include "Weather/NOAAStore.hpp"
#include "Weather/NOAAUpdater.hpp"
#include "Weather/METAR.hpp"
#include "Util/TrivialArray.hpp"
#include "Compiler.h"
#include "Renderer/NOAAListRenderer.hpp"

struct NOAAListItem
{
  StaticString<5> code;
  NOAAStore::iterator iterator;

  bool operator<(const NOAAListItem &i2) const {
    return _tcscmp(code, i2.code) == -1;
  }
};

class NOAAListWidget final
  : public ListWidget, private ActionListener {
  enum Buttons {
    DETAILS,
    ADD,
    UPDATE,
    REMOVE,
  };

  WndButton *details_button, *add_button, *update_button, *remove_button;

  TrivialArray<NOAAListItem, 20> stations;

public:
  void CreateButtons(WidgetDialog &dialog);

private:
  void UpdateList();

  void OpenDetails(unsigned index);
  void DetailsClicked();
  void AddClicked();
  void UpdateClicked();
  void RemoveClicked();

public:
  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;
  virtual void Unprepare() override;

protected:
  /* virtual methods from ListItemRenderer */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) override;

  /* virtual methods from ListCursorHandler */
  virtual bool CanActivateItem(unsigned index) const override {
    return true;
  }

  virtual void OnActivateItem(unsigned index) override;

private:
  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) override;
};

void
NOAAListWidget::CreateButtons(WidgetDialog &dialog)
{
  details_button = dialog.AddButton(_("Details"), *this, DETAILS);
  add_button = dialog.AddButton(_("Add"), *this, ADD);
  update_button = dialog.AddButton(_("Update"), *this, UPDATE);
  remove_button = dialog.AddButton(_("Remove"), *this, REMOVE);
}

void
NOAAListWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc, NOAAListRenderer::GetHeight(look));
  UpdateList();
}

void
NOAAListWidget::Unprepare()
{
  DeleteWindow();
}

void
NOAAListWidget::UpdateList()
{
  stations.clear();

  for (auto i = noaa_store->begin(), end = noaa_store->end(); i != end; ++i) {
    NOAAListItem item;
    item.code = i->GetCodeT();
    item.iterator = i;
    stations.push_back(item);
  }

  std::sort(stations.begin(), stations.end());

  ListControl &list = GetList();
  list.SetLength(stations.size());
  list.Invalidate();

  const bool empty = stations.empty(), full = stations.full();
  add_button->SetEnabled(!full);
  update_button->SetEnabled(!empty);
  remove_button->SetEnabled(!empty);
  details_button->SetEnabled(!empty);
}

void
NOAAListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned index)
{
  assert(index < stations.size());

  NOAAListRenderer::Draw(canvas, rc, *stations[index].iterator,
                         UIGlobals::GetDialogLook());
}

inline void
NOAAListWidget::AddClicked()
{
  TCHAR code[5] = _T("");
  if (!TextEntryDialog(code, 5, _("Airport ICAO code")))
    return;

  if (_tcslen(code) != 4) {
    ShowMessageBox(_("Please enter the FOUR letter code of the desired station."),
                _("Error"), MB_OK);
    return;
  }

  if (!NOAAStore::IsValidCode(code)) {
    ShowMessageBox(_("Please don't use special characters in the four letter code of the desired station."),
                  _("Error"), MB_OK);
    return;
  }

  NOAAStore::iterator i = noaa_store->AddStation(code);
  noaa_store->SaveToProfile();

  DialogJobRunner runner(UIGlobals::GetMainWindow(),
                         UIGlobals::GetDialogLook(),
                         _("Download"), true);

  NOAAUpdater::Update(*i, runner);

  UpdateList();
}

inline void
NOAAListWidget::UpdateClicked()
{
  DialogJobRunner runner(UIGlobals::GetMainWindow(),
                         UIGlobals::GetDialogLook(),
                         _("Download"), true);
  NOAAUpdater::Update(*noaa_store, runner);
  UpdateList();
}

inline void
NOAAListWidget::RemoveClicked()
{
  unsigned index = GetList().GetCursorIndex();
  assert(index < stations.size());

  StaticString<256> tmp;
  tmp.Format(_("Do you want to remove station %s?"),
             stations[index].code.c_str());

  if (ShowMessageBox(tmp, _("Remove"), MB_YESNO) == IDNO)
    return;

  noaa_store->erase(stations[index].iterator);
  noaa_store->SaveToProfile();

  UpdateList();
}

void
NOAAListWidget::OpenDetails(unsigned index)
{
  assert(index < stations.size());
  dlgNOAADetailsShowModal(stations[index].iterator);
  UpdateList();
}

inline void
NOAAListWidget::DetailsClicked()
{
  if (!stations.empty())
    OpenDetails(GetList().GetCursorIndex());
}

void
NOAAListWidget::OnActivateItem(unsigned index)
{
  OpenDetails(index);
}

void
NOAAListWidget::OnAction(int id)
{
  switch ((Buttons)id) {
  case DETAILS:
    DetailsClicked();
    break;

  case ADD:
    AddClicked();
    break;

  case UPDATE:
    UpdateClicked();
    break;

  case REMOVE:
    RemoveClicked();
    break;
  }
}

void
dlgNOAAListShowModal()
{
  NOAAListWidget widget;
  WidgetDialog dialog(UIGlobals::GetDialogLook());
  dialog.CreateFull(UIGlobals::GetMainWindow(), _("METAR and TAF"), &widget);
  dialog.AddButton(_("Close"), mrOK);
  widget.CreateButtons(dialog);

  dialog.ShowModal();
  dialog.StealWidget();
}

#else
void
dlgNOAAListShowModal()
{
  ShowMessageBox(_("This function is not available on your platform yet."),
              _("Error"), MB_OK);
}
#endif
