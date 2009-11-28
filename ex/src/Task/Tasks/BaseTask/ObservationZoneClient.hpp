#ifndef OBSERVATIONZONECLIENT_HPP
#define OBSERVATIONZONECLIENT_HPP

#include "ObservationZonePoint.hpp"

class ObservationZoneClient: public virtual ObservationZone
{
public:
  ObservationZoneClient(ObservationZonePoint* _oz):m_oz(_oz) {};

  virtual ~ObservationZoneClient() {
    delete m_oz;
  };

/** 
 * Accessor for OZ (for modifying parameters etc)
 *
 * @return Observation zone
 */
  ObservationZonePoint* get_oz() const {
    return m_oz;
  }

/** 
 * Test whether aircraft is inside observation zone.
 * 
 * @param ref Aircraft state to test
 * 
 * @return True if aircraft is inside observation zone
 */
  virtual bool isInSector(const AIRCRAFT_STATE &ref) const
  {
    return m_oz->isInSector(ref);
  }

/** 
 * Generate a random location inside the OZ (to be used for testing)
 * 
 * @param mag proportional magnitude of error from center (0-1)
 *
 * @return Location of point
 */
  GEOPOINT randomPointInSector(const double mag) const {
    return m_oz->randomPointInSector(mag);
  }

/** 
 * Calculate distance reduction for achieved task point,
 * to calcuate scored distance.
 * 
 * @return Distance reduction once achieved
 */
  virtual double score_adjustment() const {
    return m_oz->score_adjustment();
  }

protected:

/** 
 * Calculate boundary point from parametric border
 * 
 * @param t t value (0,1) of parameter
 * 
 * @return Boundary point
 */
  GEOPOINT get_boundary_parametric(double t) const
  {
    return m_oz->get_boundary_parametric(t);
  }

/** 
 * Check transition constraints 
 * 
 * @param ref_now Current aircraft state
 * @param ref_last Previous aircraft state
 * 
 * @return True if constraints are satisfied
 */
  virtual bool transition_constraint(const AIRCRAFT_STATE & ref_now, 
                                     const AIRCRAFT_STATE & ref_last) {
    return m_oz->transition_constraint(ref_now, ref_last);
  }

  virtual void set_legs(const TaskPoint *previous,
                        const TaskPoint *current,
                        const TaskPoint *next);

private:
  ObservationZonePoint* m_oz;

};


#endif
