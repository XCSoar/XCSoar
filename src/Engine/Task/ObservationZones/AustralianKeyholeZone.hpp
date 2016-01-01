/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef AUSTRALIAN_KEYHOLE_ZONE_HPP
#define AUSTRALIAN_KEYHOLE_ZONE_HPP

#include "SectorZone.hpp"

/**
 * This class implements the so called 'Australian Sector'. This sector is
 * an AAT turn point type.
 *
 * This sector type consists of two user defined vectors from the origin at
 * the waypoint and bounded by the outer radius. Also included is a cylinder
 * having an inner radius. Plainly the value inner radius is less that than
 * the outer radius.
 */

class AustralianKeyholeZone final : public SectorZone
  {
private:
  fixed inner_radius;

protected:
  /**
   * Ctor.
   * @param shape One of ObservationZone::Shape.
   * @param can_start_through_top When used as a start zone a start can be
   *                              triggered by crossing the "top". (What is
   *                              the top) (Check callers to
   *                              ObservationZone::CanStartThroughTop()).
   * @param arc_boundary Does the boundary include the arc? I.e. is crossing
   *                     the arc scored?
   * @param loc The location at the center of the inner radius.
   * @param radiusOuter The location of the outer sector.
   * @param startRadial The first radial of the sector in a CW sense, north
                        zero.
   * @param endRadial The second radial of the sector in a CW sense, north
                      zero.
   * @param inner_radius The radius of the inner cylinder, centered on loc,
   *                     in meters.
   */
  AustralianKeyholeZone(Shape shape,
                        bool  can_start_through_top,
                        bool  arc_boundary,
                        const GeoPoint &loc,
                        const fixed radiusOuter = fixed(50000.0),
                        const Angle startRadial = Angle::Zero(),
                        const Angle endRadial = Angle::FullCircle(),
                        const fixed inner_radius = fixed(1000.0))
    : SectorZone(shape,
                 can_start_through_top,
                 arc_boundary,
                 loc,
                 radiusOuter),
      inner_radius(inner_radius)
    {
    this->SectorZone::SetStartRadial(startRadial);
    this->SectorZone::SetEndRadial(endRadial);
    }

  /**
   * Clone ctor.
   * @param other The reference observation zone.
   * @param reference The location the cloned obsevation zone.
   */
  AustralianKeyholeZone(const AustralianKeyholeZone &other,
                        const GeoPoint &reference)
  : SectorZone((const SectorZone &)other, reference),
    inner_radius(other.inner_radius)
      {
      }

public:
  /**
   * Create a fully populated AustralianKeyholeZone.
   * @param ref The zone's reference point - phi, lambda.
   * @param radius The radius of the sector encompassed by the radials.
   * @param inner_radius The radius of the cylinder whose center is reference.
   * @param start_radial The most CCW radial.
   * @param end_radial The most CW radial.
   */
  static AustralianKeyholeZone *New(const GeoPoint &ref,
                                    fixed radius,
                                    fixed inner_radius,
                                    Angle start_radial,
                                    Angle end_radial)
    {
    return new AustralianKeyholeZone(Shape::AUSTRALIAN_KEYHOLE,
                                     true,
                                     true,
                                     ref,
                                     radius,
                                     start_radial,
                                     end_radial,
                                     inner_radius);
    }

  /**
   * Create a proto AustralianKeyholeZone with some parameters set "sensibly".
   * @param ref The zone's reference point - phi, lambda.
   */
  static AustralianKeyholeZone *New(const GeoPoint &ref)
    {
    return new AustralianKeyholeZone(Shape::AUSTRALIAN_KEYHOLE,
                                     true,
                                     true,
                                     ref);
    }

  /**
   * Create a proto AustralianKeyholeZone with some parameters set "sensibly".
   * @param ref The zone's reference point - phi, lambda.
   * @param radius The radius of the sector portion of the zone.
   */
  static AustralianKeyholeZone *New(const GeoPoint &ref,
                                    fixed radius)
    {
    return new AustralianKeyholeZone(Shape::AUSTRALIAN_KEYHOLE,
                                     true,
                                     true,
                                     ref,
                                     radius);
    }

  /**
   * Returns the radius of the small cylinder [m].
   */
  fixed GetInnerRadius() const
    {
    return this->inner_radius;
    }

  void SetInnerRadius(fixed radius)
    {
    this->inner_radius = radius;
    }

  /* virtual methods from class ObservationZone */
  bool IsInSector(const GeoPoint &location) const override;
  OZBoundary GetBoundary() const override;

  /* virtual methods from class ObservationZonePoint */
  ObservationZonePoint *Clone(const GeoPoint &reference) const override
    {
    return new AustralianKeyholeZone(*this, reference);
    }

  };

#endif  // AUSTRALIAN_KEYHOLE_ZONE_HPP
