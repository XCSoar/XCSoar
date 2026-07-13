// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkysightDialog.hpp"

#ifdef HAVE_HTTP

#include "DataGlobals.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/Weather/OverlayPageActions.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Form/Button.hpp"
#include "Form/CheckBox.hpp"
#include "Form/ButtonPanel.hpp"
#include "Screen/Layout.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/MultiSelectListWidget.hpp"
#include "Widget/TextWidget.hpp"
#include "Weather/Skysight/Skysight.hpp"
#include "ui/event/PeriodicTimer.hpp"

#include "util/StaticString.hxx"

#include <algorithm>
#include <functional>

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
      else if (skysight->IsThrottled())
        row_renderer.DrawSecondRow(canvas, rc,
                                   _("SkySight API rate-limited. Retrying shortly."));
      else if (!skysight->HasForecastLayers())
        row_renderer.DrawSecondRow(canvas, rc,
                                   _("Loading SkySight catalog..."));
      else
        row_renderer.DrawSecondRow(canvas, rc,
                                   _("No SkySight layers selected. Press Select to choose parameters."));

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
      if (!layer->SupportsLiveTiles() && layer->datafiles_pending) {
        if (skysight->IsThrottled())
          second_row = _("Rate-limited by SkySight, retrying forecast steps shortly...");
        else
          second_row = _("Loading forecast steps...");
      }
      else if (!layer->SupportsLiveTiles() && layer->decoding)
        second_row = _("Decoding forecast data...");
      else if (!layer->SupportsLiveTiles() && layer->pending_downloads > 1)
        second_row.Format(_("Preloading %u forecast steps..."),
                          layer->pending_downloads);
      else if (!layer->SupportsLiveTiles())
        second_row = _("Downloading forecast data...");
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

class MultiLayerPickerWidget final : public MultiSelectListWidget {
  TwoTextRowsRenderer row_renderer;
  std::shared_ptr<Skysight> skysight;
  std::function<void()> selection_changed_callback;

public:
  explicit MultiLayerPickerWidget(std::shared_ptr<Skysight> _skysight)
    :skysight(std::move(_skysight)) {}

  void SetSelectionChangedCallback(std::function<void()> callback) noexcept {
    selection_changed_callback = std::move(callback);
  }

  unsigned CalculateLayout(const DialogLook &look) noexcept {
    return row_renderer.CalculateLayout(*look.list.font_bold,
                                        look.small_font);
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    const DialogLook &look = UIGlobals::GetDialogLook();
    CreateList(parent, look, rc, row_renderer.CalculateLayout(*look.list.font_bold,
                                                              look.small_font));
    SetLengthWithSelection(skysight ? skysight->NumLayers() : 0);
    MultiSelectListWidget::Prepare(parent, rc);

    if (skysight == nullptr)
      return;

    for (unsigned i = 0; i < skysight->NumLayers(); ++i) {
      const auto *layer = skysight->GetLayer(i);
      if (layer != nullptr && skysight->IsSelectedLayer(layer->id))
        SetSelected(i, true);
    }
  }

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override {
    if (skysight == nullptr || idx >= skysight->NumLayers())
      return;

    const auto *layer = skysight->GetLayer(idx);
    if (layer == nullptr)
      return;

    const DialogLook &look = UIGlobals::GetDialogLook();
    const bool focused = GetList().HasFocus();
    const unsigned padding = Layout::GetTextPadding();
    const unsigned box_size = rc.GetHeight() > 2 * padding
      ? rc.GetHeight() - 2 * padding
      : 0;

    PixelRect box_rc;
    box_rc.left = rc.left + (int)padding;
    box_rc.top = rc.top + (int)padding;
    box_rc.right = box_rc.left + (int)box_size;
    box_rc.bottom = box_rc.top + (int)box_size;

    DrawCheckBox(canvas, look, box_rc, IsSelected(idx), focused, false, true);

    PixelRect text_rc = rc;
    text_rc.left = box_rc.right + 2 * (int)padding;
    row_renderer.DrawFirstRow(canvas, text_rc, layer->name.c_str());
    row_renderer.DrawSecondRow(canvas, text_rc, layer->description.c_str());
  }

protected:
  void OnSelectionChanged() noexcept override {
    if (selection_changed_callback)
      selection_changed_callback();
  }
};

