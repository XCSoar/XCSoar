// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkysightDialog.hpp"

#ifdef HAVE_HTTP

#include "DataGlobals.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Dialogs/Message.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TextWidget.hpp"
#include "Weather/Skysight/Skysight.hpp"
#include "ui/event/PeriodicTimer.hpp"

#include "util/StaticString.hxx"

static StaticString<32>
FormatForecastTimeLabel(time_t forecast_time) noexcept
{
  StaticString<32> label;
  if (forecast_time <= 0)
    return label;

  label = FormatLocalDateTimeYYYYMMDDHHMM(
    TimeStamp(std::chrono::duration<double>(forecast_time)),
    CommonInterface::GetComputerSettings().utc_offset).c_str();
  return label;
}

class SelectedLayerRenderer {
  TwoTextRowsRenderer row_renderer;
  std::shared_ptr<Skysight> skysight;

public:
  SelectedLayerRenderer()
    :skysight(DataGlobals::GetSkysight()) {}

  unsigned CalculateLayout(const DialogLook &look) noexcept {
    return row_renderer.CalculateLayout(*look.list.font_bold,
                                        look.small_font);
  }

  void Draw(Canvas &canvas, const PixelRect &rc, unsigned index) noexcept {
    if (skysight == nullptr)
      return;

    if (skysight->NumSelectedLayers() == 0) {
      row_renderer.DrawFirstRow(canvas, rc, _("SkySight"));

      if (!skysight->HasCredentials())
        row_renderer.DrawSecondRow(canvas, rc,
                                   _("Configure SkySight credentials in Weather settings."));
      else if (!skysight->HasForecastLayers())
        row_renderer.DrawSecondRow(canvas, rc,
                                   _("Loading SkySight catalog..."));
      else
        row_renderer.DrawSecondRow(canvas, rc,
                                   _("No SkySight layers selected. Press Add to choose a parameter."));

      return;
    }

    if (index >= skysight->NumSelectedLayers())
      return;

    const auto *layer = skysight->GetSelectedLayer(index);
    if (layer == nullptr)
      return;

    StaticString<128> first_row;
    first_row = layer->name.c_str();
    if (skysight->GetActiveLayerId() == layer->id)
      first_row.AppendFormat(" [%s]", _("Active"));

    StaticString<256> second_row;
    if (layer->updating) {
      if (!layer->SupportsLiveTiles() && layer->forecast_datafiles.empty())
        second_row = _("Loading forecast steps...");
      else if (!layer->SupportsLiveTiles())
        second_row = _("Forecast steps loaded. Downloading data...");
      else
        second_row = _("Updating...");
    } else if (layer->SupportsLiveTiles()) {
      if (layer->last_update != 0) {
        const auto &settings = CommonInterface::GetComputerSettings();
        second_row.Format(_("Live layer. Last update %s"),
                          FormatLocalTimeHHMM(TimeStamp(std::chrono::duration<double>(layer->last_update)),
                                              settings.utc_offset).c_str());
      } else {
        second_row = _("Live tile layer.");
      }
    } else if (layer->forecast_datafiles.empty()) {
      second_row = _("No forecast steps available yet.");
    } else if (layer->mtime == 0) {
      if (!skysight->IsForecastDecodeAvailable())
        second_row = _("Forecast steps available, but this build has no NetCDF decode support.");
      else
        second_row = _("Forecast steps available. Activate or choose Time to download one.");
    } else {
      const auto &settings = CommonInterface::GetComputerSettings();
      const auto now = std::time(nullptr);
      const auto age = std::chrono::seconds(now > (time_t)layer->mtime
                                            ? now - (time_t)layer->mtime
                                            : 0);
      if (layer->forecast_time != 0) {
        second_row.Format(_("Step %s. Data from %s to %s. Updated %s ago"),
                          FormatForecastTimeLabel(layer->forecast_time).c_str(),
                          FormatLocalTimeHHMM(TimeStamp(std::chrono::duration<double>(layer->from)),
                                              settings.utc_offset).c_str(),
                          FormatLocalTimeHHMM(TimeStamp(std::chrono::duration<double>(layer->to)),
                                              settings.utc_offset).c_str(),
                          FormatTimespanSmart(age).c_str());
      } else {
        second_row.Format(_("Data from %s to %s. Updated %s ago"),
                          FormatLocalTimeHHMM(TimeStamp(std::chrono::duration<double>(layer->from)),
                                              settings.utc_offset).c_str(),
                          FormatLocalTimeHHMM(TimeStamp(std::chrono::duration<double>(layer->to)),
                                              settings.utc_offset).c_str(),
                          FormatTimespanSmart(age).c_str());
      }
    }

    row_renderer.DrawFirstRow(canvas, rc, first_row.c_str());
    row_renderer.DrawSecondRow(canvas, rc, second_row.c_str());
  }
};

