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

#include "PagesConfigPanel.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Canvas.hpp"
#include "Form/ActionListener.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Pages.hpp"
#include "Language/Language.hpp"
#include "Profile/PageProfile.hpp"
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

using namespace Pages;

class PageLayoutEditWidget final
  : public RowFormWidget, private DataFieldListener {
public:
  class Listener {
  public:
    virtual void OnModified(const PageSettings::PageLayout &new_value) = 0;
  };

private:
  enum Controls {
    INFO_BOX_PANEL,
    BOTTOM,
  };

  static constexpr unsigned IBP_NONE = 0x7000;
  static constexpr unsigned IBP_AUTO = 0x7001;

  PageSettings::PageLayout value;

  Listener &listener;

public:
  PageLayoutEditWidget(const DialogLook &_look, Listener &_listener)
    :RowFormWidget(_look), listener(_listener) {}

  void SetValue(const PageSettings::PageLayout &_value);

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;

private:
  /* virtual methods from class DataFieldListener */
  virtual void OnModified(DataField &df) override;
};

class PageListWidget
  : public ListWidget, private ActionListener,
    public PageLayoutEditWidget::Listener {
  enum Buttons {
    ADD,
    DELETE,
    MOVE_UP,
    MOVE_DOWN,
  };

  PageLayoutEditWidget *editor;

  PageSettings settings;

  ButtonPanelWidget *buttons;
  WndButton *add_button, *delete_button;
  WndButton *move_up_button, *move_down_button;

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
    add_button = buttons.Add(_("Add"), *this, ADD);
    delete_button = buttons.Add(_("Delete"), *this, DELETE);
    move_up_button = buttons.AddSymbol(_("^"), *this, MOVE_UP);
    move_down_button = buttons.AddSymbol(_("v"), *this, MOVE_DOWN);
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
  virtual void Initialise(ContainerWindow &parent,
                          const PixelRect &rc) override;
  virtual void Show(const PixelRect &rc) override;
  virtual bool Save(bool &changed, bool &require_restart);

  /* virtual methods from class ListItemRenderer */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) override;

  /* virtual methods from class ListCursorHandler */
  virtual void OnCursorMoved(unsigned index) override;
  virtual bool CanActivateItem(unsigned index) const override {
    return true;
  }
  virtual void OnActivateItem(unsigned index) override;

  /* virtual methods from class PageLayoutEditWidget::Listener */
  virtual void OnModified(const PageSettings::PageLayout &new_value) override;

private:
  /* virtual methods from ActionListener */
  virtual void OnAction(int id) override;
};

void
PageLayoutEditWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const InfoBoxSettings &info_box_settings =
    CommonInterface::GetUISettings().info_boxes;

  static constexpr StaticEnumChoice ib_list[] = {
    { IBP_AUTO, N_("Auto") },
    { IBP_NONE, N_("None") },
    { 0 }
  };

  WndProperty *wp = AddEnum(_("InfoBoxes"),
                            _("Specifies which InfoBoxes should be displayed on this page."),
                            ib_list, IBP_AUTO, this);
  DataFieldEnum &ib = *(DataFieldEnum *)wp->GetDataField();
  for (unsigned i = 0; i < InfoBoxSettings::MAX_PANELS; ++i)
    ib.AddChoice(i, gettext(info_box_settings.panels[i].name));

  static constexpr StaticEnumChoice bottom_list[] = {
    { (unsigned)PageSettings::PageLayout::Bottom::NOTHING, N_("Nothing") },
    { (unsigned)PageSettings::PageLayout::Bottom::CROSS_SECTION,
                N_("Cross section") },
    { 0 }
  };
  AddEnum(_("Bottom"),
          _("Specifies what should be displayed below the map."),
          bottom_list,
          (unsigned)PageSettings::PageLayout::Bottom::NOTHING, this);
}

void
PageLayoutEditWidget::SetValue(const PageSettings::PageLayout &_value)
{
  value = _value;

  unsigned ib = IBP_NONE;
  switch (value.top_layout) {
  case PageSettings::PageLayout::tlEmpty:
    assert(false);
    gcc_unreachable();

  case PageSettings::PageLayout::tlMap:
    break;

  case PageSettings::PageLayout::tlMapAndInfoBoxes:
    if (value.infobox_config.auto_switch)
      ib = IBP_AUTO;
    else if (value.infobox_config.panel < InfoBoxSettings::MAX_PANELS)
      ib = value.infobox_config.panel;
    else
      /* fix up illegal value */
      ib = 0;
    break;
  }

  LoadValueEnum(INFO_BOX_PANEL, ib);
}

void
PageLayoutEditWidget::OnModified(DataField &df)
{
  if (&df == &GetDataField(INFO_BOX_PANEL)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    const unsigned ibp = dfe.GetValue();
    if (ibp == IBP_AUTO) {
      value.top_layout = PageSettings::PageLayout::tlMapAndInfoBoxes;
      value.infobox_config.auto_switch = true;
      value.infobox_config.panel = 0;
    } else if (ibp == IBP_NONE)
      value.top_layout = PageSettings::PageLayout::tlMap;
    else if (ibp < InfoBoxSettings::MAX_PANELS) {
      value.top_layout = PageSettings::PageLayout::tlMapAndInfoBoxes;
      value.infobox_config.auto_switch = false;
      value.infobox_config.panel = ibp;
    }
  } else if (&df == &GetDataField(BOTTOM)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    value.bottom = (PageSettings::PageLayout::Bottom)dfe.GetValue();
  } else {
    gcc_unreachable();
  }

  listener.OnModified(value);
}

