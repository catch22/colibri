//============================================================================
// dynlib.h: Dynamic library component
//
// (c) Michael Walter, 2006
//============================================================================

#ifndef UTILS_CORE_DYNLIB_H
#define UTILS_CORE_DYNLIB_H
#include "../win32/win32.h"
//----------------------------------------------------------------------------

// Interface:
class dynamic_library;
//----------------------------------------------------------------------------


//============================================================================
// dynamic_library
//============================================================================
class dynamic_library
{
public:
  // construction and destruction
  dynamic_library(const std::wstring &filename);
  ~dynamic_library();
  //--------------------------------------------------------------------------

  // accessors
  HMODULE get_hmodule() const;
  template<typename T> T *lookup(const char *name);
  template<typename T> T *lookup(unsigned ordinal);
  template<typename T> T *try_lookup(const char *name);
  template<typename T> T *try_lookup(unsigned ordinal);
  //--------------------------------------------------------------------------

private:
  dynamic_library(const dynamic_library&); // not implemented
  void operator=(const dynamic_library&); // not implemented
  typedef void procedure();
  procedure *try_lookup_impl(const char *name);
  //--------------------------------------------------------------------------

  std::wstring m_filename;
  HMODULE m_hmodule;
};
//----------------------------------------------------------------------------

#include "dynlib.inl"
#endif