class LayerPickerRenderer final : public ListItemRenderer {
  TextRowRenderer row_renderer;
  std::shared_ptr<Skysight> skysight;

public:
  LayerPickerRenderer()
    :skysight(DataGlobals::GetSkysight()) {}

  unsigned CalculateLayout(const DialogLook &look) noexcept {
    return row_renderer.CalculateLayout(*look.list.font);
  }

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned index) noexcept override {
    if (skysight == nullptr || index >= skysight->NumLayers())
      return;

    if (const auto *layer = skysight->GetLayer(index); layer != nullptr)
      row_renderer.DrawTextRow(canvas, rc, layer->name.c_str());
  }

  static const char *HelpCallback(unsigned index) noexcept {
    static StaticString<512> help;

    const auto skysight = DataGlobals::GetSkysight();
    if (!skysight || index >= skysight->NumLayers()) {
      help = _("No description available.");
      return help.c_str();
    }

    const auto *layer = skysight->GetLayer(index);
    if (layer == nullptr || layer->description.empty())
      help = _("No description available.");
    else
      help = layer->description.c_str();

    return help.c_str();
  }
};

class ForecastStepRenderer final : public ListItemRenderer {
  TextRowRenderer row_renderer;
  std::vector<time_t> forecast_times;

public:
  explicit ForecastStepRenderer(const SkySight::Layer &layer) {
    forecast_times.reserve(layer.forecast_datafiles.size());
    for (const auto &datafile : layer.forecast_datafiles)
      forecast_times.push_back(datafile.time);
  }

  unsigned CalculateLayout(const DialogLook &look) noexcept {
    return row_renderer.CalculateLayout(*look.list.font);
  }

  [[nodiscard]] time_t GetForecastTime(unsigned index) const noexcept {
    return index < forecast_times.size() ? forecast_times[index] : 0;
  }

  [[nodiscard]] unsigned FindForecastTime(time_t forecast_time) const noexcept {
    for (unsigned i = 0; i < forecast_times.size(); ++i)
      if (forecast_times[i] == forecast_time)
        return i;

    return 0;
  }

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned index) noexcept override {
    if (index >= forecast_times.size())
      return;

    row_renderer.DrawTextRow(canvas, rc,
                             FormatForecastTimeLabel(forecast_times[index]).c_str());
  }
};

class SkysightWidget final : public ListWidget {
  std::shared_ptr<Skysight> skysight;
  ButtonPanelWidget *buttons_widget = nullptr;
  Button *activate_button = nullptr;
  Button *add_button = nullptr;
  Button *time_button = nullptr;
  Button *remove_button = nullptr;
  SelectedLayerRenderer row_renderer;
  UI::PeriodicTimer update_timer{[this]{ UpdateList(); }};

public:
  explicit SkysightWidget(std::shared_ptr<Skysight> &&_skysight)
    :skysight(std::move(_skysight)) {}

