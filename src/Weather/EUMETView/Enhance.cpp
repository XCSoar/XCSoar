// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Enhance.hpp"
#include "ui/canvas/custom/UncompressedImage.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

namespace {

/** how quickly the stretch follows a change in the scene */
constexpr auto TIME_CONSTANT = std::chrono::minutes{60};

/** the shape of the stretch; below one lifts the dark end */
constexpr double GAMMA = 0.85;

/**
 * The unsharp mask, in pixels rather than metres, so the sharpening
 * stays at the scale the eye sees whatever zoom the tile came from.
 */
constexpr double SHARPEN_SIGMA = 1.5;
constexpr double SHARPEN_AMOUNT = 0.7;

[[gnu::const]]
unsigned
Luminance(unsigned r, unsigned g, unsigned b) noexcept
{
  /* the usual Rec. 601 weights; for the single channel products this
     is a no-op, because libpng widens grey to equal RGB */
  return (r * 77 + g * 151 + b * 28) >> 8;
}

/** bytes per pixel, or zero for a format we cannot handle */
[[gnu::const]]
unsigned
BytesPerPixel(UncompressedImage::Format format) noexcept
{
  switch (format) {
  case UncompressedImage::Format::GRAY:
    return 1;

  case UncompressedImage::Format::RGB:
    return 3;

  case UncompressedImage::Format::RGBA:
    return 4;

  case UncompressedImage::Format::INVALID:
    break;
  }

  return 0;
}

struct Kernel {
  std::array<float, 16> tap{};
  int radius = 0;
};

[[gnu::const]]
Kernel
MakeKernel(double sigma) noexcept
{
  Kernel k;
  k.radius = std::min(int(std::ceil(3 * sigma)), int(k.tap.size() / 2));

  float sum = 0;
  for (int i = -k.radius; i <= k.radius; ++i) {
    const float v = float(std::exp(-double(i) * i / (2 * sigma * sigma)));
    k.tap[std::size_t(i + k.radius)] = v;
    sum += v;
  }

  for (int i = 0; i <= 2 * k.radius; ++i)
    k.tap[std::size_t(i)] /= sum;

  return k;
}

} // anonymous namespace

void
EUMETView::ToneHistogram::Add(const UncompressedImage &image) noexcept
{
  const unsigned bpp = BytesPerPixel(image.GetFormat());
  if (bpp == 0)
    return;

  const auto *const data = (const uint8_t *)image.GetData();
  if (data == nullptr)
    return;

  const std::size_t pitch = image.GetPitch();

  for (unsigned y = 0; y < image.GetHeight(); ++y) {
    const uint8_t *row = data + std::size_t(y) * pitch;

    for (unsigned x = 0; x < image.GetWidth(); ++x, row += bpp) {
      /* a fully transparent pixel is outside the disc or missing, and
         counting it would drag the dark end down */
      if (bpp == 4 && row[3] < 0x80)
        continue;

      ++bins[bpp == 1 ? row[0] : Luminance(row[0], row[1], row[2])];
      ++total;
    }
  }
}

uint8_t
EUMETView::ToneHistogram::Percentile(unsigned permille) const noexcept
{
  if (total == 0)
    return 0;

  const uint64_t want = total * permille / 1000;
  uint64_t seen = 0;

  for (unsigned i = 0; i < bins.size(); ++i) {
    seen += bins[i];
    if (seen > want)
      return uint8_t(i);
  }

  return 255;
}

EUMETView::ToneWindow
EUMETView::MakeToneWindow(const ToneHistogram &histogram) noexcept
{
  if (histogram.IsEmpty())
    return {0, 0};

  ToneWindow w{histogram.Percentile(20), histogram.Percentile(980)};

  /* a scene of one brightness -- solid cloud, or night -- gives a
     window with no width; stretching it would turn noise into
     violent contrast, so leave the image alone instead */
  if (w.high <= w.low + 8)
    return {0, 0};

  return w;
}

EUMETView::ToneWindow
EUMETView::BlendToneWindow(ToneWindow previous, ToneWindow fresh,
                           std::chrono::steady_clock::duration elapsed) noexcept
{
  if (!previous.IsValid())
    return fresh;
  if (!fresh.IsValid())
    return previous;

  const double dt = std::chrono::duration<double>(elapsed).count();
  const double tau = std::chrono::duration<double>(TIME_CONSTANT).count();
  if (dt <= 0)
    return previous;

  const double alpha = 1.0 - std::exp(-dt / tau);

  const auto mix = [alpha](uint8_t old_value, uint8_t new_value) {
    return uint8_t(std::lround(old_value + alpha * (new_value - old_value)));
  };

  return {mix(previous.low, fresh.low), mix(previous.high, fresh.high)};
}

