// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkysightDialog.hpp"

#ifdef HAVE_HTTP

#include "DataGlobals.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Language/Language.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Widget/TextWidget.hpp"
#include "Widget/TextListWidget.hpp"
#include "Weather/Skysight/Skysight.hpp"

#include "util/StaticString.hxx"

class SkysightWidget final : public TextListWidget {
  std::shared_ptr<Skysight> skysight;
  ButtonPanelWidget *buttons_widget = nullptr;
  Button *activate_button = nullptr;
  Button *disable_button = nullptr;
  std::vector<StaticString<64>> rows;

public:
  explicit SkysightWidget(std::shared_ptr<Skysight> &&_skysight)
    :skysight(std::move(_skysight)) {}

  void SetButtonPanel(ButtonPanelWidget &_buttons) noexcept {
    buttons_widget = &_buttons;
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    CreateButtons(buttons_widget->GetButtonPanel());
    TextListWidget::Prepare(parent, rc);
    UpdateList();
  }

protected:
  const char *GetRowText(unsigned i) const noexcept override {
    return rows[i].c_str();
  }

  bool CanActivateItem(unsigned i) const noexcept override {
    return skysight != nullptr && i < skysight->NumLayers();
  }

  void OnActivateItem(unsigned i) noexcept override {
    ActivateClicked(i);
  }

private:
  void CreateButtons(ButtonPanel &buttons) {
    activate_button = buttons.Add(_("Activate"), [this]() {
      ActivateClicked(GetList().GetCursorIndex());
    });
    disable_button = buttons.Add(_("Disable"), [this]() {
      DisableClicked();
    });
  }

  void UpdateButtons() {
    if (activate_button == nullptr || disable_button == nullptr)
      return;

    activate_button->SetEnabled(skysight != nullptr && !rows.empty());
    disable_button->SetEnabled(skysight != nullptr && !skysight->GetActiveLayerId().empty());
  }

  void UpdateList() {
    rows.clear();

    if (skysight != nullptr) {
      rows.resize(skysight->NumLayers());

      for (std::size_t i = 0; i < rows.size(); ++i) {
        const auto *layer = skysight->GetLayer(i);
        rows[i] = layer != nullptr
          ? layer->name.c_str()
          : _("Unavailable");

        if (layer != nullptr && skysight->GetActiveLayerId() == layer->id)
          rows[i].AppendFormat(" [%s]", _("Active"));
      }
    }

    GetList().SetLength(rows.size());
    GetList().Invalidate();
    UpdateButtons();
  }

  void ActivateClicked(unsigned index) {
    if (skysight == nullptr || index >= skysight->NumLayers())
      return;

    const auto *layer = skysight->GetLayer(index);
    if (layer == nullptr)
      return;

    if (layer->requires_auth && !skysight->HasCredentials()) {
      ShowMessageBox(
        _("Configure your SkySight credentials in Weather settings before enabling live weather layers."),
        _("SkySight"), MB_OK);
      return;
    }

    (void)skysight->SetLayerActive(layer->id);
    UpdateList();
  }

  void DisableClicked() {
    if (skysight != nullptr)
      skysight->DeactivateLayer();

    UpdateList();
  }
};

std::unique_ptr<Widget>
CreateSkysightWidget()
{
  auto skysight = DataGlobals::GetSkysight();
  if (!skysight) {
    auto widget = std::make_unique<TextWidget>();
    widget->SetText(_("SkySight is unavailable."));
    return widget;
  }

  auto buttons = std::make_unique<ButtonPanelWidget>(
    std::make_unique<SkysightWidget>(std::move(skysight)),
    ButtonPanelWidget::Alignment::BOTTOM);
  static_cast<SkysightWidget &>(buttons->GetWidget()).SetButtonPanel(*buttons);
  return buttons;
}

#endif