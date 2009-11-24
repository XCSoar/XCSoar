#ifndef AIRSPACE_HPP
#define AIRSPACE_HPP

#include "Navigation/Flat/FlatBoundingBox.hpp"
#include "AbstractAirspace.hpp"
#include "Util/GenericVisitor.hpp"

/**
 * Single object container for actual airspaces, to be stored in Airspaces object
 * This class manages the bounding box of the actual airspace.
 *
 * This follows envelope-letter
 * idiom, in which the AbstractAirspace is the letter and this class
 * Airspace is an envelope, containing bounding box information for
 * use with high performance search structures.
 * 
 */
class Airspace: 
  public FlatBoundingBox,
  public BaseVisitable<>
{
public:

  /** 
   * Constructor for actual airspaces.  
   *
   * @param airspace actual concrete airspace to create an envelope for
   * @param tp task projection to be used for flat-earth representation
   * 
   * @return airspace letter inside envelope suitable for insertion in a search structure
   */
  Airspace(AbstractAirspace& airspace,
           const TaskProjection& tp);

  /** 
   * Constructor for virtual airspaces for use in range-based
   * intersection queries
   * 
   * @param loc Location about which to create a virtual airspace envelope
   * @param task_projection projection to be used for flat-earth representation
   * @param range range in meters of virtual bounding box
   * 
   * @return dummy airspace envelope
   */
  Airspace(const GEOPOINT&loc, const TaskProjection& task_projection, const
    double range=0.0):
    FlatBoundingBox(task_projection.project(loc),
                    task_projection.project_range(loc,range)),
    pimpl_airspace(NULL)
  {
  };

  /** 
   * Constructor for virtual airspaces for use in bounding-box
   * specified intersection queries
   * 
   * @param ll Lower left corner of bounding box
   * @param ur Upper right corner of bounding box
   * @param task_projection projection to be used for flat-earth representation
   * 
   * @return dummy airspace envelope
   */
  Airspace(const GEOPOINT &ll, 
           const GEOPOINT &ur,
           const TaskProjection& task_projection):
    FlatBoundingBox(task_projection.project(ll),
                    task_projection.project(ur)), 
    pimpl_airspace(NULL)
  {
  };

  /** 
   * Checks whether an aircraft is inside the airspace.  First
   * performs bounding box search, then if successful refines the
   * search by directing the query to the enclosed concrete airspace.
   * 
   * @param loc Location to check for enclosure
   * 
   * @return true if aircraft is inside airspace
   */
  bool inside(const AIRCRAFT_STATE &loc) const;

  /** 
   * Checks whether a flat-earth ray intersects with the airspace
   * bounding box.
   * 
   * @param ray Flat-earth ray to check for intersection
   * 
   * @return true if ray intersects or wholly enclosed by airspace
   */
  bool intersects(const FlatRay& ray) const;

  /** 
   * Checks whether a line intersects with the airspace, by directing
   * the query to the enclosed concrete airspace.
   * 
   * @param g1 Location of origin of search vector
   * @param vec Line from origin
   * @param tp Projection used by flat-earth representation
   * 
   * @return true if the line intersects the airspace
   */
  bool intersects(const GEOPOINT& g1, const GeoVector &vec, 
                  const TaskProjection&tp) const;

  /** 
   * Destroys concrete airspace enclosed by this instance if present.
   * Note that this should not be called by clients but only by the
   * master store.  Many copies of this airspace may point to the same
   * concrete airspace so have to be careful here.
   * 
   */
  void destroy();

private:

  /**
   * @supplierCardinality 0..1 
   */
  AbstractAirspace *pimpl_airspace;

public:
#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& f, 
                                   const Airspace& ts);
#endif
  
  /** 
   * Accepts a visitor and directs it to the contained concrete airspace.
   * 
   * @param v Visitor to accept
   */
  void Accept(BaseVisitor &v) const;
};

#endif
