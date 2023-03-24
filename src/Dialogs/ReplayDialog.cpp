// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ReplayDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Listener.hpp"
#include "UIGlobals.hpp"
#include "Components.hpp"
#include "Replay/Replay.hpp"
#include "Form/DataField/File.hpp"
#include "Form/DataField/Float.hpp"
#include "Language/Language.hpp"

class ReplayControlWidget final
  : public RowFormWidget, DataFieldListener {
  enum Controls {
    FILE,
    RATE,
  };

public:
  ReplayControlWidget(const DialogLook &look)
    :RowFormWidget(look) {}

  void CreateButtons(WidgetDialog &dialog) {
    dialog.AddButton(_("Start"), [this](){ OnStartClicked(); });
    dialog.AddButton(_("Stop"), [this](){ OnStopClicked(); });
    dialog.AddButton(_T("+10'"), [this](){ OnFastForwardClicked(); });
  }

private:
  void OnStopClicked();
  void OnStartClicked();
  void OnFastForwardClicked();

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;

private:
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
ReplayControlWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                             [[maybe_unused]] const PixelRect &rc) noexcept
{
  auto *file =
    AddFile(_("File"),
            _("Name of file to replay.  Can be an IGC file (.igc), a raw NMEA log file (.nmea), or if blank, runs the demo."),
            nullptr,
            _T("*.nmea\0*.igc\0"),
            true);
  ((FileDataField *)file->GetDataField())->SetValue(Path(replay->GetFilename()));
  file->RefreshDisplay();

  AddFloat(_("Rate"),
           _("Time acceleration of replay. Set to 0 for pause, 1 for normal real-time replay."),
           _T("%.0f x"), _T("%.0f"),
           0, 10, 1, false, replay->GetTimeScale(), this);
}

inline void
ReplayControlWidget::OnStopClicked()
{
  replay->Stop();
}

inline void
ReplayControlWidget::OnStartClicked()
{
  const auto &df = (const FileDataField &)GetDataField(FILE);
  const Path path = df.GetValue();

  try {
    replay->Start(path);
  } catch (...) {
    ShowError(std::current_exception(), _("Replay"));
  }
}

inline void
ReplayControlWidget::OnFastForwardClicked()
{
  replay->FastForward(std::chrono::minutes{10});
}

void
ReplayControlWidget::OnModified(DataField &_df) noexcept
{
  const DataFieldFloat &df = (const DataFieldFloat &)_df;

  replay->SetTimeScale(df.GetValue());
}

void
ShowReplayDialog()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  ReplayControlWidget *widget = new ReplayControlWidget(look);
  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      look, _("Replay"), widget);
  widget->CreateButtons(dialog);
  dialog.AddButton(_("Close"), mrOK);

  dialog.ShowModal();
}
