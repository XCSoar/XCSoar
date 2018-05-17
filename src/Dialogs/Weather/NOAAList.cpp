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

#include "NOAAList.hpp"
#include "NOAADetails.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Language/Language.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_NOAA

#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Form/ActionListener.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Weather/NOAAGlue.hpp"
#include "Weather/NOAAStore.hpp"
#include "Weather/NOAAUpdater.hpp"
#include "Util/TrivialArray.hxx"
#include "Util/StringAPI.hxx"
#include "Compiler.h"
#include "Renderer/NOAAListRenderer.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"

class NOAAListWidget final
  : public ListWidget, private ActionListener {
  enum Buttons {
    DETAILS,
    ADD,
    UPDATE,
    REMOVE,
  };

  ButtonPanelWidget *buttons_widget;

  Button *details_button, *add_button, *update_button, *remove_button;

  struct ListItem {
    StaticString<5> code;
    NOAAStore::iterator iterator;

    gcc_pure
    bool operator<(const ListItem &i2) const {
      return StringCollate(code, i2.code) < 0;
    }
  };

  TrivialArray<ListItem, 20> stations;

  TwoTextRowsRenderer row_renderer;

public:
  void SetButtonPanel(ButtonPanelWidget &_buttons) {
    buttons_widget = &_buttons;
  }

  void CreateButtons(ButtonPanel &buttons);

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
NOAAListWidget::CreateButtons(ButtonPanel &buttons)
{
  details_button = buttons.Add(_("Details"), *this, DETAILS);
  add_button = buttons.Add(_("Add"), *this, ADD);
  update_button = buttons.Add(_("Update"), *this, UPDATE);
  remove_button = buttons.Add(_("Remove"), *this, REMOVE);
}

void
NOAAListWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  CreateButtons(buttons_widget->GetButtonPanel());

  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc,
             row_renderer.CalculateLayout(*look.list.font_bold,
                                          look.small_font));
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
    ListItem item;
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
                         row_renderer);
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

Widget *
CreateNOAAListWidget()
{
  NOAAListWidget *list = new NOAAListWidget();
  ButtonPanelWidget *buttons =
    new ButtonPanelWidget(list, ButtonPanelWidget::Alignment::BOTTOM);
  list->SetButtonPanel(*buttons);
  return buttons;
}

#endif
