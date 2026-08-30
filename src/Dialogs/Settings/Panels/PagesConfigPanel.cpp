// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PagesConfigPanel.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Look/DialogLook.hpp"
#include "util/StaticArray.hxx"
#include "util/Compiler.h"
#include "util/StringFormat.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "PageActions.hpp"
#include "PageSetting.hpp"
#include "PageSettingDescriptor.hpp"
#include "Language/Language.hpp"
#include "Profile/PageProfile.hpp"
#include "Profile/Current.hpp"
#include "Interface.hpp"
#include "DataGlobals.hpp"
#include "Weather/Features.hpp"
#include "Weather/Rasp/FieldControls.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#ifdef HAVE_EDL
#include "Formatter/UserUnits.hpp"
#include "Weather/EDL/Levels.hpp"
#include "Weather/EDL/StateController.hpp"
#endif
#include "Widget/RowFormWidget.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Widget/VScrollWidget.hpp"
#include "UIGlobals.hpp"
#include "util/StaticString.hxx"

#ifdef HAVE_HTTP
#include "Weather/SkySight/SkySightClient.hpp"
#endif

#include <cassert>

/* this macro exists in the WIN32 API */
#ifdef DELETE
#undef DELETE
#endif

static constexpr unsigned CHOICE_GLOBAL = 0xffff;

class PageCustomSettingsWidget;

static void
ShowPageCustomSettingsDialog(PageSettingOverrides &overrides) noexcept;

/**
 * Hosts a scrollable custom-settings form and can remeasure after rows
 * are shown or hidden.
 */
class PageCustomSettingsHost final : public NullWidget {
  std::unique_ptr<VScrollWidget> scroll;
  PageCustomSettingsWidget *form = nullptr;
  PixelRect position{};
  bool visible = false;

public:
  PageCustomSettingsHost(const DialogLook &look,
                         PageSettingOverrides &overrides) noexcept;

  void RefreshLayout() noexcept {
    if (visible)
      scroll->Move(position);
  }

  PageCustomSettingsWidget &GetForm() noexcept {
    return *form;
  }

  PixelSize GetMinimumSize() const noexcept override {
    return scroll->GetMinimumSize();
  }

  PixelSize GetMaximumSize() const noexcept override {
    return scroll->GetMaximumSize();
  }

  void Initialise(ContainerWindow &parent,
                  const PixelRect &rc) noexcept override {
    position = rc;
    scroll->Initialise(parent, rc);
  }

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    position = rc;
    scroll->Prepare(parent, rc);
  }

  void Unprepare() noexcept override {
    scroll->Unprepare();
  }

  bool Save(bool &changed) noexcept override {
    return scroll->Save(changed);
  }

  void Show(const PixelRect &rc) noexcept override {
    position = rc;
    visible = true;
    scroll->Show(rc);
  }

  void Hide() noexcept override {
    visible = false;
    scroll->Hide();
  }

  void Move(const PixelRect &rc) noexcept override {
    position = rc;
    scroll->Move(rc);
  }

  bool SetFocus() noexcept override {
    return scroll->SetFocus();
  }

  bool HasFocus() const noexcept override {
    return scroll->HasFocus();
  }

  bool KeyPress(unsigned key_code) noexcept override {
    return scroll->KeyPress(key_code);
  }
};

class PageCustomSettingsWidget final
  : public RowFormWidget, private DataFieldListener {
  PageSettingOverrides &overrides;
  PageCustomSettingsHost *host = nullptr;
  Button *add_button = nullptr;
  Button *delete_button = nullptr;
  int selected_control = -1;

  void FillControl(PageSettingId id, unsigned control) noexcept;
  void SyncRows() noexcept;
  void UpdateActionButtons() noexcept;
  void OnAddClicked() noexcept;
  void OnDeleteClicked() noexcept;
  void SelectControl(unsigned control) noexcept;

public:
  PageCustomSettingsWidget(const DialogLook &_look,
                           PageSettingOverrides &_overrides) noexcept
    :RowFormWidget(_look), overrides(_overrides) {}

  void SetHost(PageCustomSettingsHost &_host) noexcept {
    host = &_host;
  }

  void SetActionButtons(Button &_add, Button &_delete) noexcept {
    add_button = &_add;
    delete_button = &_delete;
    UpdateActionButtons();
  }

  void AddClicked() noexcept {
    OnAddClicked();
  }

  void DeleteClicked() noexcept {
    OnDeleteClicked();
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

private:
  void OnModified(DataField &df) noexcept override;
};

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
    OVERLAY_DETAIL,
    CUSTOM_SETTINGS,
  };

  static constexpr unsigned IBP_NONE = 0x7000;
  static constexpr unsigned IBP_AUTO = 0x7001;

  PageLayout value;
  PageSettingOverrides *overrides = nullptr;
  unsigned page_index = 0;

  Listener &listener;

  void UpdateOverlayControls() noexcept;
  void FillOverlayDetailControl() noexcept;
  void ApplyValueToForm() noexcept;
  void UpdateCustomSettingsButton() noexcept;
  void OnCustomSettingsClicked() noexcept;