  void SetButtonPanel(ButtonPanelWidget &_buttons) noexcept {
    buttons_widget = &_buttons;
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    CreateButtons(buttons_widget->GetButtonPanel());
    const DialogLook &look = UIGlobals::GetDialogLook();
    CreateList(parent, look, rc, row_renderer.CalculateLayout(look));
    UpdateList();
    update_timer.Schedule(std::chrono::seconds(1));
  }

  void Unprepare() noexcept override {
    update_timer.Cancel();
    DeleteWindow();
  }

protected:
  bool CanActivateItem(unsigned i) const noexcept override {
    return skysight != nullptr && i < skysight->NumSelectedLayers();
  }

  void OnActivateItem(unsigned i) noexcept override {
    if (skysight == nullptr || i >= skysight->NumSelectedLayers())
      return;

    const auto *layer = skysight->GetSelectedLayer(i);
    if (layer != nullptr && skysight->GetActiveLayerId() == layer->id)
      DeactivateClicked();
    else
      ActivateClicked(i);
  }

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override {
    row_renderer.Draw(canvas, rc, idx);
  }

private:
  void CreateButtons(ButtonPanel &buttons) {
    activate_button = buttons.Add(_("Activate"), [this]() {
      ActivateClicked(GetList().GetCursorIndex());
    });
    add_button = buttons.Add(_("Add"), [this]() {
      AddClicked();
    });
    time_button = buttons.Add(_("Time"), [this]() {
      SelectTimeClicked();
    });
    remove_button = buttons.Add(_("Remove"), [this]() {
      RemoveClicked();
    });
    buttons.EnableCursorSelection();
  }

  void UpdateButtons() {
    if (activate_button == nullptr || add_button == nullptr ||
        time_button == nullptr || remove_button == nullptr)
      return;

    const auto empty = skysight == nullptr || skysight->NumSelectedLayers() == 0;
    const auto catalog_loading = skysight != nullptr && skysight->HasCredentials() &&
      !skysight->HasForecastLayers();
    add_button->SetCaption(catalog_loading ? _("Loading") : _("Add"));
    add_button->SetEnabled(skysight != nullptr && !catalog_loading &&
                           !skysight->SelectedLayersFull() &&
                           skysight->NumLayers() > 0);

    const auto index = empty ? 0u : GetList().GetCursorIndex();
    const auto *layer = empty ? nullptr : skysight->GetSelectedLayer(index);
    const auto active = layer != nullptr && skysight->GetActiveLayerId() == layer->id;

    time_button->SetEnabled(layer != nullptr && !layer->SupportsLiveTiles() &&
                            !layer->forecast_datafiles.empty() &&
                            (skysight->IsForecastDecodeAvailable() || layer->mtime != 0));
    remove_button->SetEnabled(layer != nullptr && !layer->updating);

    if (active) {
      activate_button->SetEnabled(true);
      activate_button->SetCaption(_("Deactivate"));
      activate_button->SetCallback([this]() { DeactivateClicked(); });
    } else {
      activate_button->SetEnabled(layer != nullptr);
      activate_button->SetCaption(_("Activate"));
      activate_button->SetCallback([this]() {
        ActivateClicked(GetList().GetCursorIndex());
      });
    }
  }

  void UpdateList() {
    if (skysight == nullptr)
      return;

    if (skysight->HasCredentials() && !skysight->HasForecastLayers())
      skysight->RefreshCatalog();

    GetList().SetLength(std::max<std::size_t>(1, skysight->NumSelectedLayers()));
    GetList().Invalidate();
    UpdateButtons();
  }

