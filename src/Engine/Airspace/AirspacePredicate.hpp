#ifndef AIRSPACE_PREDICATE_HPP
#define AIRSPACE_PREDICATE_HPP

#include "Compiler.h"

class AbstractAirspace;
class AirspacePredicateTrue;
struct AIRCRAFT_STATE;

/**
 *  Functor class for conditions to be applied to airspace queries
 */
class AirspacePredicate
{
public:
  /**
   * Test condition
   * @param t Airspace to test
   * @return True if condition met
   */
  gcc_pure
  virtual bool operator()(const AbstractAirspace& t) const = 0;

  /**
   * Test condition (calls operator() method)
   * @param t Airspace to test
   * @return True if condition met
   */
  gcc_pure
  bool condition(const AbstractAirspace&t) const {
    return (*this)(t);
  }

  /** Convenience condition, useful for default conditions */
  static AirspacePredicateTrue always_true;
};

/**
 * Convenience predicate for conditions always true
 */
class AirspacePredicateTrue: public AirspacePredicate
{
public:
  bool operator()(const AbstractAirspace& t) const {
    return true;
  }
};

/**
 * Convenience predicate for conditions always true
 */
class AirspacePredicateAircraftInside: public AirspacePredicate
{
public:
  /**
   * Constructor
   *
   * @param state State to check interior
   *
   * @return Initialised object
   */
  AirspacePredicateAircraftInside(const AIRCRAFT_STATE& state);

  bool operator()(const AbstractAirspace& t) const;

private:
  const AIRCRAFT_STATE& m_state;
};

/**
 * Convenience predicate for height within a specified range
 */
class AirspacePredicateHeightRange: public AirspacePredicate
{
public:
  /**
   * Constructor
   *
   * @param _h_min Lower bound on airspace (m)
   * @param _h_max Upper bound on airspace (m)
   *
   * @return Initialised object
   */
  AirspacePredicateHeightRange(const short _h_min, const short _h_max):
    h_min(_h_min), h_max(_h_max) {};

  bool operator()( const AbstractAirspace& t ) const;
private:
  const short h_min;
  const short h_max;
};

#endif