public:
  PageLayoutEditWidget(const DialogLook &_look, Listener &_listener)
    :RowFormWidget(_look), value(PageLayout::Default()),
     listener(_listener) {}

  void SetValue(unsigned index, const PageLayout &_value,
                PageSettingOverrides &_overrides);

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
    add_button = buttons.Add(C_("Button", "Add"), [this](){
      const unsigned n = GetList().GetLength();
      if (n < PageSettings::MAX_PAGES) {
        auto &page = settings.pages[n];
        page = PageLayout::Default();
        settings.overrides[n].Clear();
        GetList().SetLength(n + 1);
        GetList().SetCursorIndex(n);
      }
    });

    delete_button = buttons.Add(C_("Button", "Delete"), [this](){
      const unsigned n = GetList().GetLength();
      const unsigned cursor = GetList().GetCursorIndex();
      if (n >= 2 && GetList().GetCursorIndex() < n) {
        std::copy(settings.pages.begin() + cursor + 1,
                  settings.pages.begin() + n,
                  settings.pages.begin() + cursor);
        std::copy(settings.overrides.begin() + cursor + 1,
                  settings.overrides.begin() + n,
                  settings.overrides.begin() + cursor);
        settings.overrides[n - 1].Clear();
        GetList().SetLength(n - 1);

        if (cursor == n - 1)
          GetList().SetCursorIndex(cursor - 1);
        else
          editor->SetValue(cursor, settings.pages[cursor],
                           settings.overrides[cursor]);
      }
    });

    move_up_button = buttons.AddSymbol("^", [this](){
      const unsigned cursor = GetList().GetCursorIndex();
      if (cursor > 0) {
        std::swap(settings.pages[cursor], settings.pages[cursor - 1]);
        std::swap(settings.overrides[cursor],
                  settings.overrides[cursor - 1]);
        GetList().SetCursorIndex(cursor - 1);
      }
    });

    move_down_button = buttons.AddSymbol("v", [this](){
      const unsigned n = GetList().GetLength();
      const unsigned cursor = GetList().GetCursorIndex();
      if (cursor + 1 < n) {
        std::swap(settings.pages[cursor], settings.pages[cursor + 1]);
        std::swap(settings.overrides[cursor],
                  settings.overrides[cursor + 1]);
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
PageLayoutEditWidget::FillOverlayDetailControl() noexcept
{
  auto &control = GetControl(OVERLAY_DETAIL);
  auto &df = (DataFieldEnum &)*control.GetDataField();

  df.ClearChoices();

  switch (value.overlay) {
  case PageLayout::Overlay::RASP: {
    control.SetCaption(_("RASP Layer"));
    control.SetHelpText(
      _("RASP weather layer to display on this map page."));

    const auto rasp = DataGlobals::GetRasp();
    if (rasp == nullptr || rasp->GetItemCount() == 0) {
      df.AddChoice(-1, _("No RASP file loaded"));
      df.SetValue(-1);
      break;
    }

    Rasp::FillFieldChoices(df, rasp.get());

    if (value.rasp_field >= 0 &&
        unsigned(value.rasp_field) < rasp->GetItemCount())
      df.SetValue(value.rasp_field);
    else
      df.SetValue(0U);
    break;
  }

#ifdef HAVE_EDL
  case PageLayout::Overlay::EDL:
    control.SetCaption(_("EDL Level"));
    control.SetHelpText(
      _("EDL pressure level / altitude band for this map page. "
        "Auto follows aircraft altitude when the page is opened."));

    df.AddChoice(0, C_("Weather control", "Auto"),
                 _("Follow altitude on page enter (auto level)."));

    for (unsigned i = 0; i < EDL::NUM_ISOBARS; ++i) {
      const unsigned isobar = EDL::ISOBARS[i];
      char alt[32];
      FormatUserAltitude(EDL::GetAltitudeForIsobar(isobar), alt);

      StaticString<64> label;
      if (alt[0] != '\0')
        label.Format("%u hPa (%s)", isobar / 100, alt);
      else
        label.Format("%u hPa", isobar / 100);

      df.AddChoice(int(isobar), label.c_str());
    }

    if (value.edl_isobar > 0 &&
        EDL::IsSupportedIsobar(unsigned(value.edl_isobar)))
      df.SetValue(unsigned(value.edl_isobar));
    else
      df.SetValue(0U);
    break;
#endif

  case PageLayout::Overlay::SKYSIGHT: {
    control.SetCaption(C_("Setting", "SkySight layer"));
    control.SetHelpText(
      _("SkySight layer used when this page overlay is SkySight."));

#ifdef HAVE_HTTP
    const auto skysight = DataGlobals::GetSkySight();
    if (skysight != nullptr) {
      unsigned selected_value = 1;
      bool has_choices = false;
      bool stored_layer_is_selected = false;

      for (std::size_t i = 0; i < skysight->NumSelectedLayers(); ++i)
        if (const auto *layer = skysight->GetSelectedLayer(i);
            layer != nullptr &&
            layer->id == value.skysight_overlay.c_str()) {
          stored_layer_is_selected = true;
          selected_value = unsigned(i + 1);
          break;
        }

      if (!value.skysight_overlay.empty() && !stored_layer_is_selected) {
        df.AddChoice(0, value.skysight_overlay.c_str(),
                     value.skysight_overlay.c_str());
        has_choices = true;
        selected_value = 0;
      }

      for (std::size_t i = 0; i < skysight->NumSelectedLayers(); ++i) {
        const auto *layer = skysight->GetSelectedLayer(i);
        if (layer == nullptr)
          continue;

        df.AddChoice(unsigned(i + 1), layer->name.c_str());
        has_choices = true;
      }

      if (has_choices) {
        df.SetValue(selected_value);
        break;
      }
    }
#endif

    df.AddChoice(0, _("No SkySight layers selected"));
    df.SetValue(0U);
    break;
  }

  case PageLayout::Overlay::NONE:
  case PageLayout::Overlay::XCTHERM:
#ifndef HAVE_EDL
  case PageLayout::Overlay::EDL:
#endif
  case PageLayout::Overlay::MAX:
    control.SetCaption(C_("Setting", "Layer / Level"));
    control.SetHelpText(
      _("Select a RASP or EDL map overlay to configure its "
        "layer or level for this page."));
    df.AddChoice(-1, _("N/A"));
    df.SetValue(-1);
    break;
  }

  control.RefreshDisplay();
}

void
PageLayoutEditWidget::UpdateOverlayControls() noexcept
{
  const bool map_page = value.IsMapMain();
  bool detail_enabled = false;

  if (map_page) {
    switch (value.overlay) {
    case PageLayout::Overlay::RASP: {
      const auto rasp = DataGlobals::GetRasp();
      detail_enabled = rasp != nullptr && rasp->GetItemCount() > 0;
      break;
    }
#ifdef HAVE_EDL
    case PageLayout::Overlay::EDL:
      detail_enabled = true;
      break;
#endif
    case PageLayout::Overlay::SKYSIGHT:
#ifdef HAVE_HTTP
      detail_enabled = DataGlobals::GetSkySight() != nullptr;
#endif
      break;
    case PageLayout::Overlay::NONE:
    case PageLayout::Overlay::XCTHERM:
#ifndef HAVE_EDL
    case PageLayout::Overlay::EDL:
#endif
    case PageLayout::Overlay::MAX:
      break;
    }
  }

  SetRowEnabled(OVERLAY, map_page);
  SetRowEnabled(OVERLAY_DETAIL, detail_enabled);
}

void
PageLayoutEditWidget::ApplyValueToForm() noexcept
{
  LoadValueEnum(BOTTOM, value.bottom);
  GetControl(BOTTOM).RefreshDisplay();
  LoadValueEnum(OVERLAY, value.overlay);
  GetControl(OVERLAY).RefreshDisplay();
  FillOverlayDetailControl();
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
    { PageLayout::Main::FLARM_RADAR, N_("FLARM Radar") },
    { PageLayout::Main::THERMAL_ASSISTANT, N_("Thermal Assistant") },
    { PageLayout::Main::HORIZON, N_("Horizon") },
    nullptr
  };
  AddEnum(_("Main area"),
          _("Specifies what should be displayed in the main area."),
          main_list,
          (unsigned)PageLayout::Main::MAP, this);

  static constexpr StaticEnumChoice ib_list[] = {
    { IBP_AUTO, NC_("Setting", "Auto"), N_("Displays either the Circling, Cruise, or Final glide InfoBoxes.") },
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
    /* Always available: RASP does not require OpenGL, and the shared
       weather cursor bar works for RASP on memory canvas / Kobo. */
    { PageLayout::Bottom::WEATHER_CONTROLS, NC_("Setting", "Weather controls") },
    nullptr
  };
  AddEnum(_("Bottom area"),
          _("Specifies what should be displayed below the main area. "
            "Weather controls require a weather map "
            "overlay."),
          bottom_list,
          (unsigned)PageLayout::Bottom::NOTHING, this);

  static constexpr StaticEnumChoice overlay_list[] = {
    { PageLayout::Overlay::NONE, N_("None") },
    { PageLayout::Overlay::RASP, NC_("Abbreviation", "RASP") },
#ifdef HAVE_EDL
    { PageLayout::Overlay::EDL, NC_("Abbreviation", "EDL") },
#endif
#ifdef HAVE_HTTP
    { PageLayout::Overlay::XCTHERM, "XC Therm" },
    { PageLayout::Overlay::SKYSIGHT, "SkySight" },
#endif
    nullptr
  };
  AddEnum(C_("Setting", "Map overlay"),
          _("Optional weather overlay on map pages. "
            "Use with Weather controls in the bottom area for in-flight adjustment."),
          overlay_list,
          (unsigned)PageLayout::Overlay::NONE, this);

  AddEnum(C_("Setting", "Layer / Level"),
          _("Select a weather map overlay to configure its "
            "layer or level for this page."),
          this);
  GetControl(OVERLAY_DETAIL).GetDataField()->EnableItemHelp(true);
  FillOverlayDetailControl();
  UpdateOverlayControls();

  AddButton(_("Custom settings"), [this](){
    OnCustomSettingsClicked();
  });
  UpdateCustomSettingsButton();
}

void
PageLayoutEditWidget::SetValue(unsigned index, const PageLayout &_value,
                               PageSettingOverrides &_overrides)
{
  page_index = index;
  overrides = &_overrides;
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

  FillOverlayDetailControl();
  UpdateOverlayControls();
  UpdateCustomSettingsButton();
}

void
PageLayoutEditWidget::UpdateCustomSettingsButton() noexcept
{
  StaticString<64> caption;
  const unsigned n = overrides != nullptr ? overrides->n_items : 0;
  if (n == 0)
    caption = _("Custom settings");
  else
    caption.Format("%s (%u)", _("Custom settings"), n);

  auto &button = (Button &)GetRow(CUSTOM_SETTINGS);
  button.SetCaption(caption);
}

void
PageLayoutEditWidget::OnCustomSettingsClicked() noexcept
{
  if (overrides == nullptr)
    return;

  ShowPageCustomSettingsDialog(*overrides);
  UpdateCustomSettingsButton();
}

void
PageCustomSettingsWidget::FillControl(PageSettingId id,
                                      unsigned control) noexcept
{
  auto &df = (DataFieldEnum &)GetDataField(control);
  df.ClearChoices();
  df.AddChoice(CHOICE_GLOBAL, _("Global setting"));

  const auto &desc = PageSettingRegistry::Get(id);
  if (desc.type == PageSettingType::INTEGER) {
    char label[16];
    for (int v = desc.int_min; v <= desc.int_max; v += desc.int_step) {
      StringFormat(label, sizeof(label), "%d %%", v);
      df.AddChoice(unsigned(v), label);
    }
  } else {
    assert(desc.choices != nullptr);
    for (const StaticEnumChoice *c = desc.choices;
         c->display_string != nullptr; ++c)
      df.AddChoice(c->id, gettext(c->display_string), nullptr,
                   c->help != nullptr ? gettext(c->help) : nullptr);
  }

  unsigned selected = CHOICE_GLOBAL;
  if (const int *v = overrides.FindValue(id); v != nullptr &&
      *v != PageSettingOverrides::INHERIT)
    selected = unsigned(*v);

  df.SetValue(selected);
  GetControl(control).RefreshDisplay();
}

void
PageCustomSettingsWidget::UpdateActionButtons() noexcept
{
  if (add_button != nullptr) {
    bool can_add = false;
    for (unsigned i = 0; i < PageSettingRegistry::Count(); ++i)
      if (!overrides.Contains(PageSettingId(i))) {
        can_add = true;
        break;
      }
    add_button->SetEnabled(can_add);
  }

  if (delete_button != nullptr)
    delete_button->SetEnabled(selected_control >= 0 &&
                              overrides.Contains(PageSettingId(selected_control)));
}

void
PageCustomSettingsWidget::SelectControl(unsigned control) noexcept
{
  if (selected_control >= 0 &&
      unsigned(selected_control) != control)
    GetControl(unsigned(selected_control)).SetCaptionSelected(false);

  selected_control = int(control);
  GetControl(control).SetCaptionSelected(true);
  GetControl(control).SetFocus();
  UpdateActionButtons();
}

void
PageCustomSettingsWidget::SyncRows() noexcept
{
  for (unsigned i = 0; i < PageSettingRegistry::Count(); ++i) {
    const auto id = PageSettingId(i);
    const bool present = overrides.Contains(id);
    SetRowAvailable(i, present);
    if (present)
      FillControl(id, i);
    GetControl(i).SetCaptionSelected(false);
  }

  if (selected_control >= 0 &&
      !overrides.Contains(PageSettingId(selected_control)))
    selected_control = -1;

  UpdateLayout();
  if (host != nullptr)
    host->RefreshLayout();

  UpdateActionButtons();

  if (selected_control >= 0) {
    auto &control = GetControl(unsigned(selected_control));
    control.SetCaptionSelected(true);
    control.SetFocus();
  }
}

void
PageCustomSettingsWidget::OnAddClicked() noexcept
{
  ComboList list;
  StaticArray<PageSettingId, unsigned(PageSettingId::COUNT)> ids;
  for (unsigned i = 0; i < PageSettingRegistry::Count(); ++i) {
    const auto id = PageSettingId(i);
    if (overrides.Contains(id))
      continue;
    const auto &desc = PageSettingRegistry::Get(id);
    list.Append(ids.size(), gettext(desc.label));
    ids.append(id);
  }

  if (list.empty())
    return;

  const int result = ComboPicker(_("Add"), list, nullptr);
  if (result < 0 || unsigned(result) >= ids.size())
    return;

  const auto id = ids[result];
  overrides.Add(id, PageSettingOverrides::INHERIT);
  selected_control = int(id);
  SyncRows();
}

void
PageCustomSettingsWidget::OnDeleteClicked() noexcept
{
  if (selected_control < 0)
    return;

  const auto id = PageSettingId(selected_control);
  if (!overrides.Contains(id))
    return;

  overrides.Remove(id);
  selected_control = -1;
  SyncRows();
}

void
PageCustomSettingsWidget::Prepare(ContainerWindow &parent,
                                  const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  for (unsigned i = 0; i < PageSettingRegistry::Count(); ++i) {
    const auto &desc = PageSettingRegistry::Get(i);
    AddEnum(gettext(desc.label), gettext(desc.help), this);
    auto &control = GetControl(i);
    control.GetDataField()->EnableItemHelp(true);
    control.SetCaptionClickSelects(true, [this, i](){
      SelectControl(i);
    });
    SetRowAvailable(i, false);
  }

  SyncRows();
}

void
PageCustomSettingsWidget::OnModified(DataField &df) noexcept
{
  for (unsigned i = 0; i < PageSettingRegistry::Count(); ++i) {
    if (&df != &GetDataField(i))
      continue;

    SelectControl(i);

    const auto id = PageSettingId(i);
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    const unsigned choice = dfe.GetValue();
    if (choice == CHOICE_GLOBAL)
      overrides.SetValue(id, PageSettingOverrides::INHERIT);
    else
      overrides.SetValue(id, int(choice));
    return;
  }

  gcc_unreachable();
}

static void
ShowPageCustomSettingsDialog(PageSettingOverrides &overrides) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Custom settings"));

  auto host = std::make_unique<PageCustomSettingsHost>(look, overrides);
  auto &form = host->GetForm();

  dialog.FinishPreliminary(std::move(host));
  Button *add = dialog.AddButton(_("Add"), [&form](){
    form.AddClicked();
  });
  Button *del = dialog.AddButton(_("Delete"), [&form](){
    form.DeleteClicked();
  });
  form.SetActionButtons(*add, *del);
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

PageCustomSettingsHost::PageCustomSettingsHost(const DialogLook &look,
                                               PageSettingOverrides &overrides) noexcept
{
  auto form_widget =
    std::make_unique<PageCustomSettingsWidget>(look, overrides);
  form = form_widget.get();
  scroll = std::make_unique<VScrollWidget>(std::move(form_widget), look);
  form->SetHost(*this);
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

    if (value.bottom == PageLayout::Bottom::WEATHER_CONTROLS &&
        value.IsMapMain() &&
        !value.UsesWeatherOverlay()) {
#ifdef HAVE_EDL
      value.overlay = PageLayout::Overlay::EDL;
#else
      const auto rasp = DataGlobals::GetRasp();
      if (rasp != nullptr && rasp->GetItemCount() > 0)
        value.overlay = PageLayout::Overlay::RASP;
      else
        value.bottom = PageLayout::Bottom::NOTHING;
#endif
    }
  } else if (&df == &GetDataField(OVERLAY)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    const auto overlay = (PageLayout::Overlay)dfe.GetValue();
    if (overlay == PageLayout::Overlay::SKYSIGHT) {
#ifdef HAVE_HTTP
      const auto skysight = DataGlobals::GetSkySight();
      const SkySight::Layer *layer = nullptr;
      if (skysight != nullptr) {
        if (!value.skysight_overlay.empty() &&
            skysight->IsSelectedLayer(value.skysight_overlay.c_str()))
          layer = skysight->GetSelectedLayer(value.skysight_overlay.c_str());

        for (std::size_t i = 0; layer == nullptr &&
             i < skysight->NumSelectedLayers(); ++i)
          layer = skysight->GetSelectedLayer(i);
      }

      if (layer != nullptr) {
        value.skysight_overlay = layer->id;
      } else if (value.skysight_overlay.empty()) {
        const char *message;
        if (skysight == nullptr)
          message = _("SkySight is unavailable.");
        else if (skysight->IsThrottled())
          message = _("SkySight API rate-limited. Retrying shortly.");
        else if (!skysight->HasForecastLayers())
          message = _("Loading SkySight catalog...");
        else
          message = _("No SkySight layers selected");

        ShowMessageBox(message, "SkySight", MB_OK | MB_ICONINFORMATION);
        ApplyValueToForm();
        return;
      }

      if (layer == nullptr)
        value.skysight_overlay.clear();
#else
      value.skysight_overlay.clear();
#endif
    }
    value.overlay = overlay;
  } else if (&df == &GetDataField(OVERLAY_DETAIL)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    if (value.overlay == PageLayout::Overlay::RASP)
      value.rasp_field = dfe.GetValue();
#ifdef HAVE_EDL
    else if (value.overlay == PageLayout::Overlay::EDL)
      value.edl_isobar = dfe.GetValue();
#endif
    else if (value.overlay == PageLayout::Overlay::SKYSIGHT) {
#ifdef HAVE_HTTP
      if (auto skysight = DataGlobals::GetSkySight(); skysight != nullptr) {
        const unsigned selected = dfe.GetValue();

        bool stored_layer_is_selected = false;
        for (std::size_t i = 0; i < skysight->NumSelectedLayers(); ++i)
          if (const auto *layer = skysight->GetSelectedLayer(i);
              layer != nullptr &&
              layer->id == value.skysight_overlay.c_str()) {
            stored_layer_is_selected = true;
            break;
          }

        if (selected == 0 && !stored_layer_is_selected)
          return;

        if (selected > 0)
          if (const auto *layer =
                skysight->GetSelectedLayer(selected - 1);
              layer != nullptr &&
              value.skysight_overlay != layer->id.c_str()) {
            value.skysight_overlay = layer->id;
            value.skysight_time = PageLayout::SKYSIGHT_TIME_AUTO;
          }
      }
#endif
    }
  } else
    gcc_unreachable();

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
  const unsigned i = GetList().GetCursorIndex();
  editor->SetValue(i, settings.pages[i], settings.overrides[i]);

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
  for (unsigned i = settings.n_pages; i < PageSettings::MAX_PAGES; ++i)
    settings.overrides[i].Clear();

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

    if (settings.overrides[i] != _settings.overrides[i]) {
      Profile::Save(Profile::map, settings.overrides[i], i);
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

  editor->SetValue(idx, settings.pages[idx], settings.overrides[idx]);
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
    editor->SetValue(i, settings.pages[i], settings.overrides[i]);
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