  void AddClicked() {
    if (skysight == nullptr)
      return;

    if (!skysight->HasCredentials()) {
      ShowMessageBox(
        _("Configure your SkySight credentials in Weather settings before loading the full SkySight catalog."),
        _("SkySight"), MB_OK);
      return;
    }

    if (!skysight->HasForecastLayers()) {
      skysight->RefreshCatalog();
      return;
    }

    LayerPickerRenderer renderer;
    const int index = ListPicker(_("Choose a parameter"),
                                 skysight->NumLayers(), 0,
                                 renderer.CalculateLayout(UIGlobals::GetDialogLook()),
                                 renderer,
                                 false,
                                 nullptr,
                                 &LayerPickerRenderer::HelpCallback);
    if (index < 0)
      return;

    const auto *layer = skysight->GetLayer(index);
    if (layer == nullptr)
      return;

    if (!skysight->AddSelectedLayer(layer->id)) {
      ShowMessageBox(_("The selected layer is already in the list or the list is full."),
                     _("SkySight"), MB_OK);
      return;
    }

    UpdateList();
    GetList().SetCursorIndex(skysight->NumSelectedLayers() - 1);
    GetList().Invalidate();
    UpdateButtons();
  }

  void ActivateClicked(unsigned index) {
    if (skysight == nullptr || index >= skysight->NumSelectedLayers())
      return;

    const auto *layer = skysight->GetSelectedLayer(index);
    if (layer == nullptr)
      return;

    if (layer->requires_auth && !skysight->HasCredentials()) {
      ShowMessageBox(
        _("Configure your SkySight credentials in Weather settings before enabling SkySight layers."),
        _("SkySight"), MB_OK);
      return;
    }

    if (!skysight->SetLayerActive(layer->id))
      ShowMessageBox(_("Couldn't display data."), _("Display Error"), MB_OK);

    UpdateList();
  }

  void SelectTimeClicked() {
    if (skysight == nullptr)
      return;

    const auto index = GetList().GetCursorIndex();
    if (index >= skysight->NumSelectedLayers())
      return;

    const auto *layer = skysight->GetSelectedLayer(index);
    if (layer == nullptr || layer->SupportsLiveTiles() || layer->forecast_datafiles.empty())
      return;

    ForecastStepRenderer renderer(*layer);
    const int selected = ListPicker(_("Choose a forecast time"),
                                    layer->forecast_datafiles.size(),
                                    renderer.FindForecastTime(layer->forecast_time),
                                    renderer.CalculateLayout(UIGlobals::GetDialogLook()),
                                    renderer);
    if (selected < 0)
      return;

    if (!skysight->SelectForecastTime(layer->id, renderer.GetForecastTime(selected)))
      ShowMessageBox(_("Couldn't load the selected time step."),
                     _("SkySight"), MB_OK);

    UpdateList();
  }

  void DeactivateClicked() {
    if (skysight != nullptr)
      skysight->DeactivateLayer();

    UpdateList();
  }

  void RemoveClicked() {
    if (skysight == nullptr)
      return;

    const auto index = GetList().GetCursorIndex();
    if (index >= skysight->NumSelectedLayers())
      return;

    const auto *layer = skysight->GetSelectedLayer(index);
    if (layer == nullptr)
      return;

    StaticString<256> prompt;
    prompt.Format(_("Do you want to remove \"%s\"?"), layer->name.c_str());
    if (ShowMessageBox(prompt, _("Remove"), MB_YESNO) == IDNO)
      return;

    if (skysight->GetActiveLayerId() == layer->id)
      skysight->DeactivateLayer();

    (void)skysight->RemoveSelectedLayer(layer->id);

    UpdateList();
  }
};

std::unique_ptr<Widget>
CreateSkysightWidget()
{
  auto skysight = DataGlobals::GetSkysight();
  if (!skysight) {
    auto widget = std::make_unique<TextWidget>();
    widget->SetText(_("SkySight is unavailable."));
    return widget;
  }

  auto buttons = std::make_unique<ButtonPanelWidget>(
    std::make_unique<SkysightWidget>(std::move(skysight)),
    ButtonPanelWidget::Alignment::BOTTOM);
  static_cast<SkysightWidget &>(buttons->GetWidget()).SetButtonPanel(*buttons);
  return buttons;
}

#endif
