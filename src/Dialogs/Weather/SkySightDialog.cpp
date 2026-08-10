// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkySightDialog.hpp"

#ifdef HAVE_HTTP

#include "DataGlobals.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Weather/WeatherOverlayDraft.hpp"
#include "Formatter/ByteSizeFormatter.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Form/Button.hpp"
#include "Form/CheckBox.hpp"
#include "Form/ButtonPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/Edit.hpp"
#include "Screen/Layout.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Profile/Current.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/MultiSelectListWidget.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Widget/TextWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Weather/SkySight/SkySightClient.hpp"
#include "Weather/SkySight/ForecastFormatter.hpp"
#include "ui/event/PeriodicTimer.hpp"

#include "util/StaticString.hxx"

#include <algorithm>
#include <functional>

class SelectedLayerRenderer {
  TwoTextRowsRenderer row_renderer;
  std::shared_ptr<SkySightClient> skysight;

public:
  SelectedLayerRenderer()
    :skysight(DataGlobals::GetSkySight()) {}

  unsigned CalculateLayout(const DialogLook &look) noexcept {
    return row_renderer.CalculateLayout(*look.list.font_bold,
                                        look.small_font);
  }

  void Draw(Canvas &canvas, const PixelRect &rc, unsigned index) noexcept {
    if (skysight == nullptr)
      return;

    if (skysight->NumSelectedLayers() == 0) {
      row_renderer.DrawFirstRow(canvas, rc, "SkySight");

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
                                   _("No SkySight layers selected. Press Add to list to choose parameters."));

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
      first_row.AppendFormat(" [%s]", _("Current page"));

    StaticString<256> second_row;
    if (layer->ShouldShowUpdating()) {
      if (skysight->IsThrottled())
        second_row.Format(_("Download limit reached; retrying in %u seconds..."),
                          unsigned(skysight->GetThrottleRemainingSeconds()));
      else if (const auto retry = skysight->GetDatafilesRetryRemainingSeconds();
               retry > 0 && layer->HasPendingForecastMetadata())
        second_row.Format(_("Connection failed; retrying in %u seconds..."),
                          unsigned(retry));
      else if (!layer->SupportsLiveTiles() &&
               layer->HasPendingForecastMetadata())
        second_row = _("Loading forecast steps...");
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
        second_row.Format(_("Live layer. Last update %s"),
                          FormatLocalTimeHHMM(TimeStamp(std::chrono::duration<double>(layer->last_update)),
                                              skysight->GetForecastDisplayOffset(layer->last_update)).c_str());
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
      const auto now = std::time(nullptr);
      const auto age = std::chrono::seconds(now > layer->mtime
                                            ? now - layer->mtime
                                            : 0);
      if (layer->forecast_time != 0) {
        const auto step_offset =
          skysight->GetForecastDisplayOffset(layer->forecast_time);
        second_row.Format(_("Step %s. Data from %s to %s. Updated %s ago"),
                          SkySight::FormatForecastTimeLabel(
                            *layer, layer->forecast_time,
                            step_offset).c_str(),
                          FormatLocalTimeHHMM(TimeStamp(std::chrono::duration<double>(layer->from)),
                                              skysight->GetForecastDisplayOffset(layer->from)).c_str(),
                          FormatLocalTimeHHMM(TimeStamp(std::chrono::duration<double>(layer->to)),
                                              skysight->GetForecastDisplayOffset(layer->to)).c_str(),
                          FormatTimespanSmart(age).c_str());
      } else {
        second_row.Format(_("Data from %s to %s. Updated %s ago"),
                          FormatLocalTimeHHMM(TimeStamp(std::chrono::duration<double>(layer->from)),
                                              skysight->GetForecastDisplayOffset(layer->from)).c_str(),
                          FormatLocalTimeHHMM(TimeStamp(std::chrono::duration<double>(layer->to)),
                                              skysight->GetForecastDisplayOffset(layer->to)).c_str(),
                          FormatTimespanSmart(age).c_str());
      }
    }

    row_renderer.DrawFirstRow(canvas, rc, first_row.c_str());
    row_renderer.DrawSecondRow(canvas, rc, second_row.c_str());
  }
};

