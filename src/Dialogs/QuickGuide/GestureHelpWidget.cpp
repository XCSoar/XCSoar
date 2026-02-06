// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GestureHelpWidget.hpp"
#include "Screen/Layout.hpp"
#include "Look/FontDescription.hpp"
#include "ui/canvas/Font.hpp"
#include "Language/Language.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/AnyCanvas.hpp"
#include "Renderer/TextRenderer.hpp"

#include <memory>
#include <winuser.h>

PixelSize GestureHelpWidget::GetMinimumSize() const noexcept {
  return { Layout::FastScale(200), Layout::FastScale(200) };
}

/**
 * Calculate scaled size maintaining aspect ratio.
 * @param src_size Original bitmap size
 * @param target_height Desired height after scaling
 * @return Scaled size with correct aspect ratio
 */
static PixelSize
ScaleSizeByHeight(PixelSize src_size, unsigned target_height) noexcept
{
  if (src_size.height == 0)
    return src_size;

  const double scale = double(target_height) / src_size.height;
  return {
    unsigned(src_size.width * scale),
    target_height
  };
}

/** Default gesture icon size (matches IDB_GESTURE_* resources). */
static constexpr PixelSize kGestureIconSize{64, 64};

/**
 * Cached fonts for gesture help layout.
 * Initialized once on first use to avoid repeated font loading.
 */
struct GestureHelpFonts {
  Font fontDefault;
  Font fontTitle;
  Font fontMono;
  Font fontSmall;
  bool initialized = false;

  void Initialize() noexcept {
    if (initialized)
      return;
    fontDefault.Load(FontDescription(Layout::VptScale(12), false));
    fontTitle.Load(FontDescription(Layout::VptScale(12), true));
    fontMono.Load(FontDescription(Layout::VptScale(12), true, false, true));
    fontSmall.Load(FontDescription(Layout::VptScale(10)));
    initialized = true;
  }
};

static GestureHelpFonts gesture_help_fonts;

unsigned
GestureHelpWindow::Layout(Canvas *canvas, const PixelRect &rc,
                          GestureHelpWindow *window) noexcept
{
  const PixelSize img_src_size = window != nullptr
    ? window->du_img.GetSize()
    : kGestureIconSize;

  // Scale gesture icons based on DPI (target ~32px scaled height)
  const unsigned target_icon_height = Layout::Scale(32u);
  const PixelSize img_size = ScaleSizeByHeight(img_src_size, target_icon_height);

  const int margin = Layout::FastScale(10);
  const int x_img = rc.left + margin;
  const int x_letter = x_img + int(img_size.width) + margin;
  const int x_text = x_img + int(img_size.width) + Layout::FastScale(5) + 2 * margin;
  int y = rc.top + margin;

  // Use cached fonts (initialized once)
  gesture_help_fonts.Initialize();
  const Font &fontDefault = gesture_help_fonts.fontDefault;
  const Font &fontTitle = gesture_help_fonts.fontTitle;
  const Font &fontMono = gesture_help_fonts.fontMono;
  const Font &fontSmall = gesture_help_fonts.fontSmall;

  TextRenderer renderer;

  // Create a measurement canvas for text size calculations when not painting
  std::unique_ptr<AnyCanvas> measure_canvas_ptr;
  if (canvas == nullptr) {
    measure_canvas_ptr = std::make_unique<AnyCanvas>();
  }
  // Helper to get text size using either the paint canvas or measurement canvas
  auto CalcTextSize = [&](const Font &font, const TCHAR *text) -> PixelSize {
    if (canvas != nullptr) {
      canvas->Select(font);
      return canvas->CalcTextSize(text);
    } else {
      measure_canvas_ptr->Select(font);
      return measure_canvas_ptr->CalcTextSize(text);
    }
  };

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
      rc.right - margin,
      rc.bottom
    };
    info_text_height = canvas->DrawFormattedText(info_text_rc, info_text, DT_LEFT);
  } else {
    info_text_height = renderer.GetHeight(fontDefault, info_width, info_text);
  }
  y += int(info_text_height) + margin;

  const TCHAR *basic_title_text = _("Basic gestures");
  const PixelSize basic_title_ps = CalcTextSize(fontTitle, basic_title_text);
  if (canvas != nullptr) {
    canvas->Select(fontTitle);
    canvas->DrawText({x_img, y}, basic_title_text);
    canvas->Select(fontDefault);
  }
  y += int(basic_title_ps.height) + margin;

  auto DrawGesture = [&](const Bitmap &bitmap, const TCHAR *text,
                         const TCHAR *letter=nullptr) {
    if (canvas != nullptr && window != nullptr) {
      const PixelSize src_size = bitmap.GetSize();
      canvas->Stretch({x_img, y}, img_size, bitmap, {0, 0}, src_size);
    }

    const PixelSize text_ps = CalcTextSize(fontDefault, text);
    if (canvas != nullptr && window != nullptr) {
      canvas->Select(fontDefault);
      canvas->DrawText({x_text, y + int(img_size.height / 2) - int(text_ps.height / 2)}, text);
    }

    if (letter != nullptr) {
      const PixelSize letter_ps = CalcTextSize(fontMono, letter);
      if (canvas != nullptr && window != nullptr) {
        canvas->Select(fontMono);
        canvas->DrawText({x_letter, y + int(img_size.height / 2) - int(letter_ps.height / 2)}, letter);
      }
    }

    y += int(img_size.height) + margin;
  };

  // Only draw gesture icons when we have a window with loaded bitmaps
  if (window != nullptr) {
    DrawGesture(window->du_img, _("Show main menu, also via double tap"), _("✓"));
    DrawGesture(window->up_img, _("Zoom in"));
    DrawGesture(window->down_img, _("Zoom out"));
    DrawGesture(window->left_img, _("Show next page"));
    DrawGesture(window->right_img, _("Show previous page"));
    DrawGesture(window->urdl_img, _("Pan-mode, also via two-finger pinch gesture"), _("P"));
  } else {
    // For measurement without window, skip actual icons but account for their space
    y += int(img_size.height + margin) * 6;
  }

  y += margin;

  const TCHAR *advanced_title_text = _("Advanced gestures");
  const PixelSize advanced_title_ps = CalcTextSize(fontTitle, advanced_title_text);
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
    // For measurement without window, skip actual icons but account for their space
    y += int(img_size.height + margin) * 9;
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
