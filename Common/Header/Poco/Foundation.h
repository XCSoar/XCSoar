
#ifndef FOUNDATION_H
#define FOUNDATION_H

// JMW hack to get it compiling


// assume it's windows family for now
// later replace with check for XCSoar's defines
#define POCO_OS_FAMILY_WINDOWS   

// don't import/export dll
#define Foundation_API  

#include <assert.h>
#define poco_assert_dbg(x) assert(x)

#endif
