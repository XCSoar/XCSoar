/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "PagesConfigPanel.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "PageActions.hpp"
#include "Language/Language.hpp"
#include "Profile/PageProfile.hpp"
#include "Profile/Current.hpp"
#include "Interface.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "UIGlobals.hpp"

/* this macro exists in the WIN32 API */
#ifdef DELETE
#undef DELETE
#endif

class PageLayoutEditWidget final
  : public RowFormWidget, private DataFieldListener {
public:
  class Listener {
  public:
    virtual void OnModified(const PageLayout &new_value) noexcept = 0;
  };

private:
  enum Controls {
    MAIN,
    INFO_BOX_PANEL,
    BOTTOM,
  };

  static constexpr unsigned IBP_NONE = 0x7000;
  static constexpr unsigned IBP_AUTO = 0x7001;

  PageLayout value;

  Listener &listener;

public:
  PageLayoutEditWidget(const DialogLook &_look, Listener &_listener)
    :RowFormWidget(_look), listener(_listener) {}

  void SetValue(const PageLayout &_value);

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

private:
  /* virtual methods from class DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

class PageListWidget
  : public ListWidget,
    public PageLayoutEditWidget::Listener {

  TextRowRenderer row_renderer;

  PageLayoutEditWidget *editor;

  PageSettings settings;

  ButtonPanelWidget *buttons;
  Button *add_button, *delete_button;
  Button *move_up_button, *move_down_button;

public:
  ~PageListWidget() {
    if (IsDefined())
      DeleteWindow();
  }

  void SetEditor(PageLayoutEditWidget &_editor) {
    editor = &_editor;
  }

  void SetButtonPanel(ButtonPanelWidget &_buttons) {
    buttons = &_buttons;
  }

  void CreateButtons(ButtonPanel &buttons) {
    add_button = buttons.Add(_("Add"), [this](){
      const unsigned n = GetList().GetLength();
      if (n < PageSettings::MAX_PAGES) {
        auto &page = settings.pages[n];
        page = PageLayout::Default();
        GetList().SetLength(n + 1);
        GetList().SetCursorIndex(n);
      }
    });

    delete_button = buttons.Add(_("Delete"), [this](){
      const unsigned n = GetList().GetLength();
      const unsigned cursor = GetList().GetCursorIndex();
      if (n >= 2 && GetList().GetCursorIndex() < n) {
        std::copy(settings.pages.begin() + cursor + 1,
                  settings.pages.begin() + n,
                  settings.pages.begin() + cursor);
        GetList().SetLength(n - 1);

        if (cursor == n - 1)
          GetList().SetCursorIndex(cursor - 1);
        else
          editor->SetValue(settings.pages[cursor]);
      }
    });

    move_up_button = buttons.AddSymbol(_T("^"), [this](){
      const unsigned cursor = GetList().GetCursorIndex();
      if (cursor > 0) {
        std::swap(settings.pages[cursor], settings.pages[cursor - 1]);
        GetList().SetCursorIndex(cursor - 1);
      }
    });

    move_down_button = buttons.AddSymbol(_T("v"), [this](){
      const unsigned n = GetList().GetLength();
      const unsigned cursor = GetList().GetCursorIndex();
      if (cursor + 1 < n) {
        std::swap(settings.pages[cursor], settings.pages[cursor + 1]);
        GetList().SetCursorIndex(cursor + 1);
      }
    });
  }

  void UpdateButtons() {
    unsigned length = GetList().GetLength();
    unsigned cursor = GetList().GetCursorIndex();

    add_button->SetEnabled(length < settings.MAX_PAGES);
    delete_button->SetEnabled(length >= 2);
    move_up_button->SetEnabled(cursor > 0);
    move_down_button->SetEnabled(cursor + 1 < length);
  }

  /* virtual methods from class Widget */
  void Initialise(ContainerWindow &parent,
                  const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

  /* virtual methods from class ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  /* virtual methods from class ListCursorHandler */
  void OnCursorMoved(unsigned index) noexcept override;
  bool CanActivateItem(unsigned index) const noexcept override {
    return true;
  }
  void OnActivateItem(unsigned index) noexcept override;

  /* virtual methods from class PageLayoutEditWidget::Listener */
  void OnModified(const PageLayout &new_value) noexcept override;
};

void
PageLayoutEditWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  const InfoBoxSettings &info_box_settings =
    CommonInterface::GetUISettings().info_boxes;

  static constexpr StaticEnumChoice main_list[] = {
    { (unsigned)PageLayout::Main::MAP, N_("Map") },
    { (unsigned)PageLayout::Main::FLARM_RADAR, N_("FLARM radar") },
    { (unsigned)PageLayout::Main::THERMAL_ASSISTANT, N_("Thermal assistant") },
    { (unsigned)PageLayout::Main::HORIZON, N_("Horizon") },
    { 0 }
  };
  AddEnum(_("Main area"),
          _("Specifies what should be displayed in the main area."),
          main_list,
          (unsigned)PageLayout::Main::MAP, this);

  static constexpr StaticEnumChoice ib_list[] = {
    { IBP_AUTO, N_("Auto"), N_("Displays either the Circling, Cruise or Final glide infoxboxes") },
    { IBP_NONE, N_("None"), N_("Show fullscreen (no InfoBoxes)") },
    { 0 }
  };

  WndProperty *wp = AddEnum(_("InfoBoxes"),
                            _("Specifies which InfoBoxes should be displayed on this page."),
                            ib_list, IBP_AUTO, this);
  DataFieldEnum &ib = *(DataFieldEnum *)wp->GetDataField();
  for (unsigned i = 0; i < InfoBoxSettings::MAX_PANELS; ++i) {
    const TCHAR cruise_help[] = N_("For cruise mode.  Displayed when 'Auto' is selected and ship is below final glide altitude");
    const TCHAR circling_help[] = N_("For circling mode.  Displayed when 'Auto' is selected and ship is circling");
    const TCHAR final_glide_help[] = N_("For final glide mode.  Displayed when 'Auto' is selected and ship is above final glide altitude");
    const TCHAR *display_text = gettext(info_box_settings.panels[i].name);
    const TCHAR *help_text = N_("A custom InfoBox set");
    switch (i) {
    case 0:
      help_text = circling_help;
      break;
    case 1:
      help_text = cruise_help;
      break;
    case 2:
      help_text = final_glide_help;
      break;
    default:
      break;
    }
    ib.AddChoice(i, display_text, display_text, help_text);
  }

  static constexpr StaticEnumChoice bottom_list[] = {
    { (unsigned)PageLayout::Bottom::NOTHING, N_("Nothing") },
    { (unsigned)PageLayout::Bottom::CROSS_SECTION,
                N_("Cross section") },
    { 0 }
  };
  AddEnum(_("Bottom area"),
          _("Specifies what should be displayed below the main area."),
          bottom_list,
          (unsigned)PageLayout::Bottom::NOTHING, this);
}

