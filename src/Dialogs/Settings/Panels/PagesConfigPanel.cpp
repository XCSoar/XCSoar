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

#ifdef HAVE_HTTP
#include "Weather/Skysight/Skysight.hpp"
#endif

#include <string_view>

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
    SKYSIGHT_LAYER,
  };

  static constexpr unsigned IBP_NONE = 0x7000;
  static constexpr unsigned IBP_AUTO = 0x7001;
  static constexpr unsigned OVERLAY_NONE = 0;
  static constexpr unsigned OVERLAY_RASP = 1;
  static constexpr unsigned OVERLAY_EDL = 2;
  static constexpr unsigned OVERLAY_XCTHERM = 3;
  static constexpr unsigned OVERLAY_SKYSIGHT = 4;

  PageLayout value;

  Listener &listener;

  unsigned GetOverlayValue() const noexcept;
  void UpdateOverlayControls() noexcept;
  void FillRaspFieldControl() noexcept;
  void FillSkysightLayerControl() noexcept;
  void ApplyValueToForm() noexcept;

public:
  PageLayoutEditWidget(const DialogLook &_look, Listener &_listener)
    :RowFormWidget(_look), value(PageLayout::Default()),
     listener(_listener) {}

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
PageLayoutEditWidget::FillSkysightLayerControl() noexcept
{
  auto &control = GetControl(SKYSIGHT_LAYER);
  auto &df = (DataFieldEnum &)*control.GetDataField();
  df.ClearChoices();

#ifdef HAVE_HTTP
  const auto skysight = DataGlobals::GetSkysight();
  if (skysight != nullptr) {
    unsigned selected_value = 1;
    bool has_choices = false;
    bool stored_layer_is_selected = false;

    for (std::size_t i = 0; i < skysight->NumSelectedLayers(); ++i)
      if (const auto *layer = skysight->GetSelectedLayer(i);
          layer != nullptr && layer->id == value.skysight_overlay.c_str()) {
        stored_layer_is_selected = true;
        selected_value = unsigned(i + 1);
        break;
      }

    if (!value.skysight_overlay.empty() && !stored_layer_is_selected) {
      df.AddChoice(0, value.skysight_overlay.c_str(), value.skysight_overlay.c_str());
      has_choices = true;
      selected_value = 0;
    }

    for (std::size_t i = 0; i < skysight->NumSelectedLayers(); ++i) {
      const auto *layer = skysight->GetSelectedLayer(i);
      if (layer == nullptr)
        continue;

      const unsigned choice_value = unsigned(i + 1);
      df.AddChoice(choice_value, layer->name.c_str());

      has_choices = true;
    }

    if (has_choices) {
      df.SetValue(selected_value);
      control.RefreshDisplay();
      return;
    }
  }
#endif

  df.AddChoice(0, _("No SkySight layers selected"));
  df.SetValue(0U);
  control.RefreshDisplay();
}

unsigned
PageLayoutEditWidget::GetOverlayValue() const noexcept
{
  switch (value.overlay) {
  case PageLayout::Overlay::NONE:
    return OVERLAY_NONE;

  case PageLayout::Overlay::RASP:
    return OVERLAY_RASP;

  case PageLayout::Overlay::EDL:
    return OVERLAY_EDL;

  case PageLayout::Overlay::XCTHERM:
    return OVERLAY_XCTHERM;

  case PageLayout::Overlay::SKYSIGHT:
    return OVERLAY_SKYSIGHT;

  case PageLayout::Overlay::MAX:
    gcc_unreachable();
  }

  return OVERLAY_NONE;
}

void
PageLayoutEditWidget::UpdateOverlayControls() noexcept
{
  const bool map_page = value.IsMapMain();
  const auto rasp = DataGlobals::GetRasp();
  const bool rasp_available = rasp != nullptr && rasp->GetItemCount() > 0;
  const bool rasp_overlay = value.overlay == PageLayout::Overlay::RASP;
  const bool skysight_overlay = value.overlay == PageLayout::Overlay::SKYSIGHT;

  SetRowEnabled(OVERLAY, map_page);
  SetRowEnabled(RASP_FIELD, map_page && rasp_overlay && rasp_available);
  SetRowEnabled(SKYSIGHT_LAYER, map_page && skysight_overlay);
}

