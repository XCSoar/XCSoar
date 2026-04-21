// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermDialog.hpp"

#include "Widget/RowFormWidget.hpp"
#include "Profile/Keys.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "util/StaticString.hxx"

namespace {

enum ControlIndex {
  MODEL,
  PARAMETER,
  WAVE_HEIGHT,
  VERTICAL_WIND_AGL,
};

constexpr unsigned XCTHERM_MODEL_CH = 0;
constexpr unsigned XCTHERM_MODEL_UK = 1;

constexpr unsigned XCTHERM_PARAMETER_WAVE = 0;
constexpr unsigned XCTHERM_PARAMETER_VERTICAL = 1;

static constexpr StaticEnumChoice model_list[] = {
  { XCTHERM_MODEL_CH, N_("ICON-CH") },
  { XCTHERM_MODEL_UK, N_("ICON-UK") },
  nullptr,
};

static constexpr StaticEnumChoice parameter_list[] = {
  { XCTHERM_PARAMETER_WAVE, N_("Wave") },
  { XCTHERM_PARAMETER_VERTICAL, N_("Vertical wind") },
  nullptr,
};

constexpr unsigned CH_WAVE_HEIGHTS[] = {
  1500, 2000, 3000, 4000, 5000, 6000, 7000, 8000,
};

constexpr unsigned UK_WAVE_HEIGHTS[] = {
  1000, 1500, 2000, 2500, 3000, 4200,
};

constexpr unsigned CH_VERTICAL_WIND_AGL[] = {
  100, 400,
};

constexpr unsigned UK_VERTICAL_WIND_AGL[] = {
  100, 200, 400, 800,
};

[[gnu::pure]]
static bool
IsUKModel(unsigned model) noexcept
{
  return model == XCTHERM_MODEL_UK;
}

[[gnu::pure]]
static bool
IsVerticalParameter(unsigned parameter) noexcept
{
  return parameter == XCTHERM_PARAMETER_VERTICAL;
}

[[gnu::pure]]
static const unsigned *
GetWaveHeights(unsigned model, size_t &size) noexcept
{
  if (IsUKModel(model)) {
    size = std::size(UK_WAVE_HEIGHTS);
    return UK_WAVE_HEIGHTS;
  }

  size = std::size(CH_WAVE_HEIGHTS);
  return CH_WAVE_HEIGHTS;
}

[[gnu::pure]]
static const unsigned *
GetVerticalWindOptions(unsigned model, size_t &size) noexcept
{
  if (IsUKModel(model)) {
    size = std::size(UK_VERTICAL_WIND_AGL);
    return UK_VERTICAL_WIND_AGL;
  }

  size = std::size(CH_VERTICAL_WIND_AGL);
  return CH_VERTICAL_WIND_AGL;
}

class XCThermSettingsPanel final
  : public RowFormWidget, DataFieldListener {
public:
  XCThermSettingsPanel() noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

private:
  void UpdateOptionChoices() noexcept;
  void FillIntegerEnum(unsigned row, const unsigned *values, size_t size,
                       unsigned current_value) noexcept;

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

  void OnModified(DataField &df) noexcept override;
};

void
XCThermSettingsPanel::FillIntegerEnum(unsigned row,
                                      const unsigned *values,
                                      size_t size,
                                      unsigned current_value) noexcept
{
  auto &df = (DataFieldEnum &)GetDataField(row);

  df.ClearChoices();

  unsigned selected = 0;
  for (unsigned i = 0; i < size; ++i) {
    StaticString<32> label;
    label.Format("%u", values[i]);
    df.addEnumText(label, values[i]);
    if (values[i] == current_value)
      selected = i;
  }

  df.SetValue(selected);
  GetControl(row).RefreshDisplay();
}

void
XCThermSettingsPanel::UpdateOptionChoices() noexcept
{
  const auto model = GetValueEnum(MODEL);
  const bool vertical = IsVerticalParameter(GetValueEnum(PARAMETER));

  SetRowVisible(WAVE_HEIGHT, !vertical);
  SetRowVisible(VERTICAL_WIND_AGL, vertical);

  auto &settings = CommonInterface::SetComputerSettings().weather.xctherm;

  size_t size = 0;
  if (!vertical) {
    const auto *values = GetWaveHeights(model, size);
    FillIntegerEnum(WAVE_HEIGHT, values, size, settings.wave_height);
  } else {
    const auto *values = GetVerticalWindOptions(model, size);
    FillIntegerEnum(VERTICAL_WIND_AGL, values, size,
                    settings.vertical_wind_agl);
  }
}

void
XCThermSettingsPanel::Prepare(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  const auto &settings = CommonInterface::GetComputerSettings().weather.xctherm;

  AddEnum(_("Model"),
          _("Select XCTherm model domain."),
          model_list,
          settings.model,
          this);

  AddEnum(_("Parameter"),
          _("Select weather parameter."),
          parameter_list,
          settings.parameter,
          this);

  AddEnum(_("Wave height"),
          _("Wave height above mean sea level in meters."));

  AddEnum(_("Vertical wind AGL"),
          _("Vertical wind layer above ground level in meters."));

  UpdateOptionChoices();
}

bool
XCThermSettingsPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  auto &settings = CommonInterface::SetComputerSettings().weather.xctherm;

  changed |= SaveValueEnum(MODEL, ProfileKeys::XCThermModel,
                           settings.model);
  changed |= SaveValueEnum(PARAMETER, ProfileKeys::XCThermParameter,
                           settings.parameter);

  if (!IsVerticalParameter(settings.parameter))
    changed |= SaveValueInteger(WAVE_HEIGHT, ProfileKeys::XCThermWaveHeight,
                                settings.wave_height);
  else
    changed |= SaveValueInteger(VERTICAL_WIND_AGL,
                                ProfileKeys::XCThermVerticalWindAGL,
                                settings.vertical_wind_agl);

  _changed |= changed;
  return true;
}

void
XCThermSettingsPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(MODEL, df) || IsDataField(PARAMETER, df))
    UpdateOptionChoices();
}

} // namespace

std::unique_ptr<Widget>
CreateXCThermWidget() noexcept
{
  return std::make_unique<XCThermSettingsPanel>();
}
