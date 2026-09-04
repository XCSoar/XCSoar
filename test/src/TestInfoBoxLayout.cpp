// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/InfoBoxLayout.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "InfoBoxes/Border.hpp"
#include "TestUtil.hpp"

#include <initializer_list>
#include <utility>

using Geometry = InfoBoxSettings::Geometry;
using namespace InfoBoxFactory;

/**
 * Calculate the given geometry and apply the given InfoBox types to
 * it, the way #InfoBoxManager does.
 */
static InfoBoxLayout::Layout
Apply(PixelRect rc, Geometry geometry,
      std::initializer_list<std::pair<unsigned, Type>> contents) noexcept
{
  InfoBoxSettings::Panel panel;
  panel.Clear();

  for (const auto &i : contents)
    panel.contents[i.first] = i.second;

  auto layout = InfoBoxLayout::Calculate(rc, geometry);
  InfoBoxLayout::ApplyContents(layout, panel);
  return layout;
}

static void
TestReleaseSpace()
{
  /* portrait: a line is a row of four, and the released slot hands a
     third of its width to each of the others */
  {
    const auto l = Apply({0, 0, 480, 800}, Geometry::SPLIT_3X4,
                         {{1, e_ReleaseSpace}});

    ok1(!l.visible[1]);
    ok1(l.visible[0]);
    ok1(l.visible[2]);
    ok1(l.visible[3]);

    ok1(l.positions[0].left == 0);
    ok1(l.positions[0].right == 160);
    ok1(l.positions[2].left == 160);
    ok1(l.positions[2].right == 320);
    ok1(l.positions[3].left == 320);
    ok1(l.positions[3].right == 480);

    /* the other lines are untouched */
    ok1(l.positions[4].right == 120);
    ok1(l.positions[8].right == 120);
  }

  /* landscape: a line is a column */
  {
    const auto l = Apply({0, 0, 800, 480}, Geometry::SPLIT_3X4,
                         {{1, e_ReleaseSpace}});

    ok1(!l.visible[1]);
    ok1(l.positions[0].top == 0);
    ok1(l.positions[0].bottom == 160);
    ok1(l.positions[2].top == 160);
    ok1(l.positions[3].bottom == 480);
  }

  /* the InfoBox which grows over the last slot of a line takes over
     its outer border */
  {
    const auto base = InfoBoxLayout::Calculate({0, 0, 480, 800},
                                               Geometry::SPLIT_3X4);
    ok1((base.borders[2] & BORDERRIGHT) != 0);
    ok1((base.borders[3] & BORDERRIGHT) == 0);

    const auto l = Apply({0, 0, 480, 800}, Geometry::SPLIT_3X4,
                         {{3, e_ReleaseSpace}});
    ok1(l.positions[2].right == 480);
    ok1((l.borders[2] & BORDERRIGHT) == 0);
  }

  /* a line which consists of placeholders only cannot collapse */
  {
    const auto l = Apply({0, 0, 480, 800}, Geometry::SPLIT_3X4,
                         {{0, e_ReleaseSpace}, {1, e_ReleaseSpace},
                          {2, e_ReleaseSpace}, {3, e_ReleaseSpace}});

    ok1(l.visible[0]);
    ok1(l.visible[3]);
    ok1(l.positions[0].right == 120);
  }
}

static void
TestMergeAlongLine()
{
  /* the preceding InfoBox grows over the slot and gets twice the
     space */
  {
    const auto l = Apply({0, 0, 480, 800}, Geometry::SPLIT_3X4,
                         {{2, e_MergeAlongLine}});

    ok1(!l.visible[2]);
    ok1(l.positions[1].left == 120);
    ok1(l.positions[1].right == 360);
    ok1(l.positions[3].left == 360);
    ok1(l.positions[3].right == 480);
  }

  /* two of them widen it further, and it takes over the outer border
     of the line */
  {
    const auto l = Apply({0, 0, 480, 800}, Geometry::SPLIT_3X4,
                         {{2, e_MergeAlongLine}, {3, e_MergeAlongLine}});

    ok1(!l.visible[2]);
    ok1(!l.visible[3]);
    ok1(l.positions[1].right == 480);
    ok1((l.borders[1] & BORDERRIGHT) == 0);
  }

  /* in the first slot of a line there is nothing to merge into, so it
     releases its space instead */
  {
    const auto l = Apply({0, 0, 480, 800}, Geometry::SPLIT_3X4,
                         {{0, e_MergeAlongLine}});

    ok1(!l.visible[0]);
    ok1(l.positions[1].left == 0);
    ok1(l.positions[1].right == 160);
  }

  /* landscape: the preceding InfoBox is the one above */
  {
    const auto l = Apply({0, 0, 800, 480}, Geometry::SPLIT_3X4,
                         {{2, e_MergeAlongLine}});

    ok1(!l.visible[2]);
    ok1(l.positions[1].top == 120);
    ok1(l.positions[1].bottom == 360);
  }

  /* merging keeps the edges of the geometry: the line still lines up
     with its neighbour even when the width is not a multiple of the
     number of columns */
  {
    const auto l = Apply({0, 0, 922, 1990}, Geometry::SPLIT_3X4,
                         {{3, e_MergeAlongLine}});

    ok1(l.positions[0].right == l.positions[4].right);
    ok1(l.positions[1].right == l.positions[5].right);
    ok1(l.positions[2].left == l.positions[6].left);
    ok1(l.positions[2].right == 922);
  }
}

int main()
{
  plan_tests(24 + 19);

  TestReleaseSpace();
  TestMergeAlongLine();

  return exit_status();
}
