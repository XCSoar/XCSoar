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
#include "Version.hpp"
#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

#include <algorithm>
#include <winuser.h>

WelcomeWindow::WelcomeWindow() noexcept
  : QuickGuideLinkWindow(),
#ifdef ENABLE_OPENGL
    logo_bitmap(IDB_LOGO_HD_RGBA),
    title_bitmap(IDB_TITLE_HD_RGBA)
#else
    logo_bitmap(IDB_LOGO_HD),
    title_bitmap(IDB_TITLE_HD)
#endif
{
  const auto count = static_cast<std::size_t>(LinkAction::COUNT);
  link_rects.resize(count);
}

/**
 * Calculate scaled size maintaining aspect ratio.
 * @param src_size Original bitmap size
 * @param target_width Desired width after scaling
 * @return Scaled size with correct aspect ratio
 */
static PixelSize
ScaleSize(PixelSize src_size, unsigned target_width) noexcept
{
  if (src_size.width == 0)
    return src_size;

  const double scale = double(target_width) / src_size.width;
  return {
    target_width,
    unsigned(src_size.height * scale)
  };
}

static unsigned
LayoutWelcome(Canvas *canvas, const PixelRect &rc,
              WelcomeWindow *window) noexcept
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

  // Scale logo to fit available width (max 50% of width, scaled for DPI)
  const unsigned max_logo_width = std::min(Layout::Scale(160u),
                                           width_u > margin_u ? (width_u - margin_u) / 2 : 160u);

  // Use cached bitmaps from window if available, otherwise use known HD dimensions
  // Logo HD is 160x160 (square), Title HD is 320x53 (6:1 aspect ratio from SVG source)
  static constexpr PixelSize kLogoHdSize{160, 160};
  static constexpr PixelSize kTitleHdSize{320, 53};

  const PixelSize logo_src_size = (window != nullptr)
    ? window->GetLogoBitmap().GetSize() : kLogoHdSize;
  const PixelSize title_src_size = (window != nullptr)
    ? window->GetTitleBitmap().GetSize() : kTitleHdSize;

  const PixelSize logo_size = ScaleSize(logo_src_size, max_logo_width);
  const int x_logo = rc.GetWidth() / 2 - int(logo_size.width / 2);
  if (canvas != nullptr && window != nullptr) {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
#endif
    canvas->Stretch({x_logo, y}, logo_size, window->GetLogoBitmap(),
                    {0, 0}, logo_src_size);
  }
  y += int(logo_size.height) + half_margin;

  // Scale title to fit available width (max 80% of width, scaled for DPI)
  const unsigned max_title_width = std::min(Layout::Scale(320u),
                                            width_u > margin_u ? (width_u - margin_u) * 4 / 5 : 320u);

  const PixelSize title_size = ScaleSize(title_src_size, max_title_width);
  const int x_title = rc.GetWidth() / 2 - int(title_size.width / 2);
  if (canvas != nullptr && window != nullptr) {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
#endif
    canvas->Stretch({x_title, y}, title_size, window->GetTitleBitmap(),
                    {0, 0}, title_src_size);
  }
  y += int(title_size.height) + margin * 2;

  // Build welcome message with version
  StaticString<128> t0;
  t0.Format("%s %s", _("Welcome to XCSoar"), XCSoar_Version);
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
  t1 += " ";
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
  t2 += " ";
  t2 += _("You can always access the latest versions online:");
  if (canvas != nullptr)
    canvas->Select(look.text_font);
  const PixelRect t2_rc{x, y, rc.right - margin, rc.bottom};
  const unsigned t2_height = canvas != nullptr
    ? canvas->DrawFormattedText(t2_rc, t2.c_str(), DT_LEFT)
    : renderer.GetHeight(look.text_font, text_width, t2.c_str());
  y += int(t2_height) + margin;

  const char *t3 = "https://xcsoar.org/discover/manual.html";
  if (canvas != nullptr)
    canvas->Select(look.text_font);
  const PixelRect t3_rc{x, y, rc.right - margin, rc.bottom};
  unsigned t3_height;
  if (canvas != nullptr && window != nullptr) {
    t3_height = window->DrawLink(*canvas,
                                 static_cast<std::size_t>(WelcomeWindow::LinkAction::XCSOAR_MANUAL),
                                 t3_rc, t3);
  } else {
    t3_height = renderer.GetHeight(look.text_font, text_width, t3);
  }
  y += int(t3_height) + margin;

  StaticString<1024> t4;
  t4 = _("The XCSoar documentation is actively maintained, but some "
         "topics may still be missing or outdated.");
  t4 += " ";
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

  const char *t5 = "https://github.com/XCSoar/XCSoar";
  if (canvas != nullptr)
    canvas->Select(look.text_font);
  const PixelRect t5_rc{x, y, rc.right - margin, rc.bottom};
  unsigned t5_height;
  if (canvas != nullptr && window != nullptr) {
    t5_height = window->DrawLink(*canvas,
                                 static_cast<std::size_t>(WelcomeWindow::LinkAction::GITHUB),
                                 t5_rc, t5);
  } else {
    t5_height = renderer.GetHeight(look.text_font, text_width, t5);
  }
  y += int(t5_height) + margin;

  return static_cast<unsigned>(y);
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
  const unsigned height = LayoutWelcome(nullptr, measure_rc, nullptr);
  if (height > size.height)
    size.height = height;

  return size;
}

void
WelcomeWidget::Initialise(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();
  style.TabStop();
  auto w = std::make_unique<WelcomeWindow>();
  w->Create(parent, rc, style);
  SetWindow(std::move(w));
}

bool
WelcomeWidget::SetFocus() noexcept
{
  if (!IsDefined())
    return false;
  GetWindow().SetFocus();
  return true;
}

void
WelcomeWindow::OnPaint(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();

  canvas.Clear();

  LayoutWelcome(&canvas, rc, this);
}

bool
WelcomeWindow::OnLinkActivated(std::size_t index) noexcept
{
  switch (static_cast<LinkAction>(index)) {
  case LinkAction::XCSOAR_MANUAL:
    return OpenLink("https://xcsoar.org/discover/manual.html");
  case LinkAction::GITHUB:
    return OpenLink("https://github.com/XCSoar/XCSoar");
  case LinkAction::COUNT:
    break;
  }
  return false;
}

