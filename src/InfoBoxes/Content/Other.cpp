// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/Content/Other.hpp"
#include "InfoBoxes/Data.hpp"
#include "Dialogs/Dialogs.h"
#include "Interface.hpp"
#include "UIState.hpp"
#include "InfoBoxes/Panel/Panel.hpp"
#include "InfoBoxes/Panel/CustomTextEdit.hpp"
#include "Renderer/HorizonRenderer.hpp"
#include "Hardware/PowerGlobal.hpp"
#include "system/SystemLoad.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"

#ifdef HAVE_BATTERY
#include "Hardware/PowerInfo.hpp"
#endif

void
UpdateInfoBoxHeartRate(InfoBoxData &data) noexcept
{
  const auto &basic = CommonInterface::Basic();

  if (!basic.heart_rate_available) {
    data.SetInvalid();
    return;
  }

  data.FmtValue("{}", basic.heart_rate);
}

void
UpdateInfoBoxGLoad(InfoBoxData &data) noexcept
{
  if (!CommonInterface::Basic().acceleration.available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.FmtValue("{:2.2f}", CommonInterface::Basic().acceleration.g_load);
}

void
UpdateInfoBoxBattery(InfoBoxData &data) noexcept
{
#ifdef HAVE_BATTERY
  const auto &info = Power::global_info;
  const auto &battery = info.battery;
  const auto &external = info.external;

  bool DisplaySupplyVoltageAsValue=false;
  switch (external.status) {
  case Power::ExternalInfo::Status::OFF:
    if (CommonInterface::Basic().battery_level_available)
      data.FmtComment("{}; {}%",
                      _("AC Off"),
                      (int)CommonInterface::Basic().battery_level);
    else
      data.SetComment(_("AC Off"));
    break;

  case Power::ExternalInfo::Status::ON:
    if (!CommonInterface::Basic().voltage_available)
      data.SetComment(_("AC ON"));
    else{
      DisplaySupplyVoltageAsValue = true;
      data.SetValueFromVoltage(CommonInterface::Basic().voltage);
    }
    break;

  case Power::ExternalInfo::Status::UNKNOWN:
  default:
    data.SetCommentInvalid();
  }

  if (battery.remaining_percent) {
    if (!DisplaySupplyVoltageAsValue)
      data.SetValueFromPercent(*battery.remaining_percent);
    else
      data.SetCommentFromPercent(* battery.remaining_percent);
  } else {
    if (!DisplaySupplyVoltageAsValue)
      data.SetValueInvalid();
    else
      data.SetCommentInvalid();
  }

  return;

#endif

  if (CommonInterface::Basic().voltage_available) {
    data.SetValueFromVoltage(CommonInterface::Basic().voltage);
    return;
  } else if (CommonInterface::Basic().battery_level_available) {
    data.SetValueFromPercent(CommonInterface::Basic().battery_level);
    return;
  }

  data.SetInvalid();
}

void
UpdateInfoBoxExperimental1(InfoBoxData &data) noexcept
{
  // Set Value
  data.SetInvalid();
}

void
UpdateInfoBoxExperimental2(InfoBoxData &data) noexcept
{
  // Set Value
  data.SetInvalid();
}

void
UpdateInfoBoxCPULoad(InfoBoxData &data) noexcept
{
  const auto percent_load = SystemLoadCPU();
  if (percent_load) {
    data.SetValueFromPercent(*percent_load);
  } else {
    data.SetInvalid();
  }
}

void
UpdateInfoBoxFreeRAM(InfoBoxData &data) noexcept
{
  // used to be implemented on WinCE
  data.SetInvalid();
}

void
UpdateInfoBoxPlaceholder(InfoBoxData &data) noexcept
{
  /* there is nothing to show: the window is hidden, except when a
     whole line consists of placeholders, because a line cannot
     collapse */
  data.SetInvalid();
}

void
UpdateInfoBoxInvisible(InfoBoxData &data) noexcept
{
  /* nothing is ever drawn for this InfoBox; clear all texts so that
     no leftovers of the previous content can show up */
  data.SetTitle("");
  data.SetValue("");
  data.SetComment("");
}

void
InfoBoxContentCustomText::Update(InfoBoxData &data) noexcept
{
  const auto &settings = CommonInterface::GetUISettings().info_boxes;
  const unsigned panel = CommonInterface::GetUIState().panel_index;
  const InfoBoxCustomText &text = settings.panels[panel].text[GetSlot()];

  data.SetTitle(text.title.c_str());
  data.SetValue(text.value.c_str());
  data.SetComment(text.comment.c_str());
}

static constexpr InfoBoxPanel custom_text_infobox_panels[] = {
  { NC_("Menu", "Setup"), LoadCustomTextEditPanel },
  { nullptr, nullptr }
};

const InfoBoxPanel *
InfoBoxContentCustomText::GetDialogContent() noexcept
{
  return custom_text_infobox_panels;
}

void
InfoBoxContentHorizon::OnCustomPaint(Canvas &canvas,
                                     const PixelRect &rc) noexcept
{
  const auto &attitude = CommonInterface::Basic().attitude;
  if (!attitude.bank_angle_available && !attitude.pitch_angle_available)
    return;

  const Look &look = UIGlobals::GetLook();
  HorizonRenderer::Draw(canvas, rc, look.horizon, attitude);
}

void
InfoBoxContentHorizon::Update(InfoBoxData &data) noexcept
{
  const auto &basic = CommonInterface::Basic();

  if (!basic.attitude.bank_angle_available &&
      !basic.attitude.pitch_angle_available) {
    data.SetInvalid();
    return;
  }

  data.SetCustom(basic.attitude.bank_angle_available.ToInteger() +
                 basic.attitude.pitch_angle_available.ToInteger());
}

// TODO: merge with original copy from Dialogs/StatusPanels/SystemStatusPanel.cpp
[[gnu::pure]]
static const char *
GetGPSStatus(const NMEAInfo &basic) noexcept
{
  if (!basic.alive)
    return N_("Disconnected");
  else if (!basic.location_available)
    return N_("Fix invalid");
  else if (!basic.gps_altitude_available)
    return N_("2D fix");
  else
    return N_("3D fix");
}

void
UpdateInfoBoxNbrSat(InfoBoxData &data) noexcept
{
    const NMEAInfo &basic = CommonInterface::Basic();
    const GPSState &gps = basic.gps;

    data.SetComment(gettext(GetGPSStatus(basic)));

    if (!basic.alive)
        data.SetComment(_("No GPS"));
    else if (gps.satellites_used_available) {
        // known number of sats
        data.FmtValue("{}", gps.satellites_used);
    } else {
        // valid but unknown number of sats
        data.SetValueInvalid();
    }
}

void
InfoBoxContentNbrSat::Update(InfoBoxData &data) noexcept
{
  UpdateInfoBoxNbrSat(data);
}

bool
InfoBoxContentNbrSat::HandleClick() noexcept
{
  dlgStatusShowModal(1);
  return true;
}
