//============================================================================
// win.h: Window management
//
// (c) Michael Walter, 2006
//============================================================================

#ifndef UTILS_WIN32_WIN_H
#define UTILS_WIN32_WIN_H
#include "win32.h"
#include <vector>
//----------------------------------------------------------------------------

// Interface:
struct monitor;
std::vector<monitor> get_monitors();
monitor get_monitor(const std::wstring&);
monitor get_monitor(HWND);
void center_window(HWND, const monitor&);
void reg_wnd_class(UINT style, WNDPROC, int cls_extra, int wnd_extra, HINSTANCE, HICON, HCURSOR, HBRUSH background, LPCWSTR menu_name, LPCWSTR class_name, HICON small_icon);
//----------------------------------------------------------------------------


//============================================================================
// monitor
//============================================================================
struct monitor
{
  HMONITOR handle;
  std::wstring title;
  std::wstring name;
  RECT rect;
  bool is_primary;
};
//----------------------------------------------------------------------------

#endif