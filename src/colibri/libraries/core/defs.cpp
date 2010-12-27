//============================================================================
// defs.h: Common definitions
//
// (c) Michael Walter, 2006
//============================================================================

#include "defs.h"
#include "../log/log.h"
#include "../win32/win32.h"
#include <cstdarg>
#pragma comment(lib, "comctl32.lib")
using namespace std;
//----------------------------------------------------------------------------


//============================================================================
// init_utils()
//============================================================================
void init_utils()
{
  // initialize com
  if(FAILED(CoInitialize(0)))
    throw runtime_error("Unable to initialize COM.");

  // initialize common controls
  INITCOMMONCONTROLSEX icc;
  icc.dwSize=sizeof(INITCOMMONCONTROLSEX);
  icc.dwICC=ICC_LISTVIEW_CLASSES|ICC_TAB_CLASSES;
  if(!InitCommonControlsEx(&icc))
    throw runtime_error("Unable to initialize common controls library.");

  // initialize secondary subsystems
  extern void init_win32_gfx();
  init_win32_gfx();
}
//----------------------------------------------------------------------------


//============================================================================
// shutdown_utils()
//============================================================================
void shutdown_utils()
{
  // shutdown secondary subsystems
  extern void shutdown_win32_gfx();
  shutdown_win32_gfx();

  // shutdown com
  CoUninitialize();
}
//----------------------------------------------------------------------------


//============================================================================
// throw_error()
//============================================================================
void throw_errorf(const char *format, ...)
{
  const unsigned buffer_size=1024;
  char buffer[buffer_size];

  // format message
  va_list args;
  va_start(args, format);
  _vsnprintf(buffer, buffer_size, format, args);
  buffer[buffer_size-1]=0;
  va_end(args);

  // report error
  throw runtime_error(buffer);
}
//----------------------------------------------------------------------------
