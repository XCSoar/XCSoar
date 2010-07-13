#ifndef AIRSPACE_WARNING_VISITOR_HPP
#define AIRSPACE_WARNING_VISITOR_HPP

class AirspaceWarning;

/**
 * Generic visitor for AirspaceWarning system
 */
class AirspaceWarningVisitor {
public:
  virtual void Visit(const AirspaceWarning &w) = 0;
};


#endif

