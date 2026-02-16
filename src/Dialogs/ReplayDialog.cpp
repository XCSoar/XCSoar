// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ReplayDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Components.hpp"
#include "Replay/Replay.hpp"
#include "Form/DataField/Base.hpp"
#include "Language/Language.hpp"

class ReplayControlWidget final
  : public RowFormWidget
{
  enum Controls {
    FILE,
    RATE,
  };

  Replay &replay;

public:
  ReplayControlWidget(Replay &_replay, const DialogLook &look) noexcept
    :RowFormWidget(look), replay(_replay) {}

  void CreateButtons(WidgetDialog &dialog) noexcept {
    dialog.AddButton(_("Start"), [this](){ OnStartClicked(); });
    dialog.AddButton(_("Stop"), [this](){ OnStopClicked(); });
    dialog.AddButton("+10'", [this](){ OnFastForwardClicked(); });
  }

private:
  void OnStopClicked() noexcept;
  void OnStartClicked() noexcept;
  void OnFastForwardClicked() noexcept;

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
};

void
ReplayControlWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                             [[maybe_unused]] const PixelRect &rc) noexcept
{
  AddFile(_("File"),
          _("Name of file to replay. May be an IGC file (.igc) or a raw NMEA log file (.nmea). Leave blank to run the demo."),
          {},
          "*.nmea\0*.igc\0",
          true);
  LoadValue(FILE, replay.GetFilename());

  AddFloat(_("Rate"),
           _("Time acceleration of replay. Set to 0 for pause, 1 for normal real-time replay."),
           "%.0f x", "%.0f",
           0, 10, 1, false, replay.GetTimeScale());
  GetDataField(RATE).SetOnModified([this]{
    replay.SetTimeScale(GetValueFloat(RATE));
  });
}

inline void
ReplayControlWidget::OnStopClicked() noexcept
{
  replay.Stop();
}

inline void
ReplayControlWidget::OnStartClicked() noexcept
{
  const Path path = GetValueFile(FILE);

  try {
    replay.Start(path);
  } catch (...) {
    ShowError(std::current_exception(), _("Replay"));
  }
}

inline void
ReplayControlWidget::OnFastForwardClicked() noexcept
{
  replay.FastForward(std::chrono::minutes{10});
}

void
ShowReplayDialog(Replay &replay) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  ReplayControlWidget *widget = new ReplayControlWidget(replay, look);
  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      look, _("Replay"), widget);
  widget->CreateButtons(dialog);
  dialog.AddButton(_("Close"), mrOK);

  dialog.ShowModal();
}
