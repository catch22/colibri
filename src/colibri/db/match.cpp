//============================================================================
// match.cpp: String match algorithm
//
// (c) Michael Walter, 2005-2007
//============================================================================

#include "match.h"
#include <cfloat>
using namespace std;
//----------------------------------------------------------------------------


//============================================================================
// anonymous namespace
//============================================================================
namespace
{
  //==========================================================================
  // match_impl()
  // 
  // XXX: improve match algorithm to consider "completeness" of match (FIREfoo
  //      wins over FIREfoobar)
  // XXX: improve match algorithm to consider how early in a word the match
  //      occurs (FIREfoo wins over fooFIRE)
  //==========================================================================
  bool match_impl(const wchar_t *term, const wchar_t *string, float current_score, const wstring &current_match, const wchar_t *to_add, unsigned current_chunk_count, unsigned current_word_index, bool last_was_match, float &best_score, wstring &best_match)
  {
    // record match iff entire search term was consumed
    if(!*term)
    {
      // best match?
      if(current_score>best_score)
      {
        best_match=current_match;
        best_match+=string;
        best_score=current_score;
      }
      return true;
    }

    // continue back-tracking when encountered a dead end
    if(!*string)
      return false;

    // start new word after white space sequence
    if(iswspace(*string))
    {
      while(iswspace(string[1]))
        ++string;
      ++current_word_index;
    }

    // start match at next character
    bool found_match=match_impl(term, string+1, current_score-0.01f, current_match, to_add, current_chunk_count, current_word_index, false, best_score, best_match);

    // start/continue a match if search term and string remainder share the initial character
    if(*term==towupper(*string))
    {
      // continue match if possible
      wstring cm=current_match;
      if(last_was_match)
      {
        // append match char
        cm+='&';
        cm+=*string;
      }
      else
      {
        // add in-between characters
        cm.append(to_add, string);

        // add match chunk
        cm+=L'&';
        cm+=*string;

        // update score and stats
        current_score = current_score - current_chunk_count - 0.1f*current_word_index;
        ++current_chunk_count;
      }

      // continue match
      if(match_impl(term+1, string+1, current_score, cm, string+1, current_chunk_count, current_word_index, true, best_score, best_match))
        found_match=true;
    }
    return found_match;
  }
}
//----------------------------------------------------------------------------


//============================================================================
// normalize_match_term()
//============================================================================
wstring normalized_term(const wstring &term)
{
  wstring result;
  for(wstring::const_iterator iter=term.begin(); iter!=term.end(); ++iter)
    if(!iswspace(*iter))
      result+=towupper(*iter);
  return result;
}
//----------------------------------------------------------------------------


//============================================================================
// match()
//============================================================================
bool match(const wchar_t *term, const wchar_t *string, float &score, std::wstring &marked_up_string)
{
  score=score_not_found();
  return match_impl(term, string, 0.0f, wstring(), string, 0, 0, false, score, marked_up_string);
}
//----------------------------------------------------------------------------
