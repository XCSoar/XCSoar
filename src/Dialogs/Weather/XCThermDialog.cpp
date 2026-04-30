// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_HTTP

#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/LargeTextWidget.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "util/StaticString.hxx"
#include "Weather/xctherm/XCThermAuth.hpp"
#include "Weather/xctherm/XCThermGeoJSON.hpp"
#include "Weather/xctherm/XCThermGeoJSONOverlay.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "LogFile.hpp"

#include <fstream>
#include <sstream>

namespace {

constexpr unsigned XCTHERM_MODEL_CH = 0;
constexpr unsigned XCTHERM_MODEL_UK = 1;

struct LayerInfo {
  const char *label;       // display text
  const char *file_suffix; // e.g. "5000amsl" or "100agl"
  unsigned value;          // altitude value in meters
  bool is_agl;             // true = AGL, false = AMSL
};

static constexpr LayerInfo CH_LAYERS[] = {
  { "Vertical wind 1500 m AMSL", "1500amsl", 1500, false },
  { "Vertical wind 2000 m AMSL", "2000amsl", 2000, false },
  { "Vertical wind 3000 m AMSL", "3000amsl", 3000, false },
  { "Vertical wind 4000 m AMSL", "4000amsl", 4000, false },
  { "Vertical wind 5000 m AMSL", "5000amsl", 5000, false },
  { "Vertical wind 6000 m AMSL", "6000amsl", 6000, false },
  { "Vertical wind 7000 m AMSL", "7000amsl", 7000, false },
  { "Vertical wind 8000 m AMSL", "8000amsl", 8000, false },
  { "Vertical wind 100 m AGL",   "100agl",   100,  true },
  { "Vertical wind 400 m AGL",   "400agl",   400,  true },
};

static constexpr LayerInfo UK_LAYERS[] = {
  { "Vertical wind 1000 m AMSL", "1000amsl", 1000, false },
  { "Vertical wind 1500 m AMSL", "1500amsl", 1500, false },
  { "Vertical wind 2000 m AMSL", "2000amsl", 2000, false },
  { "Vertical wind 2500 m AMSL", "2500amsl", 2500, false },
  { "Vertical wind 3000 m AMSL", "3000amsl", 3000, false },
  { "Vertical wind 4200 m AMSL", "4200amsl", 4200, false },
  { "Vertical wind 100 m AGL",   "100agl",   100,  true },
  { "Vertical wind 200 m AGL",   "200agl",   200,  true },
  { "Vertical wind 400 m AGL",   "400agl",   400,  true },
  { "Vertical wind 800 m AGL",   "800agl",   800,  true },
};

[[gnu::pure]]
static bool
IsUKModel(unsigned model) noexcept
{
  return model == XCTHERM_MODEL_UK;
}

static const LayerInfo *
GetLayers(unsigned model, size_t &size) noexcept
{
  if (IsUKModel(model)) {
    size = std::size(UK_LAYERS);
    return UK_LAYERS;
  }
  size = std::size(CH_LAYERS);
  return CH_LAYERS;
}

[[gnu::pure]]
static bool
IsActiveLayer(const LayerInfo &layer,
              unsigned active_parameter,
              unsigned active_wave_height,
              unsigned active_vertical_agl) noexcept
{
  if (layer.is_agl)
    return active_parameter == 1 && layer.value == active_vertical_agl;
  else
    return active_parameter == 0 && layer.value == active_wave_height;
}

/* ---- List item renderer ---- */

class XCThermRowRenderer {
  TwoTextRowsRenderer row_renderer;

public:
  unsigned CalculateLayout(const DialogLook &look) {
    return row_renderer.CalculateLayout(*look.list.font_bold,
                                       look.small_font);
  }

  void Draw(Canvas &canvas, const PixelRect rc, unsigned index,
            unsigned model, unsigned active_parameter,
            unsigned active_wave_height,
            unsigned active_vertical_agl);
};

void
XCThermRowRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          unsigned index, unsigned model,
                          unsigned active_parameter,
                          unsigned active_wave_height,
                          unsigned active_vertical_agl)
{
  size_t count = 0;
  const auto *layers = GetLayers(model, count);
  if (index >= count)
    return;

  const auto &layer = layers[index];
  const bool active = IsActiveLayer(layer, active_parameter,
                                     active_wave_height,
                                     active_vertical_agl);

  StaticString<80> first_row;
  if (active)
    first_row.Format("%s  [ACTIVE]", layer.label);
  else
    first_row = layer.label;

  const char *second_row = active ? "Currently selected" : "";

  row_renderer.DrawFirstRow(canvas, rc, first_row);
  row_renderer.DrawSecondRow(canvas, rc, second_row);
}

