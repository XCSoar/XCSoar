// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/EUMETView/Enhance.hpp"
#include "ui/canvas/custom/UncompressedImage.hpp"
#include "TestUtil.hpp"

#include <chrono>
#include <memory>

using namespace std::chrono;

/**
 * A grey image whose pixels run 0, 1, 2 ... 255, 0, 1 ... so that
 * every brightness occurs equally often and the percentiles are known.
 */
static UncompressedImage
MakeRamp(unsigned width, unsigned height)
{
  auto data = std::make_unique<uint8_t[]>(size_t(width) * height);
  for (size_t i = 0; i < size_t(width) * height; ++i)
    data[i] = uint8_t(i % 256);

  return {UncompressedImage::Format::GRAY, width, width, height,
          std::move(data)};
}

int main()
{
  plan_tests(16);

  /* an empty histogram cannot name a window */
  EUMETView::ToneHistogram empty;
  ok1(empty.IsEmpty());
  ok1(empty.GetCount() == 0);
  ok1(!EUMETView::MakeToneWindow(empty).IsValid());

  /* on a flat ramp the percentiles land where arithmetic says */
  EUMETView::ToneHistogram ramp;
  ramp.Add(MakeRamp(256, 16));
  ok1(!ramp.IsEmpty());
  ok1(ramp.GetCount() == 256u * 16u);
  ok1(ramp.Percentile(0) == 0);
  ok1(ramp.Percentile(1000) == 255);
  ok1(ramp.Percentile(500) >= 127 && ramp.Percentile(500) <= 129);

  const auto window = EUMETView::MakeToneWindow(ramp);
  ok1(window.IsValid());
  ok1(window.low >= 3 && window.low <= 7);        /* 2 % of 256 */
  ok1(window.high >= 248 && window.high <= 252);  /* 98 % of 256 */

  /* a scene of one brightness must not be stretched: there is nothing
     to stretch, and the gain would only amplify noise */
  auto flat_data = std::make_unique<uint8_t[]>(64);
  for (unsigned i = 0; i < 64; ++i)
    flat_data[i] = 100;
  EUMETView::ToneHistogram flat;
  flat.Add({UncompressedImage::Format::GRAY, 8, 8, 8, std::move(flat_data)});
  ok1(!EUMETView::MakeToneWindow(flat).IsValid());

  /* the blend is decided by elapsed time, not by a frame count: after
     a long gap the carried-over window says nothing and is replaced,
     while frames in quick succession barely move it */
  const EUMETView::ToneWindow old_window{10, 200};
  const EUMETView::ToneWindow fresh{110, 250};

  const auto quick = EUMETView::BlendToneWindow(old_window, fresh, minutes{10});
  ok1(quick.low > old_window.low && quick.low < old_window.low + 30);

  const auto after_gap = EUMETView::BlendToneWindow(old_window, fresh, hours{5});
  ok1(after_gap.low > fresh.low - 5);

  /* and with nothing carried over, the fresh measurement stands */
  ok1(EUMETView::BlendToneWindow({0, 0}, fresh, minutes{10}) == fresh);

  ok1(EUMETView::ToneWindowDistance({10, 200}, {12, 205}) == 5);

  return exit_status();
}
