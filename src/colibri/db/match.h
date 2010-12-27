//============================================================================
// match.h: String match algorithm
//
// (c) Michael Walter, 2005-2007
//============================================================================

#ifndef COLIBRI_DB_MATCH_H
#define COLIBRI_DB_MATCH_H
#include "../core/defs.h"
#include <limits>
//----------------------------------------------------------------------------

// Interface:
float score_not_found();
std::wstring normalized_term(const std::wstring&);
bool match(const wchar_t *term, const wchar_t *string, float &score, std::wstring &marked_up_string);
//----------------------------------------------------------------------------

#include "match.inl"
#endif
