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



class SkysightListItemRenderer  {
 TwoTextRowsRenderer row_renderer; 
  
public:
  SkysightListItemRenderer() : skysight(DataGlobals::GetSkysight()) {}
    
  void Draw(Canvas &canvas, const PixelRect rc, unsigned i); 
  
  unsigned CalculateLayout(const DialogLook &look) {
    return row_renderer.CalculateLayout(*look.list.font_bold, look.small_font);
  }

private:
  std::shared_ptr<Skysight> skysight;

};

//TODO: Could extend this to extract Skysight data at point and display on list (see NOAAListRenderer::Draw(2)
void

SkysightListItemRenderer::Draw(Canvas &canvas, const PixelRect rc, unsigned i) {
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  SkysightActiveMetric m = SkysightActiveMetric(skysight->GetActiveMetric(i));

  tstring first_row = tstring(m.metric->name);
  if(skysight->displayed_metric == m.metric->id.c_str())
    first_row += " [ACTIVE]";

  StaticString<256> second_row;

  if(m.updating) {
    second_row.Format("%s", _("Updating..."));
  } else {
    if(!m.from || !m.to || !m.mtime) {
      second_row.Format("%s", _("No data. Press \"Update\" to update."));
    } else {
      
      uint64_t elapsed = BrokenDateTime::NowUTC().ToUnixTimeUTC() - m.mtime;

      second_row.Format(_("_Data from %s to %s. Updated %s ago"), 
                        FormatLocalTimeHHMM((int)m.from, settings.utc_offset).c_str(),
                        FormatLocalTimeHHMM((int)m.to, settings.utc_offset).c_str(),
                        FormatTimespanSmart(elapsed).c_str());  
    }
  }
  
  row_renderer.DrawFirstRow(canvas, rc, first_row.c_str());
  row_renderer.DrawSecondRow(canvas, rc, second_row.c_str());
}

/**
 * RENDERER FOR METRICS POPUP LIST
 */


class SkysightMetricsListItemRenderer : public ListItemRenderer {
  TextRowRenderer row_renderer;
  std::shared_ptr<Skysight> skysight;

public:
  SkysightMetricsListItemRenderer() : skysight(DataGlobals::GetSkysight()) {}
  
  unsigned CalculateLayout(const DialogLook &look) {
    return row_renderer.CalculateLayout(*look.list.font);
  }

  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned i) override {
    row_renderer.DrawTextRow(canvas, rc, skysight->GetMetric(i).name.c_str());
  }
  
  static const TCHAR* HelpCallback(unsigned i) {
    
    std::shared_ptr<Skysight> skysight = DataGlobals::GetSkysight();
        
    if(!skysight)
      return _("No description available.");
    
    tstring helptext = skysight->GetMetric(i).desc;
    TCHAR* help = new TCHAR[helptext.length() + 1];
    std::strcpy(help, helptext.c_str());
    return help;
  }  

};

class SkysightWidget final
  : public ListWidget, private ActionListener, private Timer {
  enum Buttons {
    ACTIVATE,
    DEACTIVATE,
    ADD,
    REMOVE,
    UPDATE,
    UPDATE_ALL
  };

  ButtonPanelWidget *buttons_widget;

  Button *activate_button, *deactivate_button, *add_button, *remove_button, *update_button,
                                                                    *updateall_button;

  struct ListItem {
    StaticString<255> name;

    gcc_pure
    bool operator<(const ListItem &i2) const {
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

  void ActivateClicked();
  void DeactivateClicked();
  void AddClicked();
  void UpdateClicked();
  void UpdateAllClicked();
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

  virtual void OnActivateItem(unsigned index) override {};

private:
  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) noexcept override;
  
  /* virtual methods from Timer */
  virtual void OnTimer() override;
  
};

void
SkysightWidget::CreateButtons(ButtonPanel &buttons)
{
  activate_button = buttons.Add(_("Activate"), *this, ACTIVATE);
  deactivate_button = buttons.Add(_("Deactivate"), *this, DEACTIVATE);
  add_button = buttons.Add(_("Add"), *this, ADD);
  remove_button = buttons.Add(_("Remove"), *this, REMOVE);
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
  bool item_active = false;
  
  if((int)index < skysight->NumActiveMetrics()) {
    SkysightActiveMetric a = skysight->GetActiveMetric(index);
    item_updating = a.updating;
    item_active = (skysight->displayed_metric == a.metric->id.c_str());
  }
  
  bool any_updating = skysight->ActiveMetricsUpdating();
  bool empty = (!(bool)skysight->NumActiveMetrics());

  ListControl &list = GetList();
  list.SetLength(skysight->NumActiveMetrics());
  list.Invalidate();

  add_button->SetEnabled(!skysight->ActiveMetricsFull());
  remove_button->SetEnabled(!empty && !item_updating);
  update_button->SetEnabled(!empty && !item_updating);
  updateall_button->SetEnabled(!empty && !any_updating);
  activate_button->SetEnabled(!empty && !item_updating); 
  deactivate_button->SetEnabled(!empty && item_active);

}

void SkysightWidget::OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned index)
{
  row_renderer.Draw(canvas, rc, index);
}

