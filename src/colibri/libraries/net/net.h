//============================================================================
// net.h: Networking helpers
//
// (c) Michael Walter, 2006
//============================================================================

#ifndef UTILS_NET_NET_H
#define UTILS_NET_NET_H
#include "../core/defs.h"
//----------------------------------------------------------------------------

// Interface:
bool download(const std::wstring &url, buffer &content, const std::wstring &user_agent);
//----------------------------------------------------------------------------

#endif