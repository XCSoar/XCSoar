#ifndef AIRSPACES_HPP
#define AIRSPACES_HPP

#include <kdtree++/kdtree.hpp>
#include "Airspace.hpp"
#include "Navigation/TaskProjection.hpp"
#include <deque>

class AirspaceVisitor;

class Airspaces
{
public:
  /** 
   * Constructor.
   * Note this class can't safely be copied (yet)
   * Note: altitude not used yet, this is a 2-D representation currently
   * 
   * @return empty Airspaces class.
   */
  Airspaces()
    {
    };

/** 
 * Destructor.
 * This also destroys Airspace objects contained in the tree or temporary buffer
 * 
 */
  ~Airspaces();

  /** 
   * Search for airspaces nearest to the aircraft.
   * Note: altitude not used yet
   * 
   * @param state state of aircraft, from which to search
   * 
   * @return single nearest airspace if external, or all airspaces enclosing the aircraft
   */
  const std::vector<Airspace> scan_nearest(const AIRCRAFT_STATE &state) const;

  /** 
   * Search for airspaces within range of the aircraft.
   * Note: altitude not used yet
   * 
   * @param state state of aircraft, from which to search
   * @param range distance in meters of search radius
   * 
   * @return vector of airspaces intersecting search radius
   */
  const std::vector<Airspace> scan_range(const AIRCRAFT_STATE &state, 
                                         const double range) const;

  /** 
   * Call visitor class on airspaces within range of location.
   * Note that the visitor is not instantiated separately for each match
   * Note: altitude not used yet
   * 
   * @param loc location of origin of search
   * @param range distance in meters of search radius
   * @param visitor visitor class to call on airspaces within range
   */
  void visit_within_range(const GEOPOINT &loc, 
                          const double range,
                          AirspaceVisitor& visitor) const;

  /** 
   * Call visitor class on airspaces intersected by vector.
   * Note that the visitor is not instantiated separately for each match
   * Note: altitude not used yet
   * 
   * @param loc location of origin of search
   * @param vec vector of line along with to search for intersections
   * @param visitor visitor class to call on airspaces intersected by line
   */
  void visit_intersecting(const GEOPOINT &loc, 
                          const GeoVector &vec,
                          AirspaceVisitor& visitor) const;

  /** 
   * Find airspaces the aircraft is inside.
   * Note: altitude not used yet
   * 
   * @param state state of aircraft for which to search
   * 
   * @return airspaces enclosing the aircraft
   */
  std::vector<Airspace> find_inside(const AIRCRAFT_STATE &state) const;

  /** 
   * Re-organise the internal airspace tree after inserting/deleting.
   * Should be called after inserting/deleting airspaces prior to performing
   * any searches, but can be done once after a batch insert/delete.
   */
  void optimise();

  /** 
   * Add airspace to the internal airspace tree.  
   * The airspace is not copied; ownership is transferred to this class.
   * 
   * @param asp New airspace to be added.
   */
  void insert(AbstractAirspace* asp);

/** 
 * Clear the airspace store
 * 
 */
  void clear();

private:

  typedef KDTree::KDTree<4, 
                         Airspace, 
                         FlatBoundingBox::kd_get_bounds,
                         FlatBoundingBox::kd_distance
                         > AirspaceTree;

  AirspaceTree airspace_tree;
  TaskProjection task_projection;

  std::deque< AbstractAirspace* > tmp_as;

  /** @link dependency */
  /*#  Airspace lnkAirspace; */
};

#endif