void
PageListWidget::Initialise(ContainerWindow &parent, const PixelRect &rc)
{
  settings = CommonInterface::GetUISettings().pages;
  auto end = std::remove_if(settings.pages.begin(), settings.pages.end(),
                            [](const PageSettings::PageLayout &layout) {
                              return !layout.IsDefined();
                            });
  unsigned n = std::distance(settings.pages.begin(), end);

  CreateList(parent, UIGlobals::GetDialogLook(),
             rc, Layout::Scale(18)).SetLength(n);

  CreateButtons(buttons->GetButtonPanel());
  UpdateButtons();
}

void
PageListWidget::Show(const PixelRect &rc)
{
  editor->SetValue(settings.pages[GetList().GetCursorIndex()]);

  ListWidget::Show(rc);
}

bool
PageListWidget::Save(bool &_changed, gcc_unused bool &require_restart)
{
  bool changed = false;

  std::fill(settings.pages.begin() + GetList().GetLength(),
            settings.pages.end(),
            PageSettings::PageLayout::Undefined());

  PageSettings &_settings = CommonInterface::SetUISettings().pages;
  for (unsigned int i = 0; i < PageSettings::MAX_PAGES; ++i) {
    PageSettings::PageLayout &dest = _settings.pages[i];
    const PageSettings::PageLayout &src = settings.pages[i];
    if (src != dest) {
      dest = src;
      Profile::Save(src, i);
      changed = true;
    }
  }

  Pages::Update();

  _changed |= changed;
  return true;
}

void
PageListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned idx)
{
  const InfoBoxSettings &info_box_settings =
    CommonInterface::GetUISettings().info_boxes;

  assert(idx < PageSettings::MAX_PAGES);
  const auto &value = settings.pages[idx];

  const TCHAR *text = _T("---");
  StaticString<64> buffer;

  switch (value.top_layout) {
  case PageSettings::PageLayout::tlEmpty:
    assert(false);
    gcc_unreachable();

  case PageSettings::PageLayout::tlMap:
    text = _("Map (Full screen)");
    break;

  case PageSettings::PageLayout::tlMapAndInfoBoxes:
    text = _("Map and InfoBoxes");

    if (!value.infobox_config.auto_switch &&
        value.infobox_config.panel < InfoBoxSettings::MAX_PANELS)
      buffer.Format(_T("%s (%s)"), text,
                    gettext(info_box_settings.panels[value.infobox_config.panel].name));
    else
      buffer.Format(_T("%s (%s)"), text, _("Auto"));
    text = buffer;
    break;
  }

  switch (value.bottom) {
  case PageSettings::PageLayout::Bottom::NOTHING:
    break;

  case PageSettings::PageLayout::Bottom::CROSS_SECTION:
    if (text != buffer) {
      buffer = text;
      text = buffer;
    }

    buffer.AppendFormat(_T(", %s"), _("Cross section"));
    break;
  }

  canvas.DrawText(rc.left + Layout::GetTextPadding(),
                  rc.top + Layout::GetTextPadding(),
                  text);
}

void
PageListWidget::OnCursorMoved(unsigned idx)
{
  UpdateButtons();

  editor->SetValue(settings.pages[idx]);
}

void
PageListWidget::OnActivateItem(unsigned idx)
{
  editor->SetFocus();
}

void
PageListWidget::OnModified(const PageSettings::PageLayout &new_value)
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

void
PageListWidget::OnAction(int id)
{
  const unsigned n = GetList().GetLength();
  const unsigned cursor = GetList().GetCursorIndex();

  switch (id) {
  case ADD:
    if (n < PageSettings::MAX_PAGES) {
      auto &page = settings.pages[n];
      page = PageSettings::PageLayout::Default();
      GetList().SetLength(n + 1);
      GetList().SetCursorIndex(n);
    }

    break;

  case DELETE:
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

    break;

  case MOVE_UP:
    if (cursor > 0) {
      std::swap(settings.pages[cursor], settings.pages[cursor - 1]);
      GetList().SetCursorIndex(cursor - 1);
    }

    break;

  case MOVE_DOWN:
    if (cursor + 1 < n) {
      std::swap(settings.pages[cursor], settings.pages[cursor + 1]);
      GetList().SetCursorIndex(cursor + 1);
    }

    break;
  }
}

Widget *
CreatePagesConfigPanel()
{
  PageListWidget *list = new PageListWidget();
  PageLayoutEditWidget *editor =
    new PageLayoutEditWidget(UIGlobals::GetDialogLook(), *list);
  list->SetEditor(*editor);

  TwoWidgets *two = new TwoWidgets(list, editor);
  ButtonPanelWidget *buttons = new ButtonPanelWidget(two, ButtonPanelWidget::Alignment::BOTTOM);
  list->SetButtonPanel(*buttons);

  return buttons;
}
