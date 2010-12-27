//============================================================================
// splash_screen.cpp: Splash screen
//
// (c) Michael Walter, 2005-2007
//============================================================================

#include "splash_screen.h"
#include "gui.h"
#include "../libraries/win32/win.h"
using namespace std;
//----------------------------------------------------------------------------


//============================================================================
// local definitions
//============================================================================
namespace
{
  //==========================================================================
  // update_message
  //==========================================================================
  struct update_message
  {
    theme *theme;
    ufloat1 progress;
    const wchar_t *text;
  };
}
//----------------------------------------------------------------------------


//============================================================================
// splash_screen
//============================================================================
splash_screen::splash_screen(const std::wstring &monitor, theme &theme_)
  :m_monitor(monitor)
  ,m_theme(theme_)
  ,m_id_main_thread(0)
  ,m_splash_thread(0)
  ,m_win(0)
{
  // create splash thread
  m_id_main_thread=GetCurrentThreadId();
  DWORD id_splash_thread;
  m_splash_thread=CreateThread(0, 0, thread_proc, this, 0, &id_splash_thread);
  if(!m_splash_thread)
    throw runtime_error("Unable to create splash screen thread.");
  while(!m_win);
}
//----

splash_screen::~splash_screen()
{
  if(!TerminateThread(m_splash_thread, 0))
    throw runtime_error("Unable to terminate splash screen thread.");
}
//----------------------------------------------------------------------------

void splash_screen::update(ufloat1 progress, const wchar_t *text)
{
  update_message msg={&m_theme, progress, text};
  SendMessage(m_win, WM_USER, 0, reinterpret_cast<LPARAM>(&msg));
}
//----------------------------------------------------------------------------

DWORD WINAPI splash_screen::thread_proc(LPVOID param)
{
  static struct reg_splash_class {reg_splash_class() {reg_wnd_class(CS_NOCLOSE, &wnd_proc, 0, 0, GetModuleHandle(0), 0, LoadCursor(0, IDC_ARROW), 0, 0, L"ColibriSplashScreen", 0);}} s_reg_splash_class;

  // create splash window
  splash_screen &ss=*reinterpret_cast<splash_screen*>(param);
  const unsigned width=ss.m_theme.get_splash_screen_width(), height=ss.m_theme.get_splash_screen_height();
  HWND win=CreateWindowExW(WS_EX_LAYERED|WS_EX_TOOLWINDOW, L"ColibriSplashScreen", L"Colibri", WS_POPUP, 0, 0, width, height, 0, 0, GetModuleHandle(0), 0);
  if(!win)
    throw runtime_error("Unable to create splash screen window.");
  ss.m_win=win;
  vector<monitor> monitors=get_monitors();
  center_window(win, get_monitor(ss.m_monitor));

  // blit image onto splash window
  ss.update(0.0f, L"");

  // show window
  ShowWindow(win, SW_SHOW);
  SetWindowPos(win, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
  UpdateWindow(win);

  // attach thread input
  if(!AttachThreadInput(GetCurrentThreadId(), ss.m_id_main_thread, TRUE))
    throw runtime_error("Unable to attach input of splash screen thread to main thread.");

  // loop
  MSG msg;
  while(GetMessage(&msg, 0, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  // attach thread input
  if(!AttachThreadInput(GetCurrentThreadId(), ss.m_id_main_thread, FALSE))
    throw runtime_error("Unable to detach input of splash screen thread from main thread.");
  return 0;
}
//----

LRESULT CALLBACK splash_screen::wnd_proc(HWND win, UINT msg, WPARAM wparam, LPARAM lparam)
{
  // ignore repaint requests
  if(WM_PAINT==msg)
  {
    PAINTSTRUCT ps;
    BeginPaint(win, &ps);
    EndPaint(win, &ps);
    return 0;
  }

  // update splash screen status
  if(WM_USER==msg)
  {
    update_message &msg=*reinterpret_cast<update_message*>(lparam);

    // prepare graphics context
    dc dc(msg.theme->get_splash_screen_width(), msg.theme->get_splash_screen_height());
    Gdiplus::Graphics graphics(dc.get_dc());

    // update splash screen
    msg.theme->paint_splash_screen(graphics, msg.progress, msg.text);

    // update layered splash window
    dc.update(win);
  }

  return DefWindowProc(win, msg, wparam, lparam);
}
//----------------------------------------------------------------------------
