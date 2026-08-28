// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "Math/Point2D.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

struct BrokenDateTime;
struct GeoPoint;
class GeoBounds;
class CurlGlobal;
class ProgressListener;
namespace Co { template<typename T> class Task; }

/**
 * The EUMETNET OPERA radar composite, a pan-European reflectivity
 * mosaic published every five minutes under CC BY 4.0.
 *
 * @see https://eumetnet.github.io/openradardata-documentation/
 */
namespace OPERA {

/** New composites appear on this grid, in minutes past the hour. */
static constexpr unsigned CADENCE_MINUTES = 5;

/**
 * How far behind the wall clock to look for the newest composite.  A
 * frame is not on the server the instant it is nominally acquired.
 */
static constexpr unsigned LATENCY_MINUTES = 10;

/**
 * The largest image we render.  The composite is a 1km grid; more
 * pixels than this only upsamples.
 */
static constexpr unsigned MAX_IMAGE_SIZE = 1024;

/**
 * Build the URL of the composite covering the given UTC time.  The
 * time is rounded down to #CADENCE_MINUTES.
 *
 * @return an empty string if the time is not plausible
 *
 * Exposed for the unit test.
 */
[[gnu::pure]]
std::string MakeCompositeURL(const BrokenDateTime &utc);

/**
 * Project a geographic location onto the composite's grid, which is
 * Lambert azimuthal equal-area centred on 55N 10E.  The result is in
 * metres in the projection plane, x east and y north.
 *
 * Exposed for the unit test.
 */
[[gnu::pure]]
DoublePoint2D Project(const GeoPoint &p) noexcept;

/**
 * Map a reflectivity to one of the colour classes.  A value that is
 * not a number means the radar looked and saw nothing.
 *
 * The class boundaries were measured against the German weather
 * service's own European product rather than derived from a Z-R
 * relation: the offset between this composite, which is a maximum
 * over all elevations, and a near-ground product is not constant but
 * shrinks as the echo grows stronger, so no single exponent fits.
 *
 * Exposed for the unit test.
 *
 * @return the class index, or -1 for "no precipitation"
 */
[[gnu::const]]
int ClassifyReflectivity(double dbz) noexcept;

/**
 * Is this "not a number"?
 *
 * The composite marks "the radar looked and saw nothing" that way,
 * and we are built with -ffast-math, under which the compiler may
 * assume no such value ever occurs; so this looks at the bits rather
 * than comparing.
 *
 * Exposed for the unit test.
 */
[[gnu::const]]
bool IsNotANumber(double value) noexcept;

/** The number of colour classes #ClassifyReflectivity() may return. */
static constexpr unsigned N_CLASSES = 15;

/**
 * The colour of one class, as 0xRRGGBB.  These are the classes the
 * Deutscher Wetterdienst uses for its radar images, so the result
 * looks like what German pilots are used to reading.
 */
[[gnu::const]]
uint32_t GetClassColour(unsigned i) noexcept;

/**
 * Download the newest composite and render the part covering
 * @p bounds as a PNG.
 *
 * Throws on error.
 *
 * @return the path of the rendered image
 */
Co::Task<AllocatedPath>
DownloadRadar(const GeoBounds &bounds, unsigned width, unsigned height,
              CurlGlobal &curl, ProgressListener &progress);

} // namespace OPERA
