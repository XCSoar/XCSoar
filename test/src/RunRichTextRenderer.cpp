// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_CMDLINE
#define ENABLE_DIALOG
#define ENABLE_MAIN_WINDOW
#define USAGE "[OPTION]... [FILE]"

#include "Main.hpp"
#include "QuickGuideNEWS.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/VScrollWidget.hpp"
#include "Widget/RichTextWidget.hpp"
#include "ui/control/RichTextWindow.hpp"
#include "ui/canvas/TextWrapper.hpp"
#include "ui/canvas/AnyCanvas.hpp"
#include "util/MarkdownParser.hpp"
#include "ProductName.hpp"
#include "Version.hpp"
#include "system/StandardVersion.hpp"
#include "system/Path.hpp"
#include "system/Args.hpp"
#include "io/FileLineReader.hpp"
#include "util/StringCompare.hxx"
#include "util/Compiler.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

static constexpr const char canonical_name[] = "RunRichTextRenderer";

struct Options {
  bool benchmark = false;
  bool plain = false;
  unsigned layout_width = 0;
  const char *file = nullptr;
};

static Options options;

static void
PrintStandardHelp() noexcept
{
  std::printf(
    "Usage: %s [OPTION]... [FILE]\n"
    "\n"
    "Show scrollable rich text (Markdown) in a full-screen dialog, or\n"
    "benchmark layout without opening the UI.\n"
    "\n"
    "With no FILE, uses the Quick Guide release-notes Markdown generated\n"
    "from NEWS.txt at build time.\n"
    "\n"
    "Options:\n"
    "  --benchmark           measure parse, wrap, and layout; exit\n"
    "  --plain               treat input as plain text (no Markdown)\n"
    "  --width=PIXELS        text width for --benchmark (default: 560)\n"
    "  -WxH                  window size (e.g. -600x800 for Kobo)\n"
    "  -h, --help            display this help and exit\n"
    "  --version             output version information and exit\n"
    "\n"
    "Interactive mode: scroll with keys or mouse; Esc closes.\n"
    "\n"
    "Report bugs to: <%s>\n"
    "%s home page: <%s>\n",
    canonical_name, PRODUCT_BUGS_URL, PRODUCT_NAME, PRODUCT_WEB_SITE_URL);
}

static void
ParseCommandLine(Args &args)
{
  while (!args.IsEmpty()) {
    const char *arg = args.PeekNext();
    if (arg == nullptr || arg[0] != '-')
      break;

    if (StringIsEqual(arg, "-h") || StringIsEqual(arg, "--help")) {
      PrintStandardHelp();
      std::exit(EXIT_SUCCESS);
    }

    if (StringIsEqual(arg, "--version")) {
      PrintStandardVersion(canonical_name, XCSoar_Version);
      std::exit(EXIT_SUCCESS);
    }

    if (StringIsEqual(arg, "--benchmark")) {
      args.GetNext();
      options.benchmark = true;
      continue;
    }

    if (StringIsEqual(arg, "--plain")) {
      args.GetNext();
      options.plain = true;
      continue;
    }

    if (StringStartsWith(arg, "--width=")) {
      args.GetNext();
      char *end = nullptr;
      const unsigned w = ParseUnsigned(arg + 8, &end);
      if (end == nullptr || *end != '\0' || w == 0)
        args.UsageError();
      options.layout_width = w;
      continue;
    }

    break;
  }

  if (!args.IsEmpty())
    options.file = args.GetNext();
}

static std::string
ReadTextFile(const Path &path)
{
  FileLineReaderA reader(path);
  std::string content;
  char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    if (!content.empty())
      content += '\n';
    content += line;
  }
  return content;
}

static std::string
LoadContent()
{
  if (options.file != nullptr)
    return ReadTextFile(Path(options.file));

  if (quick_guide_news_markdown[0] == '\0') {
    std::fprintf(stderr,
                 "%s: no FILE and Quick Guide news is empty "
                 "(build NEWS.txt first)\n",
                 canonical_name);
    std::exit(EXIT_FAILURE);
  }

  return quick_guide_news_markdown;
}

[[gnu::pure]]
static long long
Ms(const std::chrono::steady_clock::duration d) noexcept
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
}

static void
RunBenchmark(TestMainWindow &main_window, const std::string &text)
{
  const DialogLook &look = *dialog_look;
  const unsigned width = options.layout_width > 0
    ? options.layout_width
    : 560U;

  using clock = std::chrono::steady_clock;

  const auto t0 = clock::now();
  if (!options.plain)
    (void)ParseMarkdown(text.c_str());
  const auto t1 = clock::now();

  AnyCanvas canvas;
  canvas.Select(look.text_font);
  const WrappedText wrapped = WrapText(canvas, width, text);
  const auto t2 = clock::now();

  WindowStyle style;
  style.Hide();
  RichTextWindow window;
  window.Create(main_window,
                PixelRect{0, 0, int(width), 400},
                style);
  window.SetFont(look.text_font, &look.bold_font,
                 &look.heading1_font, &look.heading2_font);
  window.SetDarkMode(look.dark_mode, look.background_color);
  window.SetDialogLook(look);
  window.SetText(text.c_str(), !options.plain);
  const auto t3 = clock::now();
  const unsigned height = window.GetContentHeight();
  const auto t4 = clock::now();

  std::printf("source bytes: %zu\n", text.size());
  std::printf("layout width: %u px\n", width);
  std::printf("wrapped lines: %zu\n", wrapped.lines.size());
  std::printf("content height: %u px\n", height);
  if (!options.plain)
    std::printf("parse markdown: %lld ms\n", Ms(t1 - t0));
  std::printf("wrap text: %lld ms\n", Ms(t2 - t1));
  std::printf("set text: %lld ms\n", Ms(t3 - t2));
  std::printf("line layout (GetContentHeight): %lld ms\n", Ms(t4 - t3));
  std::printf("total: %lld ms\n", Ms(t4 - t0));
}

static void
RunInteractive(TestMainWindow &main_window, const std::string &text)
{
  WidgetDialog dialog(WidgetDialog::Full{}, main_window, *dialog_look,
                      "RunRichTextRenderer");

  auto rich_text = std::make_unique<RichTextWidget>(
    *dialog_look, text.c_str(), !options.plain);
  auto scroll = std::make_unique<VScrollWidget>(
    std::move(rich_text), *dialog_look, true);

  dialog.FinishPreliminary(std::move(scroll));
  dialog.ShowModal();
}

static void
Main(TestMainWindow &main_window)
{
  const std::string text = LoadContent();
  if (text.empty()) {
    std::fprintf(stderr, "%s: input is empty\n", canonical_name);
    std::exit(EXIT_FAILURE);
  }

  if (options.benchmark)
    RunBenchmark(main_window, text);
  else
    RunInteractive(main_window, text);
}