class SkysightWidget final : public ListWidget {
  std::shared_ptr<Skysight> skysight;
  ButtonPanelWidget *buttons_widget = nullptr;
  Button *activate_button = nullptr;
  Button *select_button = nullptr;
  Button *time_button = nullptr;
  Button *preload_button = nullptr;
  Button *preload_all_button = nullptr;
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
    select_button = buttons.Add(_("Select"), [this]() {
      SelectClicked();
    });
    time_button = buttons.Add(_("Time"), [this]() {
      SelectTimeClicked();
    });
    preload_button = buttons.Add(_("Preload"), [this]() {
      PreloadClicked();
    });
    preload_all_button = buttons.Add(_("Preload All"), [this]() {
      PreloadAllClicked();
    });
    buttons.Add(_("Add to page"), [this]() {
      AddToPageClicked(false);
    });
    buttons.Add(_("Add new page"), [this]() {
      AddToPageClicked(true);
    });
    buttons.EnableCursorSelection();
  }

  void UpdateButtons() {
    if (activate_button == nullptr || select_button == nullptr ||
        time_button == nullptr || preload_button == nullptr ||
        preload_all_button == nullptr)
      return;

    const auto empty = skysight == nullptr || skysight->NumSelectedLayers() == 0;
    const auto catalog_loading = skysight != nullptr && skysight->HasCredentials() &&
      !skysight->HasForecastLayers();

    select_button->SetCaption(catalog_loading ? _("Loading") : _("Select"));
    select_button->SetEnabled(skysight != nullptr && !catalog_loading &&
                  skysight->NumLayers() > 0);

    const auto index = empty ? 0u : GetList().GetCursorIndex();
    const auto *layer = empty ? nullptr : skysight->GetSelectedLayer(index);
    const auto active = layer != nullptr && skysight->GetActiveLayerId() == layer->id;

    bool any_forecast_layer = false;
    bool any_idle_forecast_layer = false;
    if (skysight != nullptr) {
      for (std::size_t i = 0; i < skysight->NumSelectedLayers(); ++i) {
        const auto *selected_layer = skysight->GetSelectedLayer(i);
        if (selected_layer == nullptr || selected_layer->SupportsLiveTiles())
          continue;

        any_forecast_layer = true;
        any_idle_forecast_layer = any_idle_forecast_layer || !selected_layer->updating;
      }
    }

    time_button->SetEnabled(layer != nullptr && !layer->SupportsLiveTiles() &&
                            !layer->forecast_datafiles.empty() &&
                            (skysight->IsForecastDecodeAvailable() || layer->mtime != 0));
    preload_button->SetEnabled(layer != nullptr && !layer->SupportsLiveTiles() &&
                               skysight->HasCredentials() && !layer->updating);
    preload_all_button->SetEnabled(skysight != nullptr && skysight->HasCredentials() &&
                                   any_forecast_layer && any_idle_forecast_layer);

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

    skysight->PollPendingDatafiles();

    if (skysight->HasCredentials() && !skysight->HasForecastLayers() && !skysight->IsThrottled())
      skysight->RefreshCatalog();

    GetList().SetLength(std::max<std::size_t>(1, skysight->NumSelectedLayers()));
    GetList().Invalidate();
    UpdateButtons();
  }

  void SelectClicked() {
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

    auto *picker = new MultiLayerPickerWidget(skysight);
    WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                        UIGlobals::GetDialogLook(), _("Select SkySight Layers"));
    dialog.AddButton(_("OK"), mrOK);

    Button *select_button = dialog.AddButton("", [](){});
    std::function<void()> update_buttons = [picker, select_button]() {
      select_button->SetCaption(picker->GetSelectedCount() == 0
                                ? _("Select all")
                                : _("Select none"));
    };
    select_button->SetCallback([picker, update_buttons]() mutable {
      if (picker->GetSelectedCount() == 0)
        picker->SelectAll();
      else
        picker->ClearSelection();
      update_buttons();
    });

    dialog.AddButton(_("Cancel"), mrCancel);
    dialog.FinishPreliminary(picker);

    update_buttons();
    picker->SetSelectionChangedCallback(update_buttons);

    if (dialog.ShowModal() != mrOK)
      return;

    const auto selected_indices = picker->GetSelectedIndices();

    std::vector<std::string> selected_ids;
    selected_ids.reserve(selected_indices.size());
    for (const auto idx : selected_indices) {
      const auto *layer = skysight->GetLayer(idx);
      if (layer != nullptr)
        selected_ids.emplace_back(layer->id);
    }

    std::vector<std::string> current_ids;
    current_ids.reserve(skysight->NumSelectedLayers());
    for (std::size_t i = 0; i < skysight->NumSelectedLayers(); ++i) {
      const auto *layer = skysight->GetSelectedLayer(i);
      if (layer != nullptr)
        current_ids.emplace_back(layer->id);
    }

    const auto active_layer_id = skysight->GetActiveLayerId();
    for (const auto &id : current_ids) {
      if (std::find(selected_ids.begin(), selected_ids.end(), id) != selected_ids.end())
        continue;

      if (active_layer_id == id)
        skysight->DeactivateLayer();

      (void)skysight->RemoveSelectedLayer(id);
    }

    bool add_failed = false;
    for (const auto &id : selected_ids) {
      if (skysight->IsSelectedLayer(id))
        continue;

      if (!skysight->AddSelectedLayer(id))
        add_failed = true;
    }

    if (add_failed)
      ShowMessageBox(_("Some selected layers couldn't be added (the list may be full)."),
                     _("SkySight"), MB_OK);

    UpdateList();
    if (skysight->NumSelectedLayers() > 0) {
      const auto max_index = (unsigned)(skysight->NumSelectedLayers() - 1);
      const auto current_index = GetList().GetCursorIndex();
      GetList().SetCursorIndex(std::min(current_index, max_index));
    }
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

  void ShowPreloadThrottledMessage() const {
    StaticString<256> message;
    message.Format(_("SkySight is rate-limited for about %s. Cached/older forecast data will remain visible. Please try preload again later."),
                   FormatTimespanSmart(std::chrono::seconds(
                     skysight->GetThrottleRemainingSeconds())).c_str());
    ShowMessageBox(message, _("SkySight"), MB_OK);
  }

  void PreloadClicked() {
    if (skysight == nullptr)
      return;

    const auto index = GetList().GetCursorIndex();
    if (index >= skysight->NumSelectedLayers())
      return;

    const auto *layer = skysight->GetSelectedLayer(index);
    if (layer == nullptr || layer->SupportsLiveTiles())
      return;

    const bool success = skysight->PreloadForecast(layer->id);
    if (skysight->IsThrottled()) {
      ShowPreloadThrottledMessage();
    } else if (!success) {
      ShowMessageBox(_("Couldn't preload forecast data."),
                     _("SkySight"), MB_OK);
    }

    UpdateList();
  }

  void PreloadAllClicked() {
    if (skysight == nullptr)
      return;

    const bool success = skysight->PreloadAllForecasts();
    if (skysight->IsThrottled()) {
      ShowPreloadThrottledMessage();
    } else if (!success) {
      ShowMessageBox(_("Couldn't preload forecast data."),
                     _("SkySight"), MB_OK);
    }

    UpdateList();
  }

  void DeactivateClicked() {
    if (skysight != nullptr)
      skysight->DeactivateLayer();

    UpdateList();
  }

  void AddToPageClicked(bool new_page) {
    if (skysight == nullptr)
      return;

    const auto index = GetList().GetCursorIndex();
    const auto *layer = index < skysight->NumSelectedLayers()
      ? skysight->GetSelectedLayer(index)
      : nullptr;
    if (layer == nullptr)
      return;

    if (new_page)
      WeatherDialogOverlayActions::AddOverlayToNewPage(
        PageLayout::Overlay::SKYSIGHT, -1, layer->id);
    else
      WeatherDialogOverlayActions::AddOverlayToCurrentPage(
        PageLayout::Overlay::SKYSIGHT, -1, layer->id);
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
