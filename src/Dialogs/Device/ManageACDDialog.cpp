// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ManageACDDialog.hpp"
#include "Device/Driver/AirControlDisplay/Internal.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "UIGlobals.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Widget/VScrollWidget.hpp"

#include <array>
#include <string>

class ManageACDWidget final : public RowFormWidget {
  struct Setting {
    const char *item;
    const char *label;
    bool boolean;
  };

  static constexpr std::array settings{
    Setting{"RXVOL1", N_("Primary volume"), false},
    Setting{"RXVOL2", N_("Secondary volume"), false},
    Setting{"DWATCH", N_("Dual watch"), true},
    Setting{"SQL", N_("Squelch"), false},
    Setting{"SQLOFF", N_("Squelch off"), true},
    Setting{"ICVOL1", N_("Intercom 1 volume"), false},
    Setting{"ICVOX1", N_("Intercom 1 VOX"), false},
    Setting{"ICVOL2", N_("Intercom 2 volume"), false},
    Setting{"ICVOX2", N_("Intercom 2 VOX"), false},
    Setting{"AUXVOL", N_("Auxiliary volume"), false},
  };

  WidgetDialog &dialog;
  ACDDevice &device;
  const char *com_code = "COMM";
  std::array<unsigned, settings.size()> values{};
  std::array<unsigned, settings.size()> row_indices{};
  std::array<bool, settings.size()> available{};

public:
  ManageACDWidget(const DialogLook &look, WidgetDialog &_dialog,
                  ACDDevice &_device)
    :RowFormWidget(look), dialog(_dialog), device(_device) {}

  void Prepare(ContainerWindow &, const PixelRect &) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
ManageACDWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                         [[maybe_unused]] const PixelRect &rc) noexcept
{
  bool has_settings = false;
  try {
    MessageOperationEnvironment env;
    unsigned next_row = 0;
    std::string version;
    device.ReadConfiguration("ACD", "SWVER", version, env);
    AddReadOnly(_("Firmware version"), nullptr,
                version.empty() ? _("Unknown") : version.c_str());
    ++next_row;

    unsigned number = 0;
    bool com_available = device.ReadConfigurationUnsigned(
      "COMM", "RXVOL1", number, 100, env);
    if (!com_available) {
      com_code = "COM";
      com_available = device.ReadConfigurationUnsigned(
        "COM", "RXVOL1", number, 100, env);
    }
    const unsigned primary_volume = number;
    AddReadOnly(_("Radio"), nullptr,
                com_available ? _("Connected") : _("Unavailable"));
    ++next_row;
    AddReadOnly(_("Transponder"), nullptr,
                device.ReadConfigurationUnsigned("XPDR", "SQUAWK", number,
                                                 7777, env)
                ? _("Connected") : _("Unavailable"));
    ++next_row;
    std::string altitude;
    AddReadOnly(_("Altitude"), nullptr,
                device.ReadConfiguration("ALT", "QNH", altitude, env)
                ? _("Active") : _("Unavailable"));
    ++next_row;

    if (com_available) {
      for (unsigned i = 0; i < settings.size(); ++i) {
        const auto &setting = settings[i];
        unsigned value;
        if (i == 0)
          value = primary_volume;
        else if (!device.ReadConfigurationUnsigned(
                   com_code, setting.item, value,
                   setting.boolean ? 1 : 100, env))
          continue;

        values[i] = value;
        available[i] = true;
        has_settings = true;
        row_indices[i] = next_row++;
        if (setting.boolean)
          AddBoolean(gettext(setting.label), nullptr, value != 0);
        else
          AddInteger(gettext(setting.label), nullptr, "%d%%", "%d",
                     0, 100, 1, value);
      }
    }
  } catch (OperationCancelled) {
  } catch (...) {
    ShowError(std::current_exception(), "ACD");
  }

  if (has_settings)
    dialog.AddButton(_("Save"), [this](){
      bool changed = false;
      dialog.GetWidget().Save(changed);
    });
}

bool
ManageACDWidget::Save(bool &changed) noexcept
{
  try {
    MessageOperationEnvironment env;
    bool write_failed = false;
    for (unsigned i = 0; i < settings.size(); ++i) {
      if (!available[i])
        continue;

      unsigned value = settings[i].boolean
        ? GetValueBoolean(row_indices[i]) : GetValueInteger(row_indices[i]);
      if (value == values[i])
        continue;

      if (!device.WriteConfiguration(com_code, settings[i].item, value, env)) {
        write_failed = true;
        break;
      }

      values[i] = value;
      changed = true;
    }

    /* Some radios change a related setting implicitly, e.g. dual watch
       can change secondary volume.  Re-read every visible control. */
    if (changed || write_failed) {
      for (unsigned i = 0; i < settings.size(); ++i) {
        if (!available[i])
          continue;

        const unsigned edited = settings[i].boolean
          ? GetValueBoolean(row_indices[i]) : GetValueInteger(row_indices[i]);
        const bool pending = edited != values[i];
        unsigned value;
        if (!device.ReadConfigurationUnsigned(
              com_code, settings[i].item, value,
              settings[i].boolean ? 1 : 100, env))
          continue;

        values[i] = value;
        if (pending)
          continue;

        if (settings[i].boolean)
          LoadValue(row_indices[i], values[i] != 0);
        else
          LoadValue(row_indices[i], values[i]);
      }
    }

    if (write_failed) {
      ShowMessageBox(_("Device did not accept the setting"),
                     "ACD", MB_OK | MB_ICONERROR);
      return false;
    }
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    ShowError(std::current_exception(), "ACD");
    return false;
  }

  return true;
}

void
ManageACDDialog(Device &_device)
{
  auto &device = static_cast<ACDDevice &>(_device);
  const auto &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(), look,
                      "ACD",
                      new VScrollWidget(
                        std::make_unique<ManageACDWidget>(look, dialog,
                                                          device),
                        look));
  dialog.PrepareWidget();
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
