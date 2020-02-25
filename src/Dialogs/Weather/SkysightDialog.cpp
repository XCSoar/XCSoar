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

#include "SkysightDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Language/Language.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_SKYSIGHT
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Form/ActionListener.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Weather/Skysight/Skysight.hpp"
#include "Operation/VerboseOperationEnvironment.hpp"
#include "Util/TrivialArray.hxx"
#include "Util/StringAPI.hxx"
#include "Renderer/TextRowRenderer.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Event/Timer.hpp"

#include "Protection.hpp"
#include "DataGlobals.hpp"
#include "Interface.hpp"

#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "Net/HTTP/Session.hpp"

#include "MapWindow/OverlayBitmap.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Dialogs/Error.hpp"

#include "Util/StaticString.hxx"
#include "Language/Language.hpp"
#include "Time/BrokenDateTime.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Formatter/LocalTimeFormatter.hpp"

#include "Weather/Skysight/Skysight.hpp"

class SkysightListItemRenderer
{
  TwoTextRowsRenderer row_renderer; 

public:
  SkysightListItemRenderer(): skysight(DataGlobals::GetSkysight()) {}

  void Draw(Canvas &canvas, const PixelRect rc, unsigned i); 

  unsigned CalculateLayout(const DialogLook &look) {
    return row_renderer.CalculateLayout(*look.list.font_bold, look.small_font);
  }

private:
  std::shared_ptr<Skysight> skysight;
};

/**
 * RENDERER FOR METRICS POPUP LIST
 */
class SkysightMetricsListItemRenderer: public ListItemRenderer
{
  TextRowRenderer row_renderer;
  std::shared_ptr<Skysight> skysight;

public:
  SkysightMetricsListItemRenderer(): skysight(DataGlobals::GetSkysight()) {}
  
  unsigned CalculateLayout(const DialogLook &look) {
    return row_renderer.CalculateLayout(*look.list.font);
  }

  void OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned i) override {
    row_renderer.DrawTextRow(canvas, rc, skysight->GetMetric(i).name.c_str());
  }
  
  static const TCHAR* HelpCallback(unsigned i)
  {
    std::shared_ptr<Skysight> skysight = DataGlobals::GetSkysight();

    if (!skysight)
      return _("No description available.");

    tstring helptext = skysight->GetMetric(i).desc;
    TCHAR *help = new TCHAR[helptext.length() + 1];
    std::strcpy(help, helptext.c_str());
    return help;
  }  
};

class SkysightWidget final
  : public ListWidget, private ActionListener, private Timer
{
  enum Buttons {
    UPDATE,
    UPDATE_ALL
  };

  ButtonPanelWidget *buttons_widget;

  Button *update_button, *updateall_button;

  struct ListItem {
    StaticString<255> name;

    gcc_pure bool operator<(const ListItem &i2) const
    {
      return StringCollate(name, i2.name) < 0;
    }
  };

  SkysightListItemRenderer row_renderer;
  std::shared_ptr<Skysight> skysight;

public:
  explicit SkysightWidget(std::shared_ptr<Skysight> &&_skysight)
    : skysight(std::move(_skysight)) {}

  void SetButtonPanel(ButtonPanelWidget &_buttons) {
    buttons_widget = &_buttons;
  }

  void CreateButtons(ButtonPanel &buttons);

private:
  void UpdateList();
  void UpdateClicked();
  void UpdateAllClicked();

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;
  void Unprepare() override;

protected:
  /* virtual methods from ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) override;

  /* virtual methods from ListCursorHandler */
  bool CanActivateItem(unsigned index) const override {
    return true;
  }

  void OnActivateItem(unsigned index) override {};

private:
  /* virtual methods from class ActionListener */
  void OnAction(int id) noexcept override;

  /* virtual methods from Timer */
  void OnTimer() override;
};

void
SkysightWidget::CreateButtons(ButtonPanel &buttons)
{
  update_button = buttons.Add(_("Update"), *this, UPDATE);
  updateall_button = buttons.Add(_("Update All"), *this, UPDATE_ALL);
}

void
SkysightWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  CreateButtons(buttons_widget->GetButtonPanel());
  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc, row_renderer.CalculateLayout(look));
  UpdateList();
  Timer::Schedule(std::chrono::milliseconds(500));
}

void
SkysightWidget::Unprepare()
{
  Timer::Cancel();
  DeleteWindow();
}

void
SkysightWidget::OnTimer()
{
  UpdateList();
}

void
SkysightWidget::UpdateList()
{
  unsigned index = GetList().GetCursorIndex();
  bool item_updating = false;

  if ((int)index < skysight->NumActiveMetrics()) {
    SkysightActiveMetric a = skysight->GetActiveMetric(index);
    item_updating = a.updating;
  }

  bool any_updating = skysight->ActiveMetricsUpdating();
  bool empty = (!(bool)skysight->NumActiveMetrics());

  ListControl &list = GetList();
  list.SetLength(skysight->NumActiveMetrics());
  list.Invalidate();

  update_button->SetEnabled(!empty && !item_updating);
  updateall_button->SetEnabled(!empty && !any_updating);
}

void
SkysightWidget::OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned index)
{
}

void SkysightWidget::UpdateClicked()
{
  unsigned index = GetList().GetCursorIndex();
  assert(index < (unsigned)skysight->NumActiveMetrics());

  SkysightActiveMetric a = skysight->GetActiveMetric(index);  
  if (!skysight->DownloadActiveMetric(a.metric->id))
    ShowMessageBox(_("Couldn't update data."), _("Update Error"), MB_OK);
  UpdateList();
}

void SkysightWidget::UpdateAllClicked()
{
  if (!skysight->DownloadActiveMetric("*"))
    ShowMessageBox(_("Couldn't update data."), _("Update Error"), MB_OK);
  UpdateList();
}

void
SkysightWidget::OnAction(int id) noexcept
{
  switch ((Buttons)id) {
  case UPDATE:
    UpdateClicked();
    break;
    
  case UPDATE_ALL:
    UpdateAllClicked();
    break;    
  default:
    break;
  }
}

Widget *
CreateSkysightWidget()
{
  auto skysight = DataGlobals::GetSkysight();
  SkysightWidget *list = new SkysightWidget(std::move(skysight));
  ButtonPanelWidget *buttons =
    new ButtonPanelWidget(list, ButtonPanelWidget::Alignment::BOTTOM);
  list->SetButtonPanel(*buttons);
  return buttons;
}
#endif
