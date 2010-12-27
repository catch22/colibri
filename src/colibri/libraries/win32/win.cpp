//============================================================================
// win.cpp: Window management
//
// (c) Michael Walter, 2006
//============================================================================

#include "win.h"
#include "../log/log.h"
using namespace std;
//----------------------------------------------------------------------------


//============================================================================
// anonymous namespace
//============================================================================
namespace
{
  string g_enum_error;

  //==========================================================================
  // get_monitor_title()
  //==========================================================================
  wstring get_monitor_title(const wchar_t *device_name)
  {
    // determine title of given display device
    DISPLAY_DEVICEW dev;
    dev.cb=sizeof(dev);
    if(!EnumDisplayDevicesW(device_name, 0, &dev, 0))
    {
      logger::warnf("Unable to get display device information for device '%S'.", device_name);
      return L"Unknown";
    }
    return dev.DeviceString;
  }
  //--------------------------------------------------------------------------


  //==========================================================================
  // monitor_enum_proc()
  //==========================================================================
  BOOL CALLBACK monitor_enum_proc(HMONITOR handle, HDC, LPRECT, LPARAM data)
  {
    // get monitor info
    MONITORINFOEXW info;
    info.cbSize=sizeof(info);
    if(!GetMonitorInfoW(handle, &info))
    {
      g_enum_error="Unable to get monitor information.";
      return FALSE;
    }
    // add monitor record
    monitor m;
    m.handle=handle;
    m.title=get_monitor_title(info.szDevice);
    m.name=info.szDevice;
    m.rect=info.rcMonitor;
    m.is_primary=MONITORINFOF_PRIMARY==(info.dwFlags&MONITORINFOF_PRIMARY);
    reinterpret_cast<vector<monitor>*>(data)->push_back(m);
    return TRUE;
  }
  //--------------------------------------------------------------------------


  //==========================================================================
  // monitor_sort
  //==========================================================================
  bool monitor_before(const monitor &m1, const monitor &m2)
  {
    return (m1.is_primary && !m2.is_primary) || (m1.is_primary==m2.is_primary && m1.title < m2.title);
  }
}
//----------------------------------------------------------------------------


//============================================================================
// get_monitors()
//============================================================================
vector<monitor> get_monitors()
{
  // enumerate monitors
  vector<monitor> monitors;
  g_enum_error.clear();
  g_enum_error="Unable to enumerate available monitors.";
  if(!EnumDisplayMonitors(0, 0, monitor_enum_proc, reinterpret_cast<LPARAM>(&monitors)))
    throw runtime_error(g_enum_error.c_str());

  // sort monitor list
  std::sort(monitors.begin(), monitors.end(), monitor_before);
  return monitors;
}
//----------------------------------------------------------------------------


//============================================================================
// get_monitor()
//============================================================================
monitor get_monitor(const std::wstring &name)
{
  const vector<monitor> &m=get_monitors();
  for(vector<monitor>::const_iterator iter=m.begin(); iter!=m.end(); ++iter)
    if(iter->name==name)
      return *iter;
  return m.front();
}
//----------------------------------------------------------------------------


//============================================================================
// get_monitor()
//============================================================================
monitor get_monitor(HWND win)
{
  // get monitor info for given window
  HMONITOR handle=MonitorFromWindow(win, MONITOR_DEFAULTTONEAREST);
  MONITORINFOEXW info;
  info.cbSize=sizeof(MONITORINFOEXW);
  if(!GetMonitorInfo(handle, &info))
    throw_errorf("Unable to retrieve monitor information for window '%p'.", win);

  // add monitor record
  monitor m;
  m.handle=handle;
  m.title=get_monitor_title(info.szDevice);
  m.name=info.szDevice;
  m.rect=info.rcMonitor;
  m.is_primary=MONITORINFOF_PRIMARY==(info.dwFlags&MONITORINFOF_PRIMARY);
  return m;
}
//----------------------------------------------------------------------------


//============================================================================
// center_window()
//============================================================================
void center_window(HWND win, const monitor &monitor)
{
  // get window rect
  RECT rc;
  if(!GetWindowRect(win, &rc))
    throw_errorf("Unable to determine window rectangle for window '%p'.", win);
  const int width=rc.right-rc.left, height=rc.bottom-rc.top;

  // center window on monitor
  const int x=monitor.rect.left+(monitor.rect.right-monitor.rect.left-width)/2;
  const int y=monitor.rect.top+(monitor.rect.bottom-monitor.rect.top-height)/2;
  if(!SetWindowPos(win, 0, x, y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE))
    throw_errorf("Unable to center window '%p'.", win);
}
//----------------------------------------------------------------------------


//============================================================================
// reg_wnd_class()
//============================================================================
void reg_wnd_class(UINT style, WNDPROC wnd_proc, int cls_extra, int wnd_extra, HINSTANCE instance, HICON icon, HCURSOR cursor, HBRUSH background, LPCWSTR menu_name, LPCWSTR class_name, HICON small_icon)
{
  // setup window class structure
  WNDCLASSEXW wc;
  wc.cbSize=sizeof(WNDCLASSEXW);
  wc.style=style;
  wc.lpfnWndProc=wnd_proc;
  wc.cbClsExtra=cls_extra;
  wc.cbWndExtra=wnd_extra;
  wc.hInstance=instance;
  wc.hIcon=icon;
  wc.hCursor=cursor;
  wc.hbrBackground=background;
  wc.lpszMenuName=menu_name;
  wc.lpszClassName=class_name;
  wc.hIconSm=small_icon;

  // register window class
  if(!RegisterClassExW(&wc))
    throw_errorf("Unable to register window class '%S'", class_name);
}
//----------------------------------------------------------------------------
