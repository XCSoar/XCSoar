#ifndef SCANTASKPOINT_HPP
#define SCANTASKPOINT_HPP

/**
 * A reference to a trace/search point: first element is the stage
 * number (turn point number); second element is the index in the
 * #TracePointVector / #SearchPointVector.
 */
typedef std::pair<unsigned, unsigned> ScanTaskPoint;

#endif