void
PageLayoutEditWidget::ApplyValueToForm() noexcept
{
  LoadValueEnum(BOTTOM, value.bottom);
  GetControl(BOTTOM).RefreshDisplay();
  LoadValueEnum(OVERLAY, GetOverlayValue());
  GetControl(OVERLAY).RefreshDisplay();
  FillRaspFieldControl();
  FillSkysightLayerControl();
  UpdateOverlayControls();
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

  {
    WndProperty *wp = AddEnum(_("Map overlay"),
                              _("Optional weather overlay drawn on map pages."),
                              this);
    DataFieldEnum &overlay = *(DataFieldEnum *)wp->GetDataField();
    overlay.AddChoice(OVERLAY_NONE, _("None"));
    overlay.AddChoice(OVERLAY_RASP, _("RASP"));

#ifdef HAVE_EDL
    overlay.AddChoice(OVERLAY_EDL, _("EDL"));
#endif

#ifdef HAVE_HTTP
    overlay.AddChoice(OVERLAY_XCTHERM, _("XCTherm"));
#endif

#ifdef HAVE_HTTP
    overlay.AddChoice(OVERLAY_SKYSIGHT, "SkySight");
#endif
  }

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
#if defined(HAVE_EDL) || defined(ENABLE_OPENGL)
    { PageLayout::Bottom::WEATHER_CONTROLS, N_("Weather controls") },
#endif
    nullptr
  };
  AddEnum(_("Bottom area"),
          _("Specifies what should be displayed below the main area. "
            "Weather controls require a weather map overlay."),
          bottom_list,
          (unsigned)PageLayout::Bottom::NOTHING, this);
  AddEnum(_("RASP layer"),
          _("RASP weather layer to display on this map page."),
          this);
  GetControl(RASP_FIELD).GetDataField()->EnableItemHelp(true);
  AddEnum(_("SkySight layer"),
          _("SkySight layer used when this page overlay is SkySight."),
          this);
  FillRaspFieldControl();
  FillSkysightLayerControl();
  UpdateOverlayControls();
}

void
PageLayoutEditWidget::SetValue(const PageLayout &_value)
{
  value = _value;
  value.Normalise();

  LoadValueEnum(MAIN, value.main);
  LoadValueEnum(OVERLAY, GetOverlayValue());
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

  FillRaspFieldControl();
  FillSkysightLayerControl();
  UpdateOverlayControls();
}

void
PageLayoutEditWidget::OnModified(DataField &df) noexcept
{
  if (&df == &GetDataField(MAIN)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    value.main = (PageLayout::Main)dfe.GetValue();
    if (!value.IsMapMain()) {
      value.overlay = PageLayout::Overlay::NONE;
      value.skysight_overlay.clear();
    }
  } else if (&df == &GetDataField(OVERLAY)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    value.overlay = PageLayout::Overlay::NONE;
    value.skysight_overlay.clear();

    switch (dfe.GetValue()) {
    case OVERLAY_NONE:
      break;

    case OVERLAY_RASP:
      value.overlay = PageLayout::Overlay::RASP;
      FillRaspFieldControl();
      break;

    case OVERLAY_EDL:
      value.overlay = PageLayout::Overlay::EDL;
      break;

    case OVERLAY_XCTHERM:
      value.overlay = PageLayout::Overlay::XCTHERM;
      break;

    case OVERLAY_SKYSIGHT:
#ifdef HAVE_HTTP
      value.overlay = PageLayout::Overlay::SKYSIGHT;
      if (auto skysight = DataGlobals::GetSkysight(); skysight != nullptr)
        if (const auto *layer = skysight->GetSelectedLayer(0); layer != nullptr)
            value.skysight_overlay = layer->id;
#endif
      break;
    }
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

    if (value.bottom == PageLayout::Bottom::WEATHER_CONTROLS &&
        value.IsMapMain() &&
        !value.UsesWeatherOverlay()) {
#ifdef HAVE_EDL
      value.skysight_overlay.clear();
      value.overlay = PageLayout::Overlay::EDL;
#else
      const auto rasp = DataGlobals::GetRasp();
      if (rasp != nullptr && rasp->GetItemCount() > 0) {
        value.skysight_overlay.clear();
        value.overlay = PageLayout::Overlay::RASP;
      } else
        value.bottom = PageLayout::Bottom::NOTHING;
#endif
    }
  } else if (&df == &GetDataField(RASP_FIELD)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    value.rasp_field = dfe.GetValue();
  } else if (&df == &GetDataField(SKYSIGHT_LAYER)) {
#ifdef HAVE_HTTP
    const auto selected = ((const DataFieldEnum &)df).GetValue();
    const auto previous_layer_id = value.skysight_overlay;

    value.skysight_overlay.clear();

    if (auto skysight = DataGlobals::GetSkysight(); skysight != nullptr) {
      if (selected > 0) {
        if (const auto *layer = skysight->GetSelectedLayer(selected - 1);
            layer != nullptr)
          value.skysight_overlay = layer->id;
      } else if (previous_layer_id.empty()) {
        if (const auto *layer = skysight->GetSelectedLayer(0); layer != nullptr)
          value.skysight_overlay = layer->id;
      } else {
        bool previous_is_selected = false;
        for (std::size_t i = 0; i < skysight->NumSelectedLayers(); ++i)
          if (const auto *layer = skysight->GetSelectedLayer(i);
              layer != nullptr && layer->id == previous_layer_id.c_str()) {
            previous_is_selected = true;
            break;
          }

        if (!previous_is_selected)
          value.skysight_overlay = previous_layer_id;
        else if (const auto *layer = skysight->GetSelectedLayer(0); layer != nullptr)
          value.skysight_overlay = layer->id;
      }
    }
#endif
  } else {
    gcc_unreachable();
  }

  value.Normalise();
  ApplyValueToForm();
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
