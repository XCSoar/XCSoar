// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
#include "DataGlobals.hpp"
#include "Weather/Features.hpp"
#include "Weather/Rasp/FieldControls.hpp"
#include "Weather/Rasp/RaspStore.hpp"
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
    OVERLAY,
    RASP_FIELD,
  };

  static constexpr unsigned IBP_NONE = 0x7000;
  static constexpr unsigned IBP_AUTO = 0x7001;

  PageLayout value;

  Listener &listener;

  void UpdateOverlayControls() noexcept;
  void FillRaspFieldControl() noexcept;

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

    move_up_button = buttons.AddSymbol("^", [this](){
      const unsigned cursor = GetList().GetCursorIndex();
      if (cursor > 0) {
        std::swap(settings.pages[cursor], settings.pages[cursor - 1]);
        GetList().SetCursorIndex(cursor - 1);
      }
    });

    move_down_button = buttons.AddSymbol("v", [this](){
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
  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return true;
  }
  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override;

  /* virtual methods from class PageLayoutEditWidget::Listener */
  void OnModified(const PageLayout &new_value) noexcept override;
};

void
PageLayoutEditWidget::FillRaspFieldControl() noexcept
{
  auto &control = GetControl(RASP_FIELD);
  auto &df = (DataFieldEnum &)*control.GetDataField();

  const auto rasp = DataGlobals::GetRasp();
  if (rasp == nullptr || rasp->GetItemCount() == 0) {
    df.ClearChoices();
    df.AddChoice(-1, _("No RASP file loaded"));
    df.SetValue(-1);
    control.RefreshDisplay();
    return;
  }

  Rasp::FillFieldChoices(df, rasp.get());

  if (value.rasp_field >= 0 &&
      unsigned(value.rasp_field) < rasp->GetItemCount())
    df.SetValue(value.rasp_field);
  else
    df.SetValue(0U);

  control.RefreshDisplay();
}

void
PageLayoutEditWidget::UpdateOverlayControls() noexcept
{
  const bool map_page = value.IsMapMain();
  const auto rasp = DataGlobals::GetRasp();
  const bool rasp_available = rasp != nullptr && rasp->GetItemCount() > 0;
  const bool rasp_overlay = value.overlay == PageLayout::Overlay::RASP;

  SetRowAvailable(OVERLAY, map_page);
  SetRowAvailable(RASP_FIELD, map_page && rasp_overlay);

  SetRowEnabled(OVERLAY, map_page);
  SetRowEnabled(RASP_FIELD, map_page && rasp_overlay && rasp_available);
}

void
PageLayoutEditWidget::Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept
{
  const InfoBoxSettings &info_box_settings =
    CommonInterface::GetUISettings().info_boxes;

  static constexpr StaticEnumChoice main_list[] = {
    { PageLayout::Main::MAP, N_("Map") },
    { PageLayout::Main::MAP_NORTH_UP, N_("Map (north-up)") },
    { PageLayout::Main::FLARM_RADAR, N_("FLARM radar") },
    { PageLayout::Main::THERMAL_ASSISTANT, N_("Thermal assistant") },
    { PageLayout::Main::HORIZON, N_("Horizon") },
    nullptr
  };
  AddEnum(_("Main area"),
          _("Specifies what should be displayed in the main area."),
          main_list,
          (unsigned)PageLayout::Main::MAP, this);

  static constexpr StaticEnumChoice ib_list[] = {
    { IBP_AUTO, N_("Auto"), N_("Displays either the Circling, Cruise, or Final glide InfoBoxes.") },
    { IBP_NONE, N_("None"), N_("Show fullscreen (no InfoBoxes)") },
    nullptr
  };

  WndProperty *wp = AddEnum(_("InfoBoxes"),
                            _("Specifies which InfoBoxes should be displayed on this page."),
                            ib_list, IBP_AUTO, this);
  DataFieldEnum &ib = *(DataFieldEnum *)wp->GetDataField();
  for (unsigned i = 0; i < InfoBoxSettings::MAX_PANELS; ++i) {
    const char cruise_help[] = N_("For cruise mode. Displayed when 'Auto' is selected and glider is below final glide altitude.");
    const char circling_help[] = N_("For circling mode. Displayed when 'Auto' is selected and glider is circling.");
    const char final_glide_help[] = N_("For final glide mode. Displayed when 'Auto' is selected and glider is above final glide altitude.");
    const char *display_text = gettext(info_box_settings.panels[i].name);
    const char *help_text = N_("A custom InfoBox set");
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
    { PageLayout::Bottom::NOTHING, N_("Nothing") },
    { PageLayout::Bottom::CROSS_SECTION, N_("Cross section") },
#ifdef HAVE_EDL
    { PageLayout::Bottom::EDL_CONTROLS, N_("Weather controls") },
#endif
    nullptr
  };
  AddEnum(_("Bottom area"),
          _("Specifies what should be displayed below the main area."),
          bottom_list,
          (unsigned)PageLayout::Bottom::NOTHING, this);

  static constexpr StaticEnumChoice overlay_list[] = {
    { PageLayout::Overlay::NONE, N_("None") },
    { PageLayout::Overlay::RASP, N_("RASP") },
#ifdef HAVE_EDL
    { PageLayout::Overlay::EDL, N_("EDL") },
#endif
    nullptr
  };
  AddEnum(_("Map overlay"),
          _("Optional weather overlay drawn on map pages."),
          overlay_list,
          (unsigned)PageLayout::Overlay::NONE, this);

  AddEnum(_("RASP layer"),
          _("RASP weather layer to display on this map page."),
          this);
  GetControl(RASP_FIELD).GetDataField()->EnableItemHelp(true);
  FillRaspFieldControl();
  UpdateOverlayControls();
}

void
PageLayoutEditWidget::SetValue(const PageLayout &_value)
{
  value = _value;
  value.Normalise();

  LoadValueEnum(MAIN, value.main);
  LoadValueEnum(BOTTOM, value.bottom);
  LoadValueEnum(OVERLAY, value.overlay);

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

  FillRaspFieldControl();
  UpdateOverlayControls();
}

void
PageLayoutEditWidget::OnModified(DataField &df) noexcept
{
  if (&df == &GetDataField(MAIN)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    value.main = (PageLayout::Main)dfe.GetValue();
    if (!value.IsMapMain())
      value.overlay = PageLayout::Overlay::NONE;
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
  } else if (&df == &GetDataField(OVERLAY)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    value.overlay = (PageLayout::Overlay)dfe.GetValue();
    if (value.overlay == PageLayout::Overlay::RASP)
      FillRaspFieldControl();
  } else if (&df == &GetDataField(RASP_FIELD)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    value.rasp_field = dfe.GetValue();
  } else {
    gcc_unreachable();
  }

  value.Normalise();
  LoadValueEnum(BOTTOM, value.bottom);
  GetControl(BOTTOM).RefreshDisplay();
  UpdateOverlayControls();
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

  for (unsigned i = 0; i < settings.n_pages; ++i)
    settings.pages[i].Normalise();

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
  row_renderer.DrawTextRow(canvas, rc,
                           value.MakeTitle(info_box_settings,
                                           std::span{buffer.data(), buffer.capacity()},
                                           DataGlobals::GetRasp().get()));
}

void
PageListWidget::OnCursorMoved[[maybe_unused]] (unsigned idx) noexcept
{
  UpdateButtons();

  editor->SetValue(settings.pages[idx]);
}

void
PageListWidget::OnActivateItem([[maybe_unused]] unsigned idx) noexcept
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
