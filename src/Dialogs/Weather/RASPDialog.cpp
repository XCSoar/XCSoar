// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RASPDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Weather/Rasp/Configured.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Button.hpp"
#include "Form/Edit.hpp"
#include "Repository/FileType.hpp"
#include "DataGlobals.hpp"
#include "UIGlobals.hpp"
#include "UtilsSettings.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"
#include "net/http/Features.hpp"
#ifdef HAVE_DOWNLOAD_MANAGER
#include "Dialogs/FileManager.hpp"
#include "net/http/DownloadManager.hpp"
#endif

class RASPSettingsPanel final
  : public RowFormWidget {

  enum Controls {
    FILE,
    MODIFIED,
  };

  std::shared_ptr<RaspStore> rasp;

#ifdef HAVE_DOWNLOAD_MANAGER
  Button *update_button = nullptr;
#endif

  void ReloadRasp();
  void UpdateModifiedDisplay();
  void UpdateClicked();

public:
  explicit RASPSettingsPanel(std::shared_ptr<RaspStore> &&_rasp) noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()),
     rasp(std::move(_rasp)) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
RASPSettingsPanel::ReloadRasp()
{
  rasp = LoadConfiguredRasp(false);
  DataGlobals::SetRasp(rasp);
  RaspFileChanged = true;
  Profile::Save();
  UpdateModifiedDisplay();
}

void
RASPSettingsPanel::UpdateModifiedDisplay()
{
  StaticString<32> buffer;

  if (rasp != nullptr) {
    const BrokenDateTime modified = rasp->GetFileModifiedTime();
    if (modified.IsPlausible()) {
      buffer.Format("%04u-%02u-%02u %02u:%02u",
                      modified.year, modified.month, modified.day,
                      modified.hour, modified.minute);
    }
  }

  if (buffer.empty())
    buffer = _("Unknown");

  SetText(MODIFIED, buffer.c_str());
}

void
RASPSettingsPanel::UpdateClicked()
{
#ifdef HAVE_DOWNLOAD_MANAGER
  ShowFileManager();

  const AllocatedPath profile_path = Profile::GetPath(ProfileKeys::RaspFile);
  if (profile_path != nullptr)
    LoadValue(FILE, Path(profile_path));

  ReloadRasp();
#endif
}

void
RASPSettingsPanel::Prepare([[maybe_unused]] ContainerWindow &parent,
                           [[maybe_unused]] const PixelRect &rc) noexcept
{
  WndProperty *wp = AddFile(_("File"), nullptr,
                            ProfileKeys::RaspFile,
                            GetFileTypePatterns(FileType::RASP),
                            FileType::RASP);
  wp->GetDataField()->SetOnModified([this]{
    if (SaveValueFileReader(FILE, ProfileKeys::RaspFile)) {
      ReloadRasp();
      GetControl(FILE).RefreshDisplay();
    }
  });

  AddReadOnly(_("Modified"),
              _("Local date and time of the selected RASP file."));
  UpdateModifiedDisplay();

#ifdef HAVE_DOWNLOAD_MANAGER
  update_button = AddButton(_("Update"), [this]{ UpdateClicked(); });
  if (!Net::DownloadManager::IsAvailable())
    update_button->SetEnabled(false);
#endif
}

bool
RASPSettingsPanel::Save([[maybe_unused]] bool &_changed) noexcept
{
  return true;
}

std::unique_ptr<Widget>
CreateRaspWidget() noexcept
{
  auto rasp = DataGlobals::GetRasp();
  return std::make_unique<RASPSettingsPanel>(std::move(rasp));
}