class MultiLayerPickerWidget final : public MultiSelectListWidget {
  TwoTextRowsRenderer row_renderer;
  std::shared_ptr<SkySightClient> skysight;
  std::function<void()> selection_changed_callback;

public:
  explicit MultiLayerPickerWidget(std::shared_ptr<SkySightClient> _skysight)
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

class SkySightWidget final : public ListWidget {
  std::shared_ptr<SkySightClient> skysight;
  ButtonPanelWidget *buttons_widget = nullptr;
  Button *select_button = nullptr;
  Button *set_active_button = nullptr;
  Button *preload_button = nullptr;
  Button *preload_all_button = nullptr;
  std::function<void()> cache_changed_callback;
  bool download_activity_seen = false;
  SelectedLayerRenderer row_renderer;
  UI::PeriodicTimer update_timer{[this]{ UpdateList(); }};

public:
  explicit SkySightWidget(std::shared_ptr<SkySightClient> _skysight)
    :skysight(std::move(_skysight)) {}

  void SetButtonPanel(ButtonPanelWidget &_buttons) noexcept {
    buttons_widget = &_buttons;
  }

  void SetCacheChangedCallback(std::function<void()> callback) noexcept {
    cache_changed_callback = std::move(callback);
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    CreateButtons(buttons_widget->GetButtonPanel());
    const DialogLook &look = UIGlobals::GetDialogLook();
    CreateList(parent, look, rc, row_renderer.CalculateLayout(look));
    UpdateList();
  }

  void Unprepare() noexcept override {
    DeleteWindow();
  }

  void Show(const PixelRect &rc) noexcept override {
    ListWidget::Show(rc);
    update_timer.Schedule(std::chrono::seconds(1));
  }

  void Hide() noexcept override {
    update_timer.Cancel();
    ListWidget::Hide();
  }

protected:
  bool CanActivateItem([[maybe_unused]] unsigned i) const noexcept override {
    return false;
  }

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override {
    row_renderer.Draw(canvas, rc, idx);
  }

private:
  void CreateButtons(ButtonPanel &buttons) {
    select_button = buttons.Add(_("Add to list"), [this]() {
      SelectClicked();
    });
    set_active_button = buttons.Add(_("Set active"), [this]() {
      SetActiveClicked();
    });
    preload_button = buttons.Add(_("Preload Layer"), [this]() {
      PreloadClicked();
    });
    preload_all_button = buttons.Add(_("Preload Selected"), [this]() {
      PreloadAllClicked();
    });
    buttons.EnableCursorSelection();
  }

  void UpdateButtons() {
    if (select_button == nullptr || set_active_button == nullptr ||
        preload_button == nullptr ||
        preload_all_button == nullptr)
      return;

    const auto empty = skysight == nullptr || skysight->NumSelectedLayers() == 0;
    const auto catalog_loading = skysight != nullptr && skysight->HasCredentials() &&
      !skysight->HasForecastLayers();

    select_button->SetCaption(catalog_loading ? _("Loading")
                                              : _("Add to list"));
    select_button->SetEnabled(skysight != nullptr && !catalog_loading &&
                  skysight->NumLayers() > 0);

    const auto index = empty ? 0u : GetList().GetCursorIndex();
    const auto *layer = empty ? nullptr : skysight->GetSelectedLayer(index);
    set_active_button->SetEnabled(layer != nullptr);

    bool any_forecast_layer = false;
    if (skysight != nullptr) {
      for (std::size_t i = 0; i < skysight->NumSelectedLayers(); ++i) {
        const auto *selected_layer = skysight->GetSelectedLayer(i);
        if (selected_layer == nullptr || selected_layer->SupportsLiveTiles())
          continue;

        any_forecast_layer = true;
      }
    }

    preload_button->SetEnabled(layer != nullptr && !layer->SupportsLiveTiles() &&
                               skysight->HasCredentials() &&
                               !layer->ShouldShowUpdating());
    preload_all_button->SetEnabled(skysight != nullptr && skysight->HasCredentials() &&
                                   any_forecast_layer);
  }

