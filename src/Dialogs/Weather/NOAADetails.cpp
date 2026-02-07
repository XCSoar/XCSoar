// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOAADetails.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_NOAA

#include "Dialogs/CoDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/LargeTextWidget.hpp"
#include "Weather/NOAAGlue.hpp"
#include "Weather/NOAAStore.hpp"
#include "Weather/NOAAUpdater.hpp"
#include "Weather/ParsedMETAR.hpp"
#include "Weather/NOAAFormatter.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "co/InvokeTask.hxx"
#include "co/Task.hxx"
#include "net/http/Init.hpp"
#include "UIGlobals.hpp"

class NOAADetailsWidget final : public LargeTextWidget {
  WndForm &dialog;
  NOAAStore::iterator station_iterator;

public:
  NOAADetailsWidget(WndForm &_dialog, NOAAStore::iterator _station)
    :LargeTextWidget(_dialog.GetLook()), dialog(_dialog),
    station_iterator(_station) {}

  void CreateButtons(WidgetDialog &buttons);

private:
  void Update();
  void UpdateClicked();
  void RemoveClicked();

  /* virtual methods from class Widget */
  void Show(const PixelRect &rc) noexcept override;
};

void
NOAADetailsWidget::CreateButtons(WidgetDialog &buttons)
{
  buttons.AddButton(_("Update"), [this](){ UpdateClicked(); });
  buttons.AddButton(_("Remove"), [this](){ RemoveClicked(); });

  buttons.EnableCursorSelection();
}

void
NOAADetailsWidget::Update()
{
  std::string metar_taf = "";

  NOAAFormatter::Format(*station_iterator, metar_taf);

  SetText(metar_taf.c_str());

  StaticString<100> caption;
  caption.Format("%s: ", _("METAR and TAF"));

  if (!station_iterator->parsed_metar_available ||
      !station_iterator->parsed_metar.name_available)
    caption += station_iterator->GetCodeT();
  else
    caption.AppendFormat("%s (%s)",
                         station_iterator->parsed_metar.name.c_str(),
                         station_iterator->GetCodeT());

  dialog.SetCaption(caption);
}

static Co::InvokeTask
UpdateTask(NOAAStore::Item &item, ProgressListener &progress) noexcept
{
  co_await NOAAUpdater::Update(item, *Net::curl, progress);
}

inline void
NOAADetailsWidget::UpdateClicked()
{
  PluggableOperationEnvironment env;
  if (ShowCoDialog(dialog.GetMainWindow(), dialog.GetLook(),
                   _("Download"), UpdateTask(*station_iterator, env),
                   &env))
    Update();
}

inline void
NOAADetailsWidget::RemoveClicked()
{
  StaticString<256> tmp;
  tmp.Format(_("Do you want to remove station %s?"),
             station_iterator->GetCodeT());

  if (ShowMessageBox(tmp, _("Remove"), MB_YESNO) == IDNO)
    return;

  noaa_store->erase(station_iterator);
  noaa_store->SaveToProfile();

  dialog.SetModalResult(mrOK);
}

void
NOAADetailsWidget::Show(const PixelRect &rc) noexcept
{
  LargeTextWidget::Show(rc);
  Update();
}

void
dlgNOAADetailsShowModal(NOAAStore::iterator iterator)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<NOAADetailsWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           look, _("METAR and TAF"));
  dialog.SetWidget(dialog, iterator);
  dialog.GetWidget().CreateButtons(dialog);
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

#else

#include "Dialogs/Message.hpp"

void
dlgNOAADetailsShowModal(unsigned station_index)
{
  ShowMessageBox(_("This function is not available on your platform yet."),
              _("Error"), MB_OK);
}
#endif
