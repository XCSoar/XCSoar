// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GestureHelpWidget.hpp"
#include "Screen/Layout.hpp"
#include "Look/FontDescription.hpp"
#include "Language/Language.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/AnyCanvas.hpp"
#include "Renderer/TextRenderer.hpp"

#include <winuser.h>

PixelSize GestureHelpWidget::GetMinimumSize() const noexcept {
  return { Layout::FastScale(200), Layout::FastScale(200) };
}

static PixelSize
CalcTextSizeSingle(const Font &font, const TCHAR *text) noexcept
{
  AnyCanvas canvas;
  canvas.Select(font);
  return canvas.CalcTextSize(text);
}

unsigned
GestureHelpWindow::Layout(Canvas *canvas, const PixelRect &rc,
                          GestureHelpWindow *window) noexcept
{
  Bitmap tmp_du(IDB_GESTURE_DU);
  const PixelSize img_size = window != nullptr
    ? window->du_img.GetSize()
    : tmp_du.GetSize();

  const int margin = Layout::FastScale(10);
  const int x_img = rc.left + margin;
  const int x_letter = x_img + img_size.width + margin;
  const int x_text = x_img + img_size.width + Layout::FastScale(5) + 2 * margin;
  int y = rc.top + margin;

  Font fontDefault;
  fontDefault.Load(FontDescription(Layout::VptScale(12), false));

  Font fontTitle;
  fontTitle.Load(FontDescription(Layout::VptScale(12), true));

  Font fontMono;
  fontMono.Load(FontDescription(Layout::VptScale(12), true, false, true));

  Font fontSmall;
  fontSmall.Load(FontDescription(Layout::VptScale(10)));

  TextRenderer renderer;

  if (canvas != nullptr) {
    canvas->Select(fontDefault);
    canvas->SetBackgroundTransparent();
    canvas->SetTextColor(COLOR_BLACK);
  }

  const TCHAR *info_text = _("The following gestures can be drawn on the map view.");
  const unsigned width_u = rc.GetWidth();
  const unsigned margin_u = static_cast<unsigned>(margin * 2);
  const unsigned info_width = width_u > margin_u ? width_u - margin_u : 0;
  unsigned info_text_height;
  if (canvas != nullptr) {
    PixelRect info_text_rc{
      margin,
      margin,
      int(canvas->GetWidth()) - margin,
      int(canvas->GetHeight())
    };
    info_text_height = canvas->DrawFormattedText(info_text_rc, info_text, DT_LEFT);
  } else {
    info_text_height = renderer.GetHeight(fontDefault, info_width, info_text);
  }
  y += int(info_text_height) + margin;

  const TCHAR *basic_title_text = _("Basic gestures");
  const PixelSize basic_title_ps = CalcTextSizeSingle(fontTitle, basic_title_text);
  if (canvas != nullptr) {
    canvas->Select(fontTitle);
    canvas->DrawText({x_img, y}, basic_title_text);
    canvas->Select(fontDefault);
  }
  y += int(basic_title_ps.height) + margin;

  auto DrawGesture = [&](const Bitmap &bitmap, const TCHAR *text,
                         const TCHAR *letter=nullptr) {
    if (canvas != nullptr && window != nullptr)
      canvas->Copy({x_img, y}, img_size, bitmap, {0, 0});

    const PixelSize text_ps = CalcTextSizeSingle(fontDefault, text);
    if (canvas != nullptr && window != nullptr) {
      canvas->Select(fontDefault);
      canvas->DrawText({x_text, y + int(img_size.height / 2) - int(text_ps.height / 2)}, text);
    }

    if (letter != nullptr) {
      const PixelSize letter_ps = CalcTextSizeSingle(fontMono, letter);
      if (canvas != nullptr && window != nullptr) {
        canvas->Select(fontMono);
        canvas->DrawText({x_letter, y + int(img_size.height / 2) - int(letter_ps.height / 2)}, letter);
      }
    }

    y += int(img_size.height) + margin;
  };

  if (window != nullptr) {
    DrawGesture(window->du_img, _("Show main menu, also via double tap"), _("✓"));
    DrawGesture(window->up_img, _("Zoom in"));
    DrawGesture(window->down_img, _("Zoom out"));
    DrawGesture(window->left_img, _("Show next page"));
    DrawGesture(window->right_img, _("Show previous page"));
    DrawGesture(window->urdl_img, _("Pan-mode, also via two-finger pinch gesture"), _("P"));
  } else {
    DrawGesture(tmp_du, _("Show main menu, also via double tap"), _("✓"));
    DrawGesture(tmp_du, _("Zoom in"));
    DrawGesture(tmp_du, _("Zoom out"));
    DrawGesture(tmp_du, _("Show next page"));
    DrawGesture(tmp_du, _("Show previous page"));
    DrawGesture(tmp_du, _("Pan-mode, also via two-finger pinch gesture"), _("P"));
  }

  y += margin;

  const TCHAR *advanced_title_text = _("Advanced gestures");
  const PixelSize advanced_title_ps = CalcTextSizeSingle(fontTitle, advanced_title_text);
  if (canvas != nullptr) {
    canvas->Select(fontTitle);
    canvas->DrawText({x_img, y}, advanced_title_text);
    canvas->Select(fontDefault);
  }
  y += int(advanced_title_ps.height) + margin;

  if (window != nullptr) {
    DrawGesture(window->dr_img, _("Show Select Waypoint list"), _("L"));
    DrawGesture(window->rd_img, _("Open Task Manager"), _("T"));
    DrawGesture(window->dl_img, _("Show Alternates List"));
    DrawGesture(window->ud_img, _("Enable Auto-Zoom"));
    DrawGesture(window->ldr_img, _("Show checklist"), _("C"));
    DrawGesture(window->urd_img, _("Show Analysis dialogue"), _("A"));
    DrawGesture(window->ldrdl_img, _("Open Status dialogue"), _("S"));
    DrawGesture(window->uldr_img, _("Access quick menu"), _("Q"));
    DrawGesture(window->rl_img, _("FLARM: Switches selected aircraft displaying values such as climb rate relative altitude, etc."));
  } else {
    DrawGesture(tmp_du, _("Show Select Waypoint list"), _("L"));
    DrawGesture(tmp_du, _("Open Task Manager"), _("T"));
    DrawGesture(tmp_du, _("Show Alternates List"));
    DrawGesture(tmp_du, _("Enable Auto-Zoom"));
    DrawGesture(tmp_du, _("Show checklist"), _("C"));
    DrawGesture(tmp_du, _("Show Analysis dialogue"), _("A"));
    DrawGesture(tmp_du, _("Open Status dialogue"), _("S"));
    DrawGesture(tmp_du, _("Access quick menu"), _("Q"));
    DrawGesture(tmp_du, _("FLARM: Switches selected aircraft displaying values such as climb rate relative altitude, etc."));
  }

  const TCHAR *aresti_info_text = _("Gestures are displayed in Aresti notation, where the circle indicates the start and the dash indicates the end.");
  unsigned aresti_height;
  if (canvas != nullptr) {
    canvas->Select(fontSmall);
    PixelRect aresti_rc{x_img, y, int(canvas->GetWidth()) - margin,
                        int(canvas->GetHeight())};
    aresti_height = canvas->DrawFormattedText(aresti_rc, aresti_info_text, DT_LEFT);
  } else {
    aresti_height = renderer.GetHeight(fontSmall, info_width, aresti_info_text);
  }
  y += int(aresti_height);

  return static_cast<unsigned>(y);
}

PixelSize GestureHelpWidget::GetMaximumSize() const noexcept {
  PixelSize size = GetMinimumSize();
  size.width = Layout::FastScale(300);

  unsigned width = size.width;
  if (IsDefined())
    width = GetWindow().GetSize().width;

  const PixelRect measure_rc{PixelPoint{0, 0}, PixelSize{width, 0u}};
  const unsigned height = GestureHelpWindow::Layout(nullptr, measure_rc, nullptr);
  if (height > size.height)
    size.height = height;

  return size;
}

void GestureHelpWidget::Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept {
  WindowStyle style;
  style.Hide();
  auto w = std::make_unique<GestureHelpWindow>();
  w->Create(parent, rc, style);
  SetWindow(std::move(w));
}

void
GestureHelpWindow::OnPaint(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();

  canvas.Clear();
  Layout(&canvas, rc, this);
}
