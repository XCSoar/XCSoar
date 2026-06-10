// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Language/Language.hpp"
#include "util/StaticString.hxx"

/**
 * Format "Feature is not available in this build." for a translatable
 * feature name (N_() marker).
 */
template<std::size_t N>
void
FormatFeatureNotAvailableInThisBuild(StaticString<N> &dest,
                                     const char *feature) noexcept
{
  dest.Format(_("%s is not available in this build."), _(feature));
}

/**
 * Format "... because the OpenGL renderer is not available." for a
 * translatable feature name (N_() marker).
 */
template<std::size_t N>
void
FormatFeatureNotAvailableInThisBuildWithoutOpenGLRenderer(
  StaticString<N> &dest, const char *feature) noexcept
{
  dest.Format(_("%s is not available in this build because the OpenGL "
               "renderer is not available."),
              _(feature));
}

/**
 * Format "Target device not found." / "Source device not found." for a
 * translatable role name (N_() marker, e.g. N_("Target")).
 */
template<std::size_t N>
void
FormatDeviceNotFound(StaticString<N> &dest,
                     const char *role) noexcept
{
  dest.Format(_("%s device not found."), _(role));
}

/**
 * Format "Do you want to remove station CODE?"
 */
template<std::size_t N>
void
FormatRemoveStationPrompt(StaticString<N> &dest,
                          const char *station) noexcept
{
  dest.Format(_("Do you want to remove station %s?"), station);
}

/**
 * Format "Starts in 2h" / "Starts in 3d" for a pre-formatted relative time.
 */
template<std::size_t N>
void
FormatStartsIn(StaticString<N> &dest,
               const char *time) noexcept
{
  dest.Format(_("Starts in %s"), time);
}

/**
 * Format "5 minutes ago".
 */
template<std::size_t N>
void
FormatMinutesAgo(StaticString<N> &dest,
                 unsigned minutes) noexcept
{
  dest.UnsafeFormat(_("%u minutes ago"), minutes);
}

/**
 * Generic failure message for WiFi and similar operations.
 */
[[nodiscard]] [[gnu::pure]]
inline const char *
OperationFailedText() noexcept
{
  return _("The operation failed.");
}

/**
 * Format RASP dwcrit/hwcrit help text (height qualifier differs).
 */
template<std::size_t N>
void
FormatRaspHeightCritHelp(StaticString<N> &dest,
                         bool above_ground) noexcept
{
  dest.Format(_("This parameter estimates the height%s at which the "
                "average dry updraft strength drops below 225 fpm and is "
                "expected to give better quantitative numbers for the "
                "maximum cloudless thermalling height than the BL Top "
                "height, especially when mixing results from vertical "
                "wind shear rather than thermals. (Note: the present "
                "assumptions tend to underpredict the max. thermalling "
                "height for dry conditions.) In the presence of clouds "
                "the maximum thermalling height may instead be limited by "
                "the cloud base. Being for \"dry\" thermals, this "
                "parameter omits the effect of \"cloudsuck\"."),
              above_ground ? _(N_(" above ground")) : "");
}
