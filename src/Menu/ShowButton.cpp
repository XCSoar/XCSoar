// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ShowButton.hpp"
#include "Renderer/ButtonRenderer.hpp"
#include "Renderer/SymbolButtonRenderer.hpp"
#include "Look/ButtonLook.hpp"
#include "Input/InputEvents.hpp"
#include "Interface.hpp"
#include "UIState.hpp"

#include <memory>

#ifdef ANDROID
#include "Hardware/RotateDisplay.hpp"
#include "DisplayOrientation.hpp"
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#include "Renderer/BitmapButtonRenderer.hpp"
#include "java/Global.hxx"
#endif

/**
 * Map overlay buttons (menu, zoom) are hidden on special pages.
 */
class ShowMapOverlayButtonRenderer : public ButtonRenderer {
  std::unique_ptr<ButtonRenderer> inner;

public:
  explicit ShowMapOverlayButtonRenderer(std::unique_ptr<ButtonRenderer> _inner) noexcept
    :inner(std::move(_inner)) {}

  unsigned GetMinimumButtonWidth() const noexcept override {
    return inner->GetMinimumButtonWidth();
  }

  void DrawButton(Canvas &canvas, const PixelRect &rc,
                  ButtonState state) const noexcept override {
    if (CommonInterface::GetUIState().pages.special_page.IsDefined())
      return;

    inner->DrawButton(canvas, rc, state);
  }
};

static std::unique_ptr<ButtonRenderer>
MakeMapOverlaySymbolButton(const ButtonLook &look,
                           const char *caption) noexcept
{
  return std::make_unique<ShowMapOverlayButtonRenderer>(
    std::make_unique<SymbolButtonRenderer>(look, caption));
}

void
ShowMenuButton::Create(ContainerWindow &parent, const ButtonLook &look,
                       const PixelRect &rc, WindowStyle style) noexcept
{
  Button::Create(parent, rc, style, MakeMapOverlaySymbolButton(look, "h"));
}

bool
ShowMenuButton::OnClicked() noexcept
{
  InputEvents::ShowMenu();
  return true;
}

void
ShowZoomButton::Create(ContainerWindow &parent, const ButtonLook &look,
                       const PixelRect &rc, Sign _sign,
                       WindowStyle style) noexcept
{
  sign = _sign;
  Button::Create(parent, rc, style,
                 MakeMapOverlaySymbolButton(look,
                                          sign == Sign::ZOOM_IN ? "+" : "-"));
}

bool
ShowZoomButton::OnClicked() noexcept
{
  InputEvents::eventZoom(sign == Sign::ZOOM_IN ? "in" : "out");
  return true;
}

#ifdef ANDROID

#include "Resources.hpp"

void
ShowRotateButton::Create(ContainerWindow &parent, const PixelRect &rc,
                         WindowStyle style) noexcept
{
  bitmap.Load(IDB_ROTATE);
  Button::Create(parent, rc, style,
                 std::make_unique<BitmapButtonRenderer>(bitmap, true));
}

bool
ShowRotateButton::OnClicked() noexcept
{
  /* query the device sensor for the current physical orientation
     and rotate to it */
  if (native_view != nullptr) {
    auto orientation = static_cast<DisplayOrientation>(
      native_view->GetPhysicalOrientation(Java::GetEnv()));
    if (orientation != DisplayOrientation::DEFAULT)
      Display::Rotate(orientation);
  }

  /* hide the button immediately */
  Hide();

  return true;
}

#endif /* ANDROID */
