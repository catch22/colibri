//============================================================================
// main.cpp: Topaz
//
// (c) Michael Walter, 2005-2007
//============================================================================

#define _WIN32_WINNT 0x0400
#include <windows.h>
//----------------------------------------------------------------------------

// interface:
extern "C" __declspec(dllexport) bool topaz_init(HWND);
extern "C" __declspec(dllexport) void topaz_enable_hotkey(bool);
extern "C" __declspec(dllexport) bool topaz_is_hotkey_enabled();
extern "C" __declspec(dllexport) void topaz_set_hotkey(bool ctrl, bool alt, bool shift, bool lwin, bool rwin, DWORD vk);
LRESULT CALLBACK topaz_hook(int, WPARAM, LPARAM);
//----------------------------------------------------------------------------


//============================================================================
// shared data
//============================================================================
#pragma data_seg(".adshared")
HWND g_win=0;
bool g_hotkey_enabled=true;
bool g_ctrl=true, g_alt=false, g_shift=false, g_lwin=false, g_rwin=false;
DWORD g_vk=VK_SPACE;
#pragma data_seg()
#pragma comment(linker, "/SECTION:.adshared,RWS")
//----------------------------------------------------------------------------


//============================================================================
// topaz_init()
//============================================================================
extern "C" __declspec(dllexport) bool topaz_init(HWND win)
{
  // validate and store window handle
  if(!win)
  {
    OutputDebugStringA("topaz_init(): Invalid window handle.");
    return false;
  }
  g_win=win;

  // determine module handle
#ifdef _DEBUG
  HMODULE module=GetModuleHandleA("colibri_hotkey_agent_d.dll");
#else
  HMODULE module=GetModuleHandleA("colibri_hotkey_agent.dll");
#endif
  if(!module)
  {
    OutputDebugStringA("topaz_init(): Unable to determine module handle of Topaz.dll");
    return false;
  }

  // install windows hook
  HHOOK hook=SetWindowsHookEx(WH_KEYBOARD_LL, topaz_hook, module, 0);
  if(!hook)
  {
    OutputDebugStringA("topaz_init(): Unable to install low-level keyboard hook");
    return false;
  }
  return true;
}
//----------------------------------------------------------------------------


//============================================================================
// topaz_enable_hotkey()
//============================================================================
extern "C" __declspec(dllexport) void topaz_enable_hotkey(bool enable_)
{
  g_hotkey_enabled=enable_;
}
//----------------------------------------------------------------------------


//============================================================================
// topaz_is_hotkey_enabled()
//============================================================================
extern "C" __declspec(dllexport) bool topaz_is_hotkey_enabled()
{
  return g_hotkey_enabled;
}
//----------------------------------------------------------------------------


//============================================================================
// topaz_set_hotkey()
//============================================================================
extern "C" __declspec(dllexport) void topaz_set_hotkey(bool ctrl, bool alt, bool shift, bool lwin, bool rwin, DWORD vk)
{
  g_ctrl=ctrl;
  g_alt=alt;
  g_shift=shift;
  g_lwin=lwin;
  g_rwin=rwin;
  g_vk=vk;
}
//----------------------------------------------------------------------------


//============================================================================
// topaz_hook()
//============================================================================
LRESULT CALLBACK topaz_hook(int code, WPARAM wparam, LPARAM lparam)
{
  // post WM_USER when hotkey is pressed
  if(g_win && g_hotkey_enabled && HC_ACTION==code && (WM_KEYDOWN==wparam || WM_SYSKEYDOWN==wparam))
  {
    const KBDLLHOOKSTRUCT &khs=*reinterpret_cast<KBDLLHOOKSTRUCT*>(lparam);
    if(khs.vkCode==g_vk &&
       g_ctrl==(0!=(GetAsyncKeyState(VK_CONTROL)&0x8000)) &&
       g_alt==(0!=(GetAsyncKeyState(VK_MENU)&0x8000)) &&
       g_shift==(0!=(GetAsyncKeyState(VK_SHIFT)&0x8000)) &&
       g_lwin==(0!=(GetAsyncKeyState(VK_LWIN)&0x8000)) &&
       g_rwin==(0!=(GetAsyncKeyState(VK_RWIN)&0x8000)))
    {
      PostMessage(g_win, WM_USER, wparam, 0);
      return TRUE;
    }
  }
  return CallNextHookEx(0, code, wparam, lparam);
}
//----------------------------------------------------------------------------


//============================================================================
// DllMain
//============================================================================
BOOL WINAPI DllMain(HANDLE, DWORD, LPVOID)
{
  return TRUE;
}
//----------------------------------------------------------------------------
