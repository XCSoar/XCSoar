// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DisplayBrightness.hpp"

#include "system/FileUtil.hpp"
#include "util/NumberParser.hxx"

#include <algorithm> // for std::clamp()
#include <cctype>
#include <optional>
#include <string_view>
#include <utility>

#ifdef HAVE_POSIX
#include <unistd.h>
#include <fmt/format.h>
#endif

#if defined(__linux__) && !defined(ANDROID)
static std::optional<unsigned>
ReadUnsigned(Path path) noexcept
{
  char buffer[32];
  if (!File::ReadString(path, buffer, sizeof(buffer)))
    return std::nullopt;

  std::string_view value(buffer);
  while (!value.empty() && std::isspace((unsigned char)value.front()))
    value.remove_prefix(1);
  while (!value.empty() && std::isspace((unsigned char)value.back()))
    value.remove_suffix(1);

  if (value.empty())
    return std::nullopt;

  return ParseInteger<unsigned>(value);
}
#endif

#if defined(__linux__) && !defined(ANDROID) && !defined(KOBO)
DisplayBrightness::DisplayBrightness(AllocatedPath &&_brightness_path,
                                     unsigned _max_brightness,
                                     bool _writable) noexcept
  :brightness_path(std::move(_brightness_path)),
   max_brightness(_max_brightness),
   writable(_writable) {}
#endif

std::unique_ptr<DisplayBrightness>
DisplayBrightness::Detect() noexcept
{
#if defined(__linux__) && !defined(ANDROID) && !defined(KOBO)
  struct Visitor final : Directory::DirEntryVisitor {
    unsigned best_max_brightness = 0;
    bool best_writable = false;
    std::unique_ptr<DisplayBrightness> result;

    void Visit(Path full, [[maybe_unused]] Path filename,
               bool is_dir) noexcept override {
      if (!is_dir)
        return;

      const auto max_path = AllocatedPath::Build(full, "max_brightness");
      auto brightness_path = AllocatedPath::Build(full, "brightness");

      const auto max_brightness = ReadUnsigned(max_path);
      const auto brightness = ReadUnsigned(brightness_path);
      if (!max_brightness || !brightness || *max_brightness <= 1 ||
          *brightness > *max_brightness)
        return;

      const bool writable = access(brightness_path.c_str(), W_OK) == 0;
      if (result != nullptr &&
          (writable < best_writable ||
           (writable == best_writable &&
            *max_brightness <= best_max_brightness)))
        return;

      best_max_brightness = *max_brightness;
      best_writable = writable;
      result = std::unique_ptr<DisplayBrightness>(
        new DisplayBrightness(std::move(brightness_path), *max_brightness,
                              writable));
    }
  } visitor;

  Directory::VisitDirectoriesAndFiles(Path("/sys/class/backlight"), visitor,
                                      false);
  return std::move(visitor.result);
#else
  return nullptr;
#endif
}

unsigned
DisplayBrightness::GetBrightnessPercent() const noexcept
{
#if defined(__linux__) && !defined(ANDROID)
  if (brightness_path == nullptr || max_brightness == 0)
    return 0;
  const auto brightness = ReadUnsigned(brightness_path);
  if (!brightness)
    return 0;

  return std::min((*brightness * 100u) / max_brightness, 100u);
#else
  return 0u;
#endif
}

void
DisplayBrightness::SetBrightnessPercent(unsigned percent) const noexcept
{
#ifdef HAVE_POSIX
  if (!writable || max_brightness == 0 || brightness_path == nullptr)
    return;

  if (!ReadUnsigned(brightness_path))
    return;

  const unsigned clamped = std::clamp(percent, 0u, 100u);
  const auto raw = clamped * max_brightness / 100u;
  File::WriteExisting(brightness_path, fmt::format_int(raw).c_str());
#else
  (void)percent;
#endif
}
