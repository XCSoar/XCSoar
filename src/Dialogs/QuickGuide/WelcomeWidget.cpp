// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WelcomeWidget.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Look/FontDescription.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "Resources.hpp"
#include "Language/Language.hpp"
#include "util/OpenLink.hpp"
#include "util/StaticString.hxx"
#include "Renderer/TextRenderer.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

#include <winuser.h>

struct WelcomeLayoutResult {
  unsigned height;
  PixelRect xcsoar_link_rect;
  PixelRect github_link_rect;
};

static WelcomeLayoutResult
LayoutWelcome(Canvas *canvas, const PixelRect &rc) noexcept
{
  const int margin = Layout::FastScale(10);
  const int half_margin = margin / 2;
  const int x = rc.left + margin;
  const unsigned width_u = rc.GetWidth();
  const unsigned margin_u = static_cast<unsigned>(margin * 2);
  const int text_width = width_u > margin_u
    ? int(width_u - margin_u)
    : 0;

  const DialogLook &look = UIGlobals::GetDialogLook();
  TextRenderer renderer;

  int y = rc.top + margin;

  if (canvas != nullptr) {
    canvas->SetBackgroundTransparent();
    canvas->SetTextColor(COLOR_BLACK);
  }

#ifdef ENABLE_OPENGL
  // OpenGL builds use RGBA PNGs for transparency.
  Bitmap logo_alpha(IDB_LOGO_HD_RGBA);
  Bitmap title_alpha(IDB_TITLE_HD_RGBA);
  PixelSize logo_size = logo_alpha.GetSize();
  const int x_logo = rc.GetWidth() / 2 - (logo_size.width / 2);
  if (canvas != nullptr) {
    const ScopeAlphaBlend alpha_blend;
    canvas->Copy({x_logo, y}, logo_size, logo_alpha, {0, 0});
  }
#else
  // Non-OpenGL builds use opaque bitmaps.
  Bitmap logo(IDB_LOGO_HD);
  PixelSize logo_size = logo.GetSize();
  const int x_logo = rc.GetWidth() / 2 - (logo_size.width / 2);
  if (canvas != nullptr)
    canvas->Copy({x_logo, y}, logo_size, logo, {0, 0});
#endif
  y += int(logo_size.height) + half_margin;

#ifdef ENABLE_OPENGL
  PixelSize title_size = title_alpha.GetSize();
  const int x_title = rc.GetWidth() / 2 - (title_size.width / 2);
  if (canvas != nullptr) {
    const ScopeAlphaBlend alpha_blend;
    canvas->Copy({x_title, y}, title_size, title_alpha, {0, 0});
  }
#else
  Bitmap title(IDB_TITLE_HD);
  PixelSize title_size = title.GetSize();
  const int x_title = rc.GetWidth() / 2 - (title_size.width / 2);
  if (canvas != nullptr)
    canvas->Copy({x_title, y}, title_size, title, {0, 0});
#endif
  y += int(title_size.height) + margin * 2;

  const TCHAR *t0 = _("Welcome to XCSoar");
  if (canvas != nullptr)
    canvas->Select(look.bold_font);
  const PixelRect t0_rc{x, y, rc.right - margin, rc.bottom};
  const unsigned t0_height = canvas != nullptr
    ? canvas->DrawFormattedText(t0_rc, t0, DT_LEFT)
    : renderer.GetHeight(look.bold_font, text_width, t0);
  y += int(t0_height) + margin;

  StaticString<1024> t1;
  t1 = _("To get the most out of XCSoar and to learn about its many "
         "functions in detail, it is highly recommended to read the "
         "Quick Guide or the complete documentation.");
  t1 += _T(" ");
  t1 += _("The manuals explain step by step how to use XCSoar "
          "efficiently and cover both basic and advanced features.");
  if (canvas != nullptr)
    canvas->Select(look.text_font);
  const PixelRect t1_rc{x, y, rc.right - margin, rc.bottom};
  const unsigned t1_height = canvas != nullptr
    ? canvas->DrawFormattedText(t1_rc, t1.c_str(), DT_LEFT)
    : renderer.GetHeight(look.text_font, text_width, t1.c_str());
  y += int(t1_height) + margin;

  StaticString<512> t2;
  t2 = _("Documentation is available in several languages, including "
         "English, German, French, and Portuguese.");
  t2 += _T(" ");
  t2 += _("You can always access the latest versions online:");
  if (canvas != nullptr)
    canvas->Select(look.text_font);
  const PixelRect t2_rc{x, y, rc.right - margin, rc.bottom};
  const unsigned t2_height = canvas != nullptr
    ? canvas->DrawFormattedText(t2_rc, t2.c_str(), DT_LEFT)
    : renderer.GetHeight(look.text_font, text_width, t2.c_str());
  y += int(t2_height) + margin;

  const TCHAR *t3 = _("https://xcsoar.org/discover/manual.html");
  if (canvas != nullptr) {
    canvas->SetTextColor(COLOR_BLUE);
    canvas->Select(look.text_font);
  }
  const PixelRect t3_rc{x, y, rc.right - margin, rc.bottom};
  const unsigned t3_height = canvas != nullptr
    ? canvas->DrawFormattedText(t3_rc, t3, DT_LEFT | DT_UNDERLINE)
    : renderer.GetHeight(look.text_font, text_width, t3);
  const PixelRect xcsoar_link_rect{x, y, rc.right - margin, y + int(t3_height)};
  if (canvas != nullptr)
    canvas->SetTextColor(COLOR_BLACK);
  y += int(t3_height) + margin;

  StaticString<1024> t4;
  t4 = _("The XCSoar documentation is actively maintained, but some "
         "topics may still be missing or outdated.");
  t4 += _T(" ");
  t4 += _("If you find mistakes or have ideas for improvement, "
          "please contribute — XCSoar is open source and thrives on "
          "community input:");
  if (canvas != nullptr)
    canvas->Select(look.text_font);
  const PixelRect t4_rc{x, y, rc.right - margin, rc.bottom};
  const unsigned t4_height = canvas != nullptr
    ? canvas->DrawFormattedText(t4_rc, t4.c_str(), DT_LEFT)
    : renderer.GetHeight(look.text_font, text_width, t4.c_str());
  y += int(t4_height) + margin;

  const TCHAR *t5 = _("https://github.com/XCSoar/XCSoar");
  if (canvas != nullptr) {
    canvas->SetTextColor(COLOR_BLUE);
    canvas->Select(look.text_font);
  }
  const PixelRect t5_rc{x, y, rc.right - margin, rc.bottom};
  const unsigned t5_height = canvas != nullptr
    ? canvas->DrawFormattedText(t5_rc, t5, DT_LEFT | DT_UNDERLINE)
    : renderer.GetHeight(look.text_font, text_width, t5);
  const PixelRect github_link_rect{x, y, rc.right - margin, y + int(t5_height)};
  if (canvas != nullptr)
    canvas->SetTextColor(COLOR_BLACK);
  y += int(t5_height) + margin;

  return {
    static_cast<unsigned>(y),
    xcsoar_link_rect,
    github_link_rect,
  };
}

