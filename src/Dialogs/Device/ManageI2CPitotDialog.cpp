/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "ManageI2CPitotDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "Language/Language.hpp"
#include "NMEA/Info.hpp"
#include "Device/Descriptor.hpp"
#include "Profile/Current.hpp"
#include "Profile/Profile.hpp"
#include "Profile/DeviceConfig.hpp"
#include "Interface.hpp"

using namespace UI;

class ManageI2CPitotWidget final
  : public RowFormWidget, NullBlackboardListener {

  enum Controls {
    PITOT,
    STATIC,
    OFFSET,
    CALIBRATE,
  };

  DeviceDescriptor &device;

  Button *calibrate_button;

public:
  ManageI2CPitotWidget(const DialogLook &look,
                       DeviceDescriptor &_device) noexcept
    :RowFormWidget(look), device(_device) {}

  void CreateButtons(WidgetDialog &dialog) noexcept {
    calibrate_button = dialog.AddButton(_("Calibrate"), [this](){ Calibrate(); });
    calibrate_button->SetEnabled(false);
  }

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  void Show(const PixelRect &rc) noexcept override {
    RowFormWidget::Show(rc);
    UpdateValues(CommonInterface::Basic());
    CommonInterface::GetLiveBlackboard().AddListener(*this);
  }

  void Hide() noexcept override {
    CommonInterface::GetLiveBlackboard().RemoveListener(*this);
    RowFormWidget::Hide();
  }

private:
  void UpdateValues(const NMEAInfo &basic) noexcept;
  void Calibrate() noexcept;

  /* virtual methods from BlackboardListener */
  void OnGPSUpdate(const MoreData &basic) override {
    UpdateValues(basic);
  }
};

void
ManageI2CPitotWidget::UpdateValues(const NMEAInfo &all_data) noexcept
{
  const auto &device_data = device.GetData();

  if (device_data.pitot_pressure_available)
    LoadValue(PITOT, device_data.pitot_pressure.GetHectoPascal(),
              UnitGroup::PRESSURE);
  else
    LoadValue(PITOT, 0.f);

  if (all_data.static_pressure_available)
    LoadValue(STATIC, all_data.static_pressure.GetHectoPascal(),
              UnitGroup::PRESSURE);
  else
    LoadValue(STATIC, 0.f);

  calibrate_button->SetEnabled(device_data.pitot_pressure_available &&
                               all_data.static_pressure_available);
}

void
ManageI2CPitotWidget::Calibrate() noexcept
{
  const unsigned index = device.GetIndex();
  DeviceConfig &config = CommonInterface::SetSystemSettings().devices[index];
  config.sensor_offset = GetValueFloat(PITOT)
    + device.GetConfig().sensor_offset - GetValueFloat(STATIC);

  Profile::SetDeviceConfig(Profile::map, index, config);
  Profile::Save();

  device.SetConfig(config);

  LoadValue(OFFSET, config.sensor_offset,
            UnitGroup::PRESSURE);
}

void
ManageI2CPitotWidget::Prepare(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  AddReadOnly(_("Pitot"), nullptr, _T("%.2f"), UnitGroup::PRESSURE, 0);
  AddReadOnly(_("Pressure"), nullptr, _T("%.2f"), UnitGroup::PRESSURE, 0);
  AddReadOnly(_("Offset"), nullptr, _T("%.2f"),
              UnitGroup::PRESSURE, device.GetConfig().sensor_offset);
}

void
ManageI2CPitotDialog(UI::SingleWindow &parent, const DialogLook &look,
                     DeviceDescriptor &device) noexcept
{
  assert(device.GetConfig().port_type == DeviceConfig::PortType::DROIDSOAR_V2 ||
         (device.GetConfig().port_type == DeviceConfig::PortType::I2CPRESSURESENSOR &&
          device.GetConfig().press_use == DeviceConfig::PressureUse::PITOT));

  TWidgetDialog<ManageI2CPitotWidget>
   dialog(WidgetDialog::Auto{}, parent, look,
                      _("Pitot sensor calibration"));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.SetWidget(look, device);
  dialog.GetWidget().CreateButtons(dialog);
  dialog.ShowModal();
}
