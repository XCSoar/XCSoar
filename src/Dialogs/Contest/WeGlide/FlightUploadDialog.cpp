/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#include "FlightUploadDialog.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "contest/weglide/UploadIGCFile.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/DataField/File.hpp"
#include "Form/DataField/Date.hpp"
#include "Language/Language.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "system/FileUtil.hpp"
#include "ui/event/KeyCode.hpp"
#include "Widget/RowFormWidget.hpp"

class UploadDialog final : public WidgetDialog {

public:
  UploadDialog(Auto style, UI::SingleWindow &parent, const DialogLook &look,
               const TCHAR *caption, Widget *widget) noexcept
      : WidgetDialog(style, parent, look, caption, widget) {}

  // take focus setting from here, not the default!
  void SetDefaultFocus() override {}

private:
  bool OnAnyKeyDown(unsigned key_code) override;
};


class UploadWidget final : public RowFormWidget, DataFieldListener {

public:
  UploadWidget(const DialogLook &look, const Path &igc_path,
               const WeGlide::User &user_, const uint_least32_t glider_id)
      : RowFormWidget(look), igcpath(igc_path), user(user_),
        aircraft_id(glider_id) {}

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;

  /* Getter for Widget data */
  auto GetIGCPath() { return igcpath; }
  auto GetUser() { return user; }
  auto GetAircraft() { return aircraft_id; }

  bool ShowUploadDialog();
private:
  // DataFieldIndex is the order of AddField in Prepare()!
  enum DataFieldIndex {
    SPACER1,
    IGC_FILE,
    SPACER2,
    USER_ID,
    USER_BIRTH,
    AIRCRAFT_ID,
    SPACER3
  };

  Path igcpath;
  WeGlide::User user;
  uint_least32_t aircraft_id;

  Button *ok_button;
  Button *cancel_button;
  WndProperty *data_fields[5];

  /* virtual methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

bool 
UploadWidget::SetFocus() noexcept
{
  bool data_ok;
  RowFormWidget::SetFocus();

  // Parse data and find 1st failure:
  if (!File::Exists(igcpath)) {
    data_ok = false;
    data_fields[0]->SetFocus();
  } else if (user.id == 0) {
    data_ok = false;
    data_fields[1]->SetFocus();
  } else if (!user.birthdate.IsPlausible()) {
    data_ok = false;
    data_fields[2]->SetFocus();
  } else if (aircraft_id == 0) {
    data_ok = false;
    data_fields[3]->SetFocus();
  } else {
    data_ok = true;
    ok_button->SetFocus();  // darkblue
  }

  ok_button->SetEnabled(data_ok);  // normal or gray
  ok_button->SetSelected(data_ok);   // lightblue
  cancel_button->SetSelected(!data_ok);

  return data_ok;
}

void 
UploadWidget::OnModified(DataField &df) noexcept
{
  if (IsDataField(IGC_FILE, df)) {
    igcpath = static_cast<FileDataField &>(df).GetValue();
  } else if (IsDataField(USER_ID, df)) {
    user.id = df.GetAsInteger();
  } else if (IsDataField(USER_BIRTH, df)) {
    user.birthdate = static_cast<DataFieldDate &>(df).GetValue();
  } else if (IsDataField(AIRCRAFT_ID, df)) {
    aircraft_id = df.GetAsInteger();
  }
  SetFocus(); // with check state of fields!
}

void
UploadWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  const auto settings = CommonInterface::GetComputerSettings(); 
  if (user.id == 0)
    user = settings.weglide.pilot;
  if (aircraft_id == 0)
    aircraft_id = settings.plane.weglide_glider_type;
  
  AddSpacer();

  data_fields[0] =  AddFile(_("IGC File"), nullptr, nullptr,
    _("*.igc"), FileType::IGC);
  if (data_fields[0]) {
    auto file_df = static_cast<FileDataField *>(data_fields[0]->GetDataField());
    file_df->SetListener(this);
    if (!File::Exists(igcpath)) {
      file_df->SetIndex(1);
      igcpath = file_df->GetValue();
    } else {
      file_df->SetValue(igcpath);
    }
    data_fields[0]->RefreshDisplay();
  }

  AddSpacer();
  data_fields[1] =AddInteger(_("Pilot"),
             _("Take this from your WeGlide Profile. Or set to 0 if not used."),
             _T("%u"), _T("%u"), 0, 99999, 1, user.id, this);

  data_fields[2] = AddDate(_("User date of birth"), nullptr, user.birthdate, this);

  data_fields[3] =  AddInteger(_("Aircraft"),
      _("Take this from your WeGlide Profile. Or set to 0 if not used."),
      _T("%u"), _T("%u"), 1, 9999, 1, aircraft_id, this);

  AddSpacer();

  AddLabel(_("Do you want to upload this flight to WeGlide?"));

  SetFocus();
}

bool
UploadWidget::ShowUploadDialog()
{
  UploadDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(), _("Upload Flight"), this);
  ok_button = dialog.AddButton(_("OK"), mrOK);
  cancel_button = dialog.AddButton(_("Cancel"), mrCancel);
 
  bool do_upload = dialog.ShowModal() == mrOK;

  /* the caller manages the Widget */
  dialog.StealWidget();

  if (do_upload) {
    dialog.Hide();  // dialog is objectionable in the Background
    return UploadIGCFile(GetIGCPath(), GetUser(), GetAircraft());
  } else {
    return false;
  }
}

bool
UploadDialog::OnAnyKeyDown(unsigned key_code)
{
  switch (toupper(key_code)) {
  case 'Q':  // 'Quit
  case 'X':  // 'eXit
    WndForm::SetModalResult(mrCancel);
    return true;

  default:
    return WidgetDialog::OnAnyKeyDown(key_code);
  }
}


namespace WeGlide {

int 
FlightUploadDialog(const Path &igc_path, const WeGlide::User &user,
                          const uint_least32_t glider_id) noexcept
{
  UploadWidget widget(UIGlobals::GetDialogLook(), igc_path,
                             user, glider_id);

  return widget.ShowUploadDialog();
}

} // namespace WeGlide
