#include "ASTPoint.hpp"

bool
ASTPoint::equals(const OrderedTaskPoint* other) const
{
  if (dynamic_cast<const ASTPoint*>(other)) {
    return OrderedTaskPoint::equals(other);
  } else {
    return false;
  }
}