void
PageLayoutEditWidget::SetValue(const PageLayout &_value)
{
  value = _value;

  LoadValueEnum(MAIN, value.main);
  LoadValueEnum(BOTTOM, value.bottom);

  unsigned ib = IBP_NONE;
  if (value.infobox_config.enabled) {
    if (value.infobox_config.auto_switch)
      ib = IBP_AUTO;
    else if (value.infobox_config.panel < InfoBoxSettings::MAX_PANELS)
      ib = value.infobox_config.panel;
    else
      /* fix up illegal value */
      ib = 0;
  }

  LoadValueEnum(INFO_BOX_PANEL, ib);
}

void
PageLayoutEditWidget::OnModified(DataField &df) noexcept
{
  if (&df == &GetDataField(MAIN)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    value.main = (PageLayout::Main)dfe.GetValue();
  } else if (&df == &GetDataField(INFO_BOX_PANEL)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    const unsigned ibp = dfe.GetValue();
    if (ibp == IBP_AUTO) {
      value.infobox_config.enabled = true;
      value.infobox_config.auto_switch = true;
      value.infobox_config.panel = 0;
    } else if (ibp == IBP_NONE)
      value.infobox_config.enabled = false;
    else if (ibp < InfoBoxSettings::MAX_PANELS) {
      value.infobox_config.enabled = true;
      value.infobox_config.auto_switch = false;
      value.infobox_config.panel = ibp;
    }
  } else if (&df == &GetDataField(BOTTOM)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    value.bottom = (PageLayout::Bottom)dfe.GetValue();
  } else {
    gcc_unreachable();
  }

  listener.OnModified(value);
}