void SkysightWidget::AddClicked()
{
  
  if (!skysight->IsReady()) {
    ShowMessageBox(
      _("Please check your Skysight settings and internet connection."),
      _("Couldn't connect to Skysight"),
      MB_OK
    );
    return;
  }

  SkysightMetricsListItemRenderer item_renderer;
 
  int i = ListPicker(_("Choose a parameter"),
                     skysight->NumMetrics(), 0,
                     item_renderer.CalculateLayout(UIGlobals::GetDialogLook()),
                     item_renderer,
                     false, /*timer */
                     nullptr, /*TCHAR help text */
                     &SkysightMetricsListItemRenderer::HelpCallback,
                     nullptr /*Extra caption*/);

  if (i < 0)
    return;

  assert((int)i < skysight->NumMetrics());
  skysight->AddActiveMetric(skysight->GetMetric(i).id.c_str());
  
  UpdateList();
}

void SkysightWidget::UpdateClicked()
{
  unsigned index = GetList().GetCursorIndex();
  assert(index < (unsigned)skysight->NumActiveMetrics());
  
  SkysightActiveMetric a = skysight->GetActiveMetric(index);  
  if(!skysight->DownloadActiveMetric(a.metric->id))
    ShowMessageBox(_("Couldn't update data."), _("Update Error"), MB_OK);
  UpdateList();
}

void SkysightWidget::UpdateAllClicked()
{
  if(!skysight->DownloadActiveMetric("*"))
    ShowMessageBox(_("Couldn't update data."), _("Update Error"), MB_OK);
  UpdateList();
}
 

void SkysightWidget::RemoveClicked()
{
  
  unsigned index = GetList().GetCursorIndex();
  assert(index < (unsigned)skysight->NumActiveMetrics());
  
  SkysightActiveMetric a = skysight->GetActiveMetric(index);
  StaticString<256> tmp;
  tmp.Format(_("Do you want to remove \"%s\"?"),
             a.metric->name.c_str());

  if (ShowMessageBox(tmp, _("Remove"), MB_YESNO) == IDNO)
    return;

  skysight->RemoveActiveMetric(a.metric->id);

  UpdateList();

}

inline void
SkysightWidget::ActivateClicked()
{
  unsigned index = GetList().GetCursorIndex();
  assert(index < (unsigned)skysight->NumActiveMetrics());
  
  SkysightActiveMetric a = skysight->GetActiveMetric(index);  
  if(!skysight->DisplayActiveMetric(a.metric->id.c_str()))
    ShowMessageBox(_("Couldn't display data. There is no forecast data available for this time."), _("Display Error"), MB_OK);
  UpdateList();
}

inline void
SkysightWidget::DeactivateClicked()
{
  unsigned index = GetList().GetCursorIndex();
  assert(index < (unsigned)skysight->NumActiveMetrics());

  skysight->DisplayActiveMetric();
  UpdateList();
}

void
SkysightWidget::OnAction(int id) noexcept
{
  switch ((Buttons)id) {
  case ACTIVATE:
    ActivateClicked();
    break;

  case DEACTIVATE:
    DeactivateClicked();
    break;

  case ADD:
    AddClicked();
    break;

  case UPDATE:
    UpdateClicked();
    break;
    
  case UPDATE_ALL:
    UpdateAllClicked();
    break;    

  case REMOVE:
    RemoveClicked();
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
