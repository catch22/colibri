//============================================================================
// splash_screen.h: Splash screen
//
// (c) Michael Walter, 2005-2007
//============================================================================

#ifndef COLIBRI_GUI_SPLASH_SCREEN_H
#define COLIBRI_GUI_SPLASH_SCREEN_H
#include "../core/defs.h"
#include "../libraries/win32/gfx.h"
class theme;
//----------------------------------------------------------------------------

// Interface:
class splash_screen;
//----------------------------------------------------------------------------


//============================================================================
// splash_screen
//============================================================================
class splash_screen
{
public:
  // construction and destruction
  splash_screen(const std::wstring &monitor, theme&);
  ~splash_screen();
  //--------------------------------------------------------------------------

  // control
  void update(ufloat1 progress, const wchar_t *text);
  //--------------------------------------------------------------------------

private:
  static DWORD WINAPI thread_proc(LPVOID);
  static LRESULT CALLBACK wnd_proc(HWND, UINT msg, WPARAM, LPARAM);
  //--------------------------------------------------------------------------

  std::wstring m_monitor;
  theme &m_theme;
  DWORD m_id_main_thread;
  HANDLE m_splash_thread;
  volatile HWND m_win;
};
//----------------------------------------------------------------------------

#endif