// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeGlideDialog.hpp"

#include "Dialogs/Settings/Panels/WeGlideConfigPanel.hpp"
#include "Dialogs/Settings/WeGlideDeviceSignIn.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Weather/WeGlide/PageOverlay.hpp"
#include "WeatherCredentialGateWidget.hpp"
#include "WeatherOverlayDraft.hpp"
#include "Widget/RowFormWidget.hpp"
#include "net/client/WeGlide/Settings.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace {

std::string_view
CanonicalLayer(std::string_view id) noexcept
{
  return id.empty() ? "thermal" : id;
}

class WeGlideWeatherWidget final : public RowFormWidget {
  enum Controls {
    STATUS,
    LAYER,
  };

  WeatherOverlayDraft::State overlay;
  std::vector<std::string> layer_ids;
  Button *account_button = nullptr;
  Button *apply_button = nullptr;
  Button *add_page_button = nullptr;

  void RefreshStatus() noexcept;
  void UpdateAccountButton() noexcept;
  void OnLayerModified() noexcept;

public:
  WeGlideWeatherWidget() noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
};

void
WeGlideWeatherWidget::RefreshStatus() noexcept
{
  const auto &settings = CommonInterface::GetComputerSettings().weglide;
  const char *text;
  if (!settings.enabled)
    text = _("Disabled");
  else if (!WeGlideAccountConnected())
    text = _("Not connected");
  else
    text = _("Connected");
  SetText(STATUS, text);
}

void
WeGlideWeatherWidget::UpdateAccountButton() noexcept
{
  if (account_button == nullptr)
    return;

  account_button->SetCaption(WeGlideAccountConnected()
                              ? C_("Button", "Disconnect")
                              : C_("Button", "Connect"));
}

void
WeGlideWeatherWidget::OnLayerModified() noexcept
{
  const unsigned which = GetValueEnum(LAYER);
  if (which >= layer_ids.size())
    return;

  const std::string_view id = layer_ids[which];
  if (CanonicalLayer(overlay.draft.weglide_layer.c_str()) == id)
    return;

  overlay.draft.weglide_layer = id == "thermal" ? "" : id.data();
  overlay.SyncButtons(apply_button, add_page_button);
}

void
WeGlideWeatherWidget::Prepare(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  overlay.Load(PageLayout::Overlay::WEGLIDE);

  AddReadOnly(_("Status"));
  RefreshStatus();

  auto *layer = AddEnum(_("Layer"), nullptr);
  auto &df = static_cast<DataFieldEnum &>(*layer->GetDataField());
  const auto &catalog = WeGlide::Catalog();
  if (catalog.empty()) {
    df.AddChoice(0, "thermal");
    df.AddChoice(1, "vario");
    layer_ids.emplace_back("thermal");
    layer_ids.emplace_back("vario");
  } else {
    for (unsigned i = 0; i < catalog.size(); ++i) {
      const char *label = catalog[i].name.empty()
        ? catalog[i].id.c_str() : catalog[i].name.c_str();
      df.AddChoice(i, label);
      layer_ids.emplace_back(catalog[i].id);
    }
  }

  const auto selected = CanonicalLayer(overlay.draft.weglide_layer.c_str());
  unsigned index = 0;
  for (unsigned i = 0; i < layer_ids.size(); ++i)
    if (layer_ids[i] == selected)
      index = i;
  df.SetValue(index);
  df.SetOnModified([this] { OnLayerModified(); });

  account_button = AddButton(C_("Button", "Connect"), [this] {
    if (WeGlideAccountConnected())
      ForgetWeGlideAccount();
    else if (CommonInterface::GetComputerSettings().weglide.enabled)
      RunWeGlideDeviceSignIn();
    UpdateAccountButton();
    RefreshStatus();
  });
  UpdateAccountButton();

  apply_button = AddButton(C_("Button", "Apply to page"), [this] {
    if (overlay.ApplyIfDirty())
      overlay.SyncButtons(apply_button, add_page_button);
  });
  add_page_button = AddButton(C_("Button", "Add page"), [this] {
    overlay.AddPage(apply_button, add_page_button);
  });
  AddButton(C_("Button", "Pages setup"), [] {
    WeatherOverlayDraft::OpenPagesConfig();
  });

  overlay.SyncButtons(apply_button, add_page_button);
}

void
WeGlideWeatherWidget::Show(const PixelRect &rc) noexcept
{
  RowFormWidget::Show(rc);
  RefreshStatus();
  UpdateAccountButton();
}

bool
WeGlideWeatherReady() noexcept
{
  const auto &settings = CommonInterface::GetComputerSettings().weglide;
  if (!settings.enabled)
    return false;
  return settings.IsConfigured() || WeGlideAccountConnected();
}

} // namespace

std::unique_ptr<Widget>
CreateWeGlideWeatherTab() noexcept
{
  return CreateWeatherCredentialGateWidget(
    WeGlideWeatherReady, CreateWeGlideConfigPanel, [] {
      return std::make_unique<WeGlideWeatherWidget>();
    });
}
