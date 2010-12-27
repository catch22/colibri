//============================================================================
// defs.h: Common definitions
//
// (c) Michael Walter, 2006
//============================================================================

#ifndef UTILS_CORE_DEFS_H
#define UTILS_CORE_DEFS_H
#define log __old_log
#include <cmath>
#undef log
#include <string>
#include <vector>
#include <boost/cstdint.hpp>
//----------------------------------------------------------------------------

// Interface:
typedef std::vector<boost::uint8_t> buffer;
void init_utils();
void shutdown_utils();
__declspec(noreturn) void throw_errorf(const char*, ...);
//----------------------------------------------------------------------------

#endif
