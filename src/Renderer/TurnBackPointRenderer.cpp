#include "TurnBackPointRenderer.hpp"
#include "Look/MapLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Math/Angle.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "Projection/WindowProjection.hpp"
#include "Geo/Math.hpp"
#include "Screen/Layout.hpp"
#include "Math/Screen.hpp"
#include "Computer/Settings.hpp" // Added for ComputerSettings
#include <cmath>
#include <limits>

// Small value to avoid division by zero and floating point issues
constexpr double TBP_EPSILON = 1e-6;

void
TurnBackPointRenderer::Draw(Canvas &canvas,
                            const WindowProjection &projection,
                            [[maybe_unused]] const PixelPoint pos,
                            const NMEAInfo &basic,
                            const DerivedInfo &calculated,
                            const ComputerSettings &settings) const noexcept
{
  // Check if the feature is enabled in settings
  if (!settings.task.turn_back_marker_enabled)
    return;

  // Check if we have a valid task and solution data structure
  const TaskStats &task_stats = calculated.task_stats;
  if (!task_stats.task_valid)
    return;

  const ElementStat &total = task_stats.total;
  const GlideResult &solution = total.solution_remaining;

  // Need a valid solution to proceed (even if below glide for the *old* TBP)
  if (!solution.IsOk())
      return;

  // Check if we have a valid track
  if (!basic.track_available)
    return;

  // Get the current task waypoint location
  const GeoPoint &waypoint_location = task_stats.current_leg.location_remaining;
  if (!waypoint_location.IsValid())
    return;

  // Calculate direct distance (C) and bearing to waypoint from current position
  const GeoVector direct_vector = basic.location.DistanceBearing(waypoint_location);
  if (!direct_vector.IsValid() || direct_vector.distance < TBP_EPSILON) // Avoid issues if already at WP
    return;
  const double C_direct_distance = direct_vector.distance;

  // Calculate angle (theta) between current track and direct path to waypoint
  const Angle track_to_waypoint_angle = (basic.track - direct_vector.bearing).AsDelta();
  const double cos_theta = track_to_waypoint_angle.cos();

  // --- NEW TBP CALCULATION ---

  // Estimate total altitude available above the target arrival point at the waypoint
  // This assumes solution targets waypoint altitude correctly.
  const double Alt_Total = solution.height_glide + solution.altitude_difference;

  // We need positive altitude relative to the waypoint to glide there
  if (Alt_Total <= TBP_EPSILON) {
      return; // Cannot reach waypoint even directly, let alone via TBP
  }

  // Calculate the effective glide ratio *required* for the direct leg
  // Use this as an estimate for the detour legs. Need valid height_glide.
  if (solution.height_glide <= TBP_EPSILON) {
      // Cannot reliably calculate required glide ratio
      return;
  }
  const double glide_ratio = C_direct_distance / solution.height_glide;
  if (glide_ratio <= TBP_EPSILON) {
      // Invalid glide ratio
      return;
  }

  // Calculate D_Total: Total horizontal distance achievable with Alt_Total
  const double D_Total = Alt_Total * glide_ratio;

  // Calculate the denominator for the solution of x
  const double denominator = 2.0 * (D_Total - C_direct_distance * cos_theta);

  // Check for degenerate case (denominator close to zero)
  if (std::abs(denominator) < TBP_EPSILON) {
      // Geometric configuration prevents a stable solution here
      return;
  }

  // Calculate distance_to_tbp (x)
  // x = (D_Total^2 - C^2) / (2 * (D_Total - C * cos(theta)))
  const double distance_to_tbp = (D_Total * D_Total - C_direct_distance * C_direct_distance) / denominator;

  // --- Validity Checks for the solution x ---

  // 1. TBP must be ahead of the aircraft.
  if (distance_to_tbp < 0.0) {
    // The calculated point is behind the current position.
    return;
  }

  // 2. The geometry requires D_Total - x >= 0 (from sqrt step)
  //    This means x <= D_Total. If x > D_Total, the geometry is impossible.
  if (distance_to_tbp > D_Total + TBP_EPSILON) { // Add epsilon for float tolerance
     // This implies the required distance from TBP to WP would be negative.
     return;
  }

  // --- TBP Calculation Successful ---

  // Find the GEOGRAPHIC LOCATION of the TBP
  GeoPoint tbp_location = FindLatitudeLongitude(basic.location,
                                                basic.track,
                                                distance_to_tbp);

  // Convert the geographic TBP location to screen coordinates
  auto tbp_screen = projection.GeoToScreenIfVisible(tbp_location);
  if (!tbp_screen) {
    // TBP is calculated but currently off-screen
    return;
  }

  // --- Draw the TBP Symbol ---
  BulkPixelPoint triangle[3];
  triangle[0].x = 0;                      triangle[0].y = -Layout::Scale(5);
  triangle[1].x = -Layout::Scale(4);      triangle[1].y = Layout::Scale(3);
  triangle[2].x = Layout::Scale(4);       triangle[2].y = Layout::Scale(3);

  PolygonRotateShift(std::span<BulkPixelPoint>(triangle, 3), *tbp_screen,
                     basic.track - projection.GetScreenAngle());

  canvas.Select(look.tbp_pen);
  canvas.Select(look.tbp_brush);
  canvas.DrawPolygon(triangle, 3);
}