  void UpdateList() {
    if (skysight == nullptr)
      return;

    GetList().SetLength(std::max<std::size_t>(1, skysight->NumSelectedLayers()));
    GetList().Invalidate();
    UpdateButtons();

    const bool download_activity = skysight->HasDownloadActivity();

    if (download_activity_seen && !download_activity &&
        cache_changed_callback)
      cache_changed_callback();
    download_activity_seen = download_activity;
  }

  void SelectClicked() {
    if (skysight == nullptr)
      return;

    if (!skysight->HasCredentials()) {
      ShowMessageBox(
        _("Configure your SkySight credentials in Weather settings before loading the full SkySight catalog."),
        "SkySight", MB_OK);
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

    Button *toggle_all_button = dialog.AddButton("", [](){});
    std::function<void()> update_buttons = [picker, toggle_all_button]() {
      toggle_all_button->SetCaption(picker->GetSelectedCount() == 0
                                    ? _("Select all")
                                    : _("Select none"));
    };
    toggle_all_button->SetCallback([picker, update_buttons]() mutable {
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
                     "SkySight", MB_OK);

    UpdateList();
    if (skysight->NumSelectedLayers() > 0) {
      const auto max_index = (unsigned)(skysight->NumSelectedLayers() - 1);
      const auto current_index = GetList().GetCursorIndex();
      GetList().SetCursorIndex(std::min(current_index, max_index));
    }
    GetList().Invalidate();
    UpdateButtons();
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
    if (!success) {
      ShowMessageBox(_("Couldn't preload forecast data."),
                     "SkySight", MB_OK);
    }

    UpdateList();
  }

  void SetActiveClicked() {
    if (skysight == nullptr)
      return;

    const auto index = GetList().GetCursorIndex();
    const auto *layer = index < skysight->NumSelectedLayers()
      ? skysight->GetSelectedLayer(index)
      : nullptr;
    if (layer == nullptr)
      return;

    WeatherOverlayDraft::State overlay;
    overlay.Load(PageLayout::Overlay::SKYSIGHT, layer->id.c_str());
    overlay.draft.overlay = PageLayout::Overlay::SKYSIGHT;
    overlay.draft.skysight_overlay = layer->id;
    overlay.draft.skysight_time = PageLayout::SKYSIGHT_TIME_AUTO;
    if (overlay.IsDirty() && !overlay.ApplyIfDirty()) {
      ShowMessageBox(_("Couldn't apply the selected SkySight layer "
                       "to the current page."),
                     "SkySight", MB_OK);
      return;
    }

    (void)skysight->SetLayerActive(layer->id, true);
    UpdateList();
  }

  void PreloadAllClicked() {
    if (skysight == nullptr)
      return;

    const unsigned layers = skysight->GetSelectedForecastLayerCount();
    const unsigned files = skysight->GetPreloadFileCount();
    StaticString<256> prompt;
    if (files > 0)
      prompt.Format(_("Cache %u forecast files for %u selected SkySight layers for offline use?"),
                    files, layers);
    else
      prompt.Format(_("Discover and cache all forecasts for %u selected SkySight layers for offline use?"),
                    layers);

    if (ShowMessageBox(prompt.c_str(), _("SkySight offline cache"),
                       MB_YESNO | MB_ICONQUESTION) != IDYES)
      return;

    const bool success = skysight->PreloadAllForecasts();
    if (!success) {
      ShowMessageBox(_("Couldn't preload forecast data."),
                     "SkySight", MB_OK);
    }

    UpdateList();
  }

};

class SkySightOptionsPanel final : public RowFormWidget {
  enum Controls {
    AUTO_UPDATE,
    CACHE_SIZE,
  };

  std::shared_ptr<SkySightClient> skysight;

public:
  void UpdateCacheSize() noexcept {
    if (skysight == nullptr)
      return;

    const auto usage = skysight->GetCacheUsage();
    char size[32];
    FormatByteSize(size, sizeof(size), usage.bytes, false);

    StaticString<64> value;
    value.Format(_("%s in %u files"), size, usage.files);
    SetText(CACHE_SIZE, value.c_str());
  }

private:
  void ClearCache() noexcept {
    if (skysight == nullptr)
      return;

    const auto usage = skysight->GetCacheUsage();
    char size[32];
    FormatByteSize(size, sizeof(size), usage.bytes, false);

    StaticString<256> prompt;
    prompt.Format(
      _("Delete %s of cached SkySight data? This includes any recently "
        "downloaded forecasts and map tiles. The active layer may be "
        "downloaded again if Auto update is enabled."),
      size);
    if (ShowMessageBox(prompt.c_str(), _("Clear SkySight cache"),
                       MB_YESNO | MB_ICONWARNING) != IDYES)
      return;

    const auto deleted = skysight->ClearDownloadedData();
    UpdateCacheSize();

    char deleted_size[32];
    FormatByteSize(deleted_size, sizeof(deleted_size), deleted.bytes, false);
    StaticString<96> result;
    result.Format(_("Deleted %s in %u files."), deleted_size, deleted.files);
    ShowMessageBox(result.c_str(), _("Clear SkySight cache"), MB_OK);
  }

public:
  explicit SkySightOptionsPanel(std::shared_ptr<SkySightClient> _skysight) noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()),
     skysight(std::move(_skysight)) {}

  void Prepare([[maybe_unused]] ContainerWindow &parent,
               [[maybe_unused]] const PixelRect &rc) noexcept override {
    const auto &settings =
      CommonInterface::GetComputerSettings().weather.skysight;
    AddBoolean(
      _("Auto update"),
      _("Automatically download missing or newer SkySight data for the "
        "current map page. Manual preload remains available."),
      settings.auto_update);
    GetControl(AUTO_UPDATE).GetDataField()->SetOnModified([this] {
      auto &weather = CommonInterface::SetComputerSettings().weather;
      if (SaveValue(AUTO_UPDATE, ProfileKeys::SkySightAutoUpdate,
                    weather.skysight.auto_update)) {
        Profile::Save();
        if (skysight != nullptr)
          skysight->OnAutoUpdateChanged();
      }
    });

    AddReadOnly(_("Cache"),
                _("Total disk space used by the SkySight cache folder."));
    UpdateCacheSize();
    AddButton(_("Clear downloaded data"), [this] { ClearCache(); });
    AddButton(_("Pages setup"), [] {
      WeatherOverlayDraft::OpenPagesConfig();
    });
  }

  void Show(const PixelRect &rc) noexcept override {
    RowFormWidget::Show(rc);
    UpdateCacheSize();
  }
};

std::unique_ptr<Widget>
CreateSkySightWidget()
{
  auto skysight = DataGlobals::GetSkySight();
  if (!skysight) {
    auto widget = std::make_unique<TextWidget>();
    widget->SetText(_("SkySight is unavailable."));
    return widget;
  }

  auto list = std::make_unique<SkySightWidget>(skysight);
  auto *list_ptr = list.get();
  auto buttons = std::make_unique<ButtonPanelWidget>(
    std::move(list),
    ButtonPanelWidget::Alignment::BOTTOM);
  list_ptr->SetButtonPanel(*buttons);

  auto options = std::make_unique<SkySightOptionsPanel>(skysight);
  auto *options_ptr = options.get();
  list_ptr->SetCacheChangedCallback([options_ptr] {
    options_ptr->UpdateCacheSize();
  });
  return std::make_unique<TwoWidgets>(std::move(buttons),
                                      std::move(options));
}

#endif