void
PageListWidget::Initialise(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  settings = CommonInterface::GetUISettings().pages;

  CreateList(parent, UIGlobals::GetDialogLook(), rc,
             row_renderer.CalculateLayout(*look.list.font))
    .SetLength(settings.n_pages);

  CreateButtons(buttons->GetButtonPanel());
  UpdateButtons();
}

void
PageListWidget::Show(const PixelRect &rc) noexcept
{
  editor->SetValue(settings.pages[GetList().GetCursorIndex()]);

  ListWidget::Show(rc);
}

bool
PageListWidget::Save(bool &_changed) noexcept
{
  bool changed = false;

  settings.n_pages = GetList().GetLength();
  std::fill(settings.pages.begin() + settings.n_pages,
            settings.pages.end(),
            PageLayout::Undefined());

  PageSettings &_settings = CommonInterface::SetUISettings().pages;
  for (unsigned int i = 0; i < PageSettings::MAX_PAGES; ++i) {
    PageLayout &dest = _settings.pages[i];
    const PageLayout &src = settings.pages[i];
    if (src != dest) {
      Profile::Save(Profile::map, src, i);
      changed = true;
    }
  }

  if (changed) {
    _settings = settings;
    PageActions::Update();
  }

  _changed |= changed;
  return true;
}

void
PageListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                            unsigned idx) noexcept
{
  const InfoBoxSettings &info_box_settings =
    CommonInterface::GetUISettings().info_boxes;

  assert(idx < PageSettings::MAX_PAGES);
  const auto &value = settings.pages[idx];

  StaticString<64> buffer;

  switch (value.main) {
  case PageLayout::Main::MAP:
    buffer = _("Map");
    break;

  case PageLayout::Main::FLARM_RADAR:
    buffer = _("FLARM radar");
    break;

  case PageLayout::Main::THERMAL_ASSISTANT:
    buffer = _("Thermal assistant");
    break;

  case PageLayout::Main::HORIZON:
    buffer = _("Horizon");
    break;

  case PageLayout::Main::MAX:
    gcc_unreachable();
  }

  if (value.infobox_config.enabled) {
    buffer.AppendFormat(_T(", %s"), _("InfoBoxes"));

    if (!value.infobox_config.auto_switch &&
        value.infobox_config.panel < InfoBoxSettings::MAX_PANELS)
      buffer.AppendFormat(_T(" (%s)"),
                          gettext(info_box_settings.panels[value.infobox_config.panel].name));
    else
      buffer.AppendFormat(_T(" (%s)"), _("Auto"));
  }

  switch (value.bottom) {
  case PageLayout::Bottom::NOTHING:
  case PageLayout::Bottom::CUSTOM:
    break;

  case PageLayout::Bottom::CROSS_SECTION:
    buffer.AppendFormat(_T(", %s"), _("Cross section"));
    break;

  case PageLayout::Bottom::MAX:
    gcc_unreachable();
  }

  row_renderer.DrawTextRow(canvas, rc, buffer);
}

void
PageListWidget::OnCursorMoved(unsigned idx) noexcept
{
  UpdateButtons();

  editor->SetValue(settings.pages[idx]);
}

void
PageListWidget::OnActivateItem(unsigned idx) noexcept
{
  editor->SetFocus();
}

void
PageListWidget::OnModified(const PageLayout &new_value) noexcept
{
  unsigned i = GetList().GetCursorIndex();
  assert(i < PageSettings::MAX_PAGES);

  if (i == 0 && !new_value.IsDefined()) {
    /* refuse to delete the first page (kludge) */
    editor->SetValue(settings.pages[i]);
    return;
  }

  settings.pages[i] = new_value;
  GetList().Invalidate();
}

std::unique_ptr<Widget>
CreatePagesConfigPanel()
{
  auto _list = std::make_unique<PageListWidget>();
  auto _editor = std::make_unique<PageLayoutEditWidget>(UIGlobals::GetDialogLook(),
                                                        *_list);

  auto two = std::make_unique<TwoWidgets>(std::move(_list),
                                          std::move(_editor));
  auto &list = (PageListWidget &)two->GetFirst();
  auto &editor = (PageLayoutEditWidget &)two->GetSecond();
  list.SetEditor(editor);

  auto buttons = std::make_unique<ButtonPanelWidget>(std::move(two),
                                                     ButtonPanelWidget::Alignment::BOTTOM);
  list.SetButtonPanel(*buttons);

  return buttons;
}