unsigned
EUMETView::ToneWindowDistance(ToneWindow a, ToneWindow b) noexcept
{
  return std::max(unsigned(std::abs(int(a.low) - int(b.low))),
                  unsigned(std::abs(int(a.high) - int(b.high))));
}

UncompressedImage
EUMETView::Enhance(const UncompressedImage &image, ToneWindow window) noexcept
{
  const unsigned bpp = BytesPerPixel(image.GetFormat());
  if (bpp == 0 || !window.IsValid())
    return {};

  const auto *const src = (const uint8_t *)image.GetData();
  if (src == nullptr)
    return {};

  const unsigned width = image.GetWidth(), height = image.GetHeight();
  const std::size_t pitch = image.GetPitch();
  const std::size_t n = std::size_t(width) * height;
  if (n == 0)
    return {};

  /* the stretch and the gamma both depend only on the input value, so
     they collapse into one table of 256 entries */
  std::array<uint8_t, 256> curve;
  const double span = double(window.high) - window.low;
  for (unsigned i = 0; i < curve.size(); ++i) {
    const double t = std::clamp((double(i) - window.low) / span, 0.0, 1.0);
    curve[i] = uint8_t(std::lround(std::pow(t, GAMMA) * 255.0));
  }

  /* work on luminance: one plane to blur instead of three, and the
     colour products keep their hue because the gain is applied to all
     channels alike */
  auto plane = std::make_unique<uint8_t[]>(n);
  for (unsigned y = 0; y < height; ++y) {
    const uint8_t *row = src + std::size_t(y) * pitch;
    uint8_t *out = plane.get() + std::size_t(y) * width;

    for (unsigned x = 0; x < width; ++x, row += bpp)
      out[x] = curve[bpp == 1 ? row[0] : Luminance(row[0], row[1], row[2])];
  }

  /* separable Gaussian, two passes over the plane */
  const auto kernel = MakeKernel(SHARPEN_SIGMA);
  auto tmp = std::make_unique<float[]>(n);
  auto blur = std::make_unique<float[]>(n);

  for (unsigned y = 0; y < height; ++y) {
    const uint8_t *row = plane.get() + std::size_t(y) * width;
    float *out = tmp.get() + std::size_t(y) * width;

    for (unsigned x = 0; x < width; ++x) {
      float a = 0;
      for (int i = -kernel.radius; i <= kernel.radius; ++i) {
        const int xx = std::clamp(int(x) + i, 0, int(width) - 1);
        a += kernel.tap[std::size_t(i + kernel.radius)] * row[xx];
      }
      out[x] = a;
    }
  }

  for (unsigned y = 0; y < height; ++y) {
    float *out = blur.get() + std::size_t(y) * width;

    for (unsigned x = 0; x < width; ++x) {
      float a = 0;
      for (int i = -kernel.radius; i <= kernel.radius; ++i) {
        const int yy = std::clamp(int(y) + i, 0, int(height) - 1);
        a += kernel.tap[std::size_t(i + kernel.radius)] *
          tmp[std::size_t(yy) * width + x];
      }
      out[x] = a;
    }
  }

  auto dest = std::make_unique<uint8_t[]>(n * bpp);

  for (unsigned y = 0; y < height; ++y) {
    const uint8_t *row = src + std::size_t(y) * pitch;
    uint8_t *out = dest.get() + std::size_t(y) * width * bpp;

    for (unsigned x = 0; x < width; ++x, row += bpp, out += bpp) {
      const std::size_t i = std::size_t(y) * width + x;
      const float sharp = plane[i] + SHARPEN_AMOUNT * (plane[i] - blur[i]);
      const unsigned value = unsigned(std::clamp(sharp, 0.0f, 255.0f));

      if (bpp == 1) {
        out[0] = uint8_t(value);
        continue;
      }

      /* scale the three channels by the same factor, so a colour
         composite keeps its hue and only its brightness changes */
      const unsigned before = Luminance(row[0], row[1], row[2]);
      for (unsigned c = 0; c < 3; ++c)
        out[c] = before == 0
          ? uint8_t(value)
          : uint8_t(std::min(255u, row[c] * value / before));

      if (bpp == 4)
        out[3] = row[3];
    }
  }

  return {image.GetFormat(), std::size_t(width) * bpp, width, height,
          std::move(dest)};
}
