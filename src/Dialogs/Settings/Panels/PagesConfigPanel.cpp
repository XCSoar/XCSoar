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
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Pages.hpp"
#include "Language/Language.hpp"
#include "Profile/PageProfile.hpp"
#include "Interface.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "UIGlobals.hpp"

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
  };

  static constexpr unsigned IBP_NONE = 0x7000;
  static constexpr unsigned IBP_AUTO = 0x7001;
  static constexpr unsigned IBP_INVALID = 0x7002;

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
  : public ListWidget, public PageLayoutEditWidget::Listener {
  PageLayoutEditWidget *editor;

  PageSettings settings;

public:
  ~PageListWidget() {
    if (IsDefined())
      DeleteWindow();
  }

  void SetEditor(PageLayoutEditWidget &_editor) {
    editor = &_editor;
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
};

void
PageLayoutEditWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const InfoBoxSettings &info_box_settings =
    CommonInterface::GetUISettings().info_boxes;

  static constexpr StaticEnumChoice ib_list[] = {
    { IBP_INVALID, _T("---") },
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
}

void
PageLayoutEditWidget::SetValue(const PageSettings::PageLayout &_value)
{
  value = _value;

  unsigned ib = IBP_INVALID;
  switch (value.top_layout) {
  case PageSettings::PageLayout::tlEmpty:
    break;

  case PageSettings::PageLayout::tlMap:
    ib = IBP_NONE;
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
    if (ibp == IBP_INVALID)
      value.top_layout = PageSettings::PageLayout::tlEmpty;
    else if (ibp == IBP_AUTO) {
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
  } else {
    gcc_unreachable();
  }

  listener.OnModified(value);
}

void
PageListWidget::Initialise(ContainerWindow &parent, const PixelRect &rc)
{
  CreateList(parent, UIGlobals::GetDialogLook(),
             rc, Layout::Scale(18)).SetLength(PageSettings::MAX_PAGES);

  settings = CommonInterface::GetUISettings().pages;
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

  PageSettings &_settings = CommonInterface::SetUISettings().pages;
  for (unsigned int i = 0; i < PageSettings::MAX_PAGES; ++i) {
    PageSettings::PageLayout &dest = _settings.pages[i];
    const PageSettings::PageLayout &src = settings.pages[i];
    if (src != dest) {
      SetLayout(i, src);
      dest = src;
      Profile::Save(src, i);
      changed = true;
    }
  }

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
    break;

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

  canvas.DrawText(rc.left + Layout::GetTextPadding(),
                  rc.top + Layout::GetTextPadding(),
                  text);
}

void
PageListWidget::OnCursorMoved(unsigned idx)
{
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

  if (i == 0 && new_value.top_layout == PageSettings::PageLayout::tlEmpty) {
    /* refuse to delete the first page (kludge) */
    editor->SetValue(settings.pages[i]);
    return;
  }

  settings.pages[i] = new_value;
  GetList().Invalidate();
}

Widget *
CreatePagesConfigPanel()
{
  PageListWidget *list = new PageListWidget();
  PageLayoutEditWidget *editor =
    new PageLayoutEditWidget(UIGlobals::GetDialogLook(), *list);
  list->SetEditor(*editor);

  return new TwoWidgets(list, editor);
}