PixelSize WelcomeWidget::GetMinimumSize() const noexcept {
  return { Layout::FastScale(200), Layout::FastScale(200) };
}

PixelSize WelcomeWidget::GetMaximumSize() const noexcept {
  PixelSize size = GetMinimumSize();
  size.width = Layout::FastScale(300);

  unsigned width = size.width;
  if (IsDefined())
    width = GetWindow().GetSize().width;

  const PixelRect measure_rc{PixelPoint{0, 0}, PixelSize{width, 0u}};
  const auto layout = LayoutWelcome(nullptr, measure_rc);
  if (layout.height > size.height)
    size.height = layout.height;

  return size;
}

void
WelcomeWidget::Initialise(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();
  auto w = std::make_unique<WelcomeWindow>();
  w->Create(parent, rc, style);
  SetWindow(std::move(w));
}

void
WelcomeWindow::OnPaint(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();

  canvas.Clear();

  const auto layout = LayoutWelcome(&canvas, rc);
  xcsoar_link_rect = layout.xcsoar_link_rect;
  github_link_rect = layout.github_link_rect;
}

bool WelcomeWindow::OnMouseUp(PixelPoint p) noexcept {
  if (xcsoar_link_rect.Contains(p)) {
    return OpenLink("https://xcsoar.org/discover/manual.html");
  }
  if (github_link_rect.Contains(p)) {
    return OpenLink("https://github.com/XCSoar/XCSoar");
  }
  return false;
}
#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif
