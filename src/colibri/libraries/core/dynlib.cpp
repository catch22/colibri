//============================================================================
// dynlib.cpp: Dynamic library component
//
// (c) Michael Walter, 2006
//============================================================================

#include "dynlib.h"
#include "../win32/win32.h"
using namespace std;
//----------------------------------------------------------------------------


//============================================================================
// dynamic_library
//============================================================================
dynamic_library::dynamic_library(const wstring &filename)
  :m_filename(filename)
  ,m_hmodule(LoadLibraryW(filename.c_str()))
{
  // load library
  if(!m_hmodule)
    throw_errorf("Unable to load library '%S'", filename.c_str());
}
//----

dynamic_library::~dynamic_library()
{
  // unload library
  FreeLibrary(m_hmodule);
}
//----------------------------------------------------------------------------

HMODULE dynamic_library::get_hmodule() const
{
  return m_hmodule;
}
//----

dynamic_library::procedure *dynamic_library::try_lookup_impl(const char *name)
{
  // lookup procedure
  return (procedure*)GetProcAddress(m_hmodule, name);
}
//----------------------------------------------------------------------------