/* ---- String choice renderer for model picker ---- */

class StringChoiceRenderer final : public ListItemRenderer {
  TextRowRenderer row_renderer;
  const char *const *choices;

public:
  explicit StringChoiceRenderer(const char *const *_choices) noexcept
    : choices(_choices) {}

  unsigned CalculateLayout(const DialogLook &look) {
    return row_renderer.CalculateLayout(*look.list.font);
  }

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned i) noexcept override {
    row_renderer.DrawTextRow(canvas, rc, choices[i]);
  }
};

/* ---- Main widget ---- */

class XCThermWidget final : public ListWidget {
  ButtonPanelWidget *buttons_widget = nullptr;
  Button *activate_button = nullptr;
  Button *update_button = nullptr;
  Button *model_button = nullptr;

  XCThermRowRenderer row_renderer;

public:
  void SetButtonPanel(ButtonPanelWidget &_buttons) {
    buttons_widget = &_buttons;
  }

  void CreateButtons(ButtonPanel &buttons);

private:
  void UpdateList();
  void SaveSettings();

  void ActivateClicked();
  void UpdateClicked();
  void ModelClicked();

public:
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;

protected:
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  void OnCursorMoved(unsigned index) noexcept override;

  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override {
    ActivateClicked();
  }
};

void
XCThermWidget::CreateButtons(ButtonPanel &buttons)
{
  activate_button = buttons.Add("Activate", [this]() { ActivateClicked(); });
  update_button = buttons.Add("Update", [this]() { UpdateClicked(); });
  model_button = buttons.Add("Model", [this]() { ModelClicked(); });
}

void
XCThermWidget::SaveSettings()
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  Profile::Set(ProfileKeys::XCThermModel, (int)settings.model);
  Profile::Set(ProfileKeys::XCThermParameter, (int)settings.parameter);
  Profile::Set(ProfileKeys::XCThermWaveHeight, (int)settings.wave_height);
  Profile::Set(ProfileKeys::XCThermVerticalWindAGL,
               (int)settings.vertical_wind_agl);
}

void
XCThermWidget::UpdateList()
{
  ListControl &list = GetList();
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;

  size_t count = 0;
  const auto *layers = GetLayers(settings.model, count);

  list.SetLength(count);

  /* Find the currently active row */
  int active_index = -1;
  for (unsigned i = 0; i < count; ++i) {
    if (IsActiveLayer(layers[i], settings.parameter,
                       settings.wave_height,
                       settings.vertical_wind_agl)) {
      active_index = (int)i;
      break;
    }
  }

  if (active_index >= 0)
    list.SetCursorIndex(active_index);

  list.Invalidate();

  /* Update model button label */
  model_button->SetCaption(IsUKModel(settings.model)
                           ? "Model: UK" : "Model: CH");

  /* Update activate/update button state based on cursor */
  OnCursorMoved(list.GetCursorIndex());
}

void
XCThermWidget::OnCursorMoved(unsigned index) noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;

  size_t count = 0;
  const auto *layers = GetLayers(settings.model, count);

  const bool cursor_is_active = index < count &&
    IsActiveLayer(layers[index], settings.parameter,
                   settings.wave_height, settings.vertical_wind_agl);

  if (cursor_is_active) {
    activate_button->SetCaption("Active");
    activate_button->SetEnabled(false);
  } else {
    activate_button->SetCaption("Activate");
    activate_button->SetEnabled(true);
  }
}

void
XCThermWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                            unsigned idx) noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  row_renderer.Draw(canvas, rc, idx,
                    settings.model, settings.parameter,
                    settings.wave_height, settings.vertical_wind_agl);
}

void
XCThermWidget::ActivateClicked()
{
  auto &settings = CommonInterface::SetComputerSettings().weather.xctherm;

  const int index = GetList().GetCursorIndex();
  size_t count = 0;
  const auto *layers = GetLayers(settings.model, count);

  if (index < 0 || (unsigned)index >= count)
    return;

  const auto &layer = layers[index];

  if (layer.is_agl) {
    settings.parameter = 1;
    settings.vertical_wind_agl = layer.value;
  } else {
    settings.parameter = 0;
    settings.wave_height = layer.value;
  }

  SaveSettings();
  UpdateList();
}

void
XCThermWidget::UpdateClicked()
{
  auto *map = UIGlobals::GetMap();
  if (map == nullptr) {
    ShowMessageBox("Map not available.", "XCTherm", MB_OK);
    return;
  }

  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;

  /* Find the active layer */
  size_t count = 0;
  const auto *layers = GetLayers(settings.model, count);

  const LayerInfo *active = nullptr;
  for (unsigned i = 0; i < count; ++i) {
    if (IsActiveLayer(layers[i], settings.parameter,
                      settings.wave_height,
                      settings.vertical_wind_agl)) {
      active = &layers[i];
      break;
    }
  }

  if (active == nullptr) {
    ShowMessageBox("No layer activated.\nSelect a layer and press Activate first.",
                   "XCTherm", MB_OK);
    return;
  }

  /* Build path to local example GeoJSON file.
   * TODO: replace with HTTP download from XCTherm API */
  StaticString<256> filename;
  filename.Format("vertical_wind_%s.geojson", active->file_suffix);

  /* Try unzipped first, then zipped */
  const std::string base_path =
    "/Users/pheinrich/Documents/Fliegen/xctherm/xcsoar_wave/geojson_example/";
  std::string file_path = base_path + filename.c_str();

  /* Check if the unzipped file is in a subdirectory (macOS unzip artifact) */
  std::ifstream file(file_path);
  if (!file.is_open()) {
    /* Try subdirectory */
    file_path = base_path + filename.c_str() + "/" + filename.c_str();
    file.open(file_path);
  }

  if (!file.is_open()) {
    StaticString<512> msg;
    msg.Format("File not found:\n%s\n\nPlease unzip the .geojson.zip first.",
               filename.c_str());
    ShowMessageBox(msg, "XCTherm", MB_OK);
    return;
  }

  LogFmt("xctherm: loading {}", file_path);

  /* Read entire file */
  std::ostringstream ss;
  ss << file.rdbuf();
  file.close();
  std::string content = ss.str();

  LogFmt("xctherm: read {} bytes", content.size());

  /* Parse GeoJSON */
  auto forecast = XCThermGeoJSON::Parse(content, true);

  if (forecast.IsEmpty()) {
    ShowMessageBox("GeoJSON parsed but no data found.", "XCTherm", MB_OK);
    return;
  }

  forecast.layer_name = active->label;

  /* Capture stats before move */
  const unsigned n_polys = forecast.TotalPolygons();
  const unsigned n_bands = forecast.bands.size();

  /* Create overlay and set it on the map */
  auto overlay = std::make_unique<XCThermGeoJSONOverlay>();
  overlay->SetForecast(std::move(forecast), active->label);
  map->SetOverlay(std::move(overlay));

  StaticString<256> msg;
  msg.Format("Loaded: %s\n%u polygons in %u wind bands.",
             active->label, n_polys, n_bands);
  ShowMessageBox(msg, "XCTherm", MB_OK);

  UpdateList();
}

void
XCThermWidget::ModelClicked()
{
  static constexpr const char *choices[] = { "CH (Alps)", "UK" };

  auto &settings = CommonInterface::SetComputerSettings().weather.xctherm;
  StringChoiceRenderer item_renderer(choices);

  int index = IsUKModel(settings.model) ? 1 : 0;
  index = ListPicker("Select model",
                     std::size(choices), index,
                     item_renderer.CalculateLayout(UIGlobals::GetDialogLook()),
                     item_renderer,
                     false, nullptr, nullptr, nullptr);

  if (index < 0)
    return;

  settings.model = index == 1 ? XCTHERM_MODEL_UK : XCTHERM_MODEL_CH;
  SaveSettings();
  UpdateList();
}

void
XCThermWidget::Prepare(ContainerWindow &parent,
                        const PixelRect &rc) noexcept
{
  CreateButtons(buttons_widget->GetButtonPanel());
  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc, row_renderer.CalculateLayout(look));
  UpdateList();
}

} // namespace

std::unique_ptr<Widget>
CreateXCThermWidget() noexcept
{
  /*
   * If no XCTherm account is configured, show a simple message
   * (same pattern as pc_met).
   */
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;

  if (!settings.credentials.IsDefined())
    return std::make_unique<LargeTextWidget>(UIGlobals::GetDialogLook(),
                                             "No XCTherm account configured.\n\n"
                                             "Enter your credentials in\n"
                                             "Config > System > Weather.");

  auto widget = std::make_unique<XCThermWidget>();
  auto buttons = std::make_unique<ButtonPanelWidget>(
    std::move(widget),
    ButtonPanelWidget::Alignment::BOTTOM);
  auto *widget_ptr = (XCThermWidget *)&buttons->GetWidget();
  widget_ptr->SetButtonPanel(*buttons);
  return buttons;
}

#endif
