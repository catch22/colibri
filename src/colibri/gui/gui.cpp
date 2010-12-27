//============================================================================
// gui.cpp: Brick GUI
//
// (c) Michael Walter, 2005-2007
//============================================================================

#include "gui.h"
#include "controller.h"
#include "../core/version.h"
#include "../plugins/colibri_plugin.h"
#include "../libraries/win32/shell.h"
#include "../libraries/win32/win.h"
#include "../libraries/log/log.h"
#include <set>
#include <sstream>
using namespace std;
using namespace boost;
//----------------------------------------------------------------------------


//============================================================================
// local definitions
//============================================================================
namespace
{
  //==========================================================================
  // null_controller
  //==========================================================================
  class null_controller: public controller
  {
  public:
    // controller gui
    virtual bool on_tab(gui&);
    virtual bool on_enter(gui&);
    virtual void on_input_changed(gui&);
  };
  //--------------------------------------------------------------------------

  bool null_controller::on_tab(gui&)
  {
    return false;
  }
  //----

  bool null_controller::on_enter(gui&)
  {
    return false;
  }
  //----

  void null_controller::on_input_changed(gui&)
  {
  }
}
//----------------------------------------------------------------------------


//============================================================================
// Interface::brick
//============================================================================
struct gui::brick
{
  // construction and destruction
  brick(e_brick_type, HWND, controller&, boost::optional<icon_info> custom_icon);
  ~brick();
  //--------------------------------------------------------------------------

  // general properties
  const e_brick_type type;
  const HWND handle;
  controller &controller;
  boost::optional<icon_info> custom_icon;
  //--------------------------------------------------------------------------

  // shared state
  wstring input;
  //--------------------------------------------------------------------------

  // option brick state
  unsigned y_scroll_amount;
  vector<option> options;
  vector<option>::size_type active_option_index; // well-defined iff options.size()>0
  boost::optional<boost::uint64_t> last_user_activated_option; // last option consciously activated by user
  //--------------------------------------------------------------------------

  // text brick state
  //--------------------------------------------------------------------------

  // credits brick state
  DWORD credits_start_ticks;
};
//----------------------------------------------------------------------------

gui::brick::brick(e_brick_type type, HWND handle, ::controller &controller, boost::optional<icon_info> custom_icon)
  :type(type)
  ,handle(handle)
  ,controller(controller)
  ,custom_icon(custom_icon)
  ,y_scroll_amount(0)
  ,active_option_index(0)
  ,last_user_activated_option(none)
  ,credits_start_ticks(0)
{
}
//----

gui::brick::~brick()
{
  DestroyWindow(handle);
  delete &controller;
}
//----------------------------------------------------------------------------


//============================================================================
// gui
//============================================================================
gui *gui::s_instance=0;
//----------------------------------------------------------------------------
gui::gui(colibri_plugin &colibri)
  :m_theme(new theme(colibri.get_theme()))
  ,m_colibri(colibri)
  ,m_splash_screen(colibri.is_splash_screen_enabled() ? new splash_screen(colibri.get_monitor(), *m_theme) : 0)
#ifdef _DEBUG
  ,m_topaz(L"colibri_hotkey_agent_d.dll")
#else
  ,m_topaz(L"colibri_hotkey_agent.dll")
#endif
  ,m_in_startup(true)
{
  static struct reg_classes
  {
    reg_classes()
    {
      reg_wnd_class(CS_NOCLOSE, &wnd_proc_brick, 0, 0, GetModuleHandle(0), 0, LoadCursor(0, IDC_ARROW), 0, 0, L"ColibriBrick", 0);
      reg_wnd_class(CS_NOCLOSE, &wnd_proc_dropdown, 0, 0, GetModuleHandle(0), 0, LoadCursor(0, IDC_ARROW), 0, 0, L"ColibriDropdown", 0);
      reg_wnd_class(CS_NOCLOSE, &wnd_proc_dimmer, 0, 0, GetModuleHandle(0), 0, LoadCursor(0, IDC_ARROW), (HBRUSH)GetStockObject(BLACK_BRUSH), 0, L"ColibriDimmer", 0);
    }
  } s_reg_classes;
  s_instance=this;

  // load icons
  m_icon_colibri=LoadIconW(GetModuleHandle(0), MAKEINTRESOURCEW(IDI_COLIBRI));
  m_icon_colibri_l=LoadIconW(GetModuleHandle(0), MAKEINTRESOURCEW(IDI_COLIBRI_L));
  m_icon_colibri_d=LoadIconW(GetModuleHandle(0), MAKEINTRESOURCEW(IDI_COLIBRI_D));
  m_icon_colibri_dl=LoadIconW(GetModuleHandle(0), MAKEINTRESOURCEW(IDI_COLIBRI_DL));
  if(!m_icon_colibri || !m_icon_colibri_l || !m_icon_colibri_d || !m_icon_colibri_dl)
    throw runtime_error("Unable to load icons.");

  // load context menu
  m_context_menu=LoadMenuW(GetModuleHandle(0), L"ContextMenu");
  if(!m_context_menu)
    throw runtime_error("Unable to load context menu.");

  // get topaz entry points
  m_topaz_init=m_topaz.lookup<topaz_init>("topaz_init");
  m_topaz_enable_hotkey=m_topaz.lookup<topaz_enable_hotkey>("topaz_enable_hotkey");
  m_topaz_is_hotkey_enabled=m_topaz.lookup<topaz_is_hotkey_enabled>("topaz_is_hotkey_enabled");
  m_topaz_set_hotkey=m_topaz.lookup<topaz_set_hotkey>("topaz_set_hotkey");

  // create dropdown
  m_dropdown=CreateWindowExW(WS_EX_LAYERED|WS_EX_TOOLWINDOW, L"ColibriDropdown", L"Colibri", WS_POPUP, 0, 0, m_theme->get_dropdown_width(), m_theme->get_dropdown_height(), 0, 0, GetModuleHandle(0), this);
  if(!m_dropdown)
    throw runtime_error("Unable to create drop down window.");

  // create dimmer
  m_dimmer=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_LAYERED|WS_EX_TRANSPARENT, L"ColibriDimmer", L"Colibri", WS_POPUP, 0, 0, 10, 10, 0, 0, GetModuleHandle(0), this);
  if(!m_dimmer)
    throw runtime_error("Unable to create dimmer window.");
  if(!SetLayeredWindowAttributes(m_dimmer, 0, 0, LWA_ALPHA))
    throw runtime_error("Unable to set alpha to zero.");

  // create tray icon
  update_tray_icon(true);

  // set hotkey
  update_hotkey();

  // update gui
  enable_hotkey(m_colibri.is_hotkey_enabled());
}
//----

gui::~gui()
{
  // pop all seconcary bricks
  while(m_bricks.size()>1)
    pop_brick();

  // close primary brick
  if(m_bricks.size())
  {
    // remove tray icon
    NOTIFYICONDATAW nid;
    nid.cbSize=sizeof(NOTIFYICONDATA);
    nid.hWnd=m_dropdown;
    nid.uID=0;
    nid.uFlags=0;
    Shell_NotifyIconW(NIM_DELETE, &nid);

    // free main brick
    m_bricks.pop_front();
  }

  // free dropdown
  DestroyWindow(m_dropdown);

  // free dimmer
  DestroyWindow(m_dimmer);

  // free context menu
  DestroyMenu(m_context_menu);
}
//----

void gui::update_splash_screen(ufloat1 progress, const wstring &text)
{
  if(!m_splash_screen.get())
    return;

  // update splash screen
  m_splash_screen->update(progress, text.c_str());
}
//----

void gui::notify_startup_complete()
{
  m_in_startup=false;
  update_tray_icon();
  m_splash_screen.reset();
}
//----

void gui::restart_colibri()
{
  // main brick is backmost brick
  PostMessageW(m_bricks.back().handle, WM_COLIBRI_RESTART, 0, 0);
}
//----

void gui::popup_other_colibri()
{
  // determine Colibri window handle
  HWND win=FindWindowW(0, L"ColibriMain");
  if(!win)
  {
    logger::warn("Unable to determine Colibri main window.");
    return;
  }

  // popup Colibri
  PostMessageW(win, WM_COLIBRI_ACTIVATE, 0, 0);
}
//----------------------------------------------------------------------------

void gui::enable_hotkey(bool enable)
{
  // already enabled?
  if(enable==is_hotkey_enabled())
    return;

  // enable hot key
  if(!m_topaz_enable_hotkey)
    throw logic_error("Unable to enable hotkey, Topaz not initialized yet.");
  m_topaz_enable_hotkey(enable);

  // update tray icon
  update_tray_icon();
}
//----

bool gui::is_hotkey_enabled() const
{
  if(!m_topaz_is_hotkey_enabled)
    throw logic_error("Unable to query hotkey enable state, Topaz not initialized yet.");
  return m_topaz_is_hotkey_enabled();
}
//----------------------------------------------------------------------------

void gui::push_option_brick(controller *controller)
{
  push_brick(brick_type_option, *controller);
}
//----

void gui::push_text_brick(controller *controller, boost::optional<icon_info> custom_icon)
{
  push_brick(brick_type_text, *controller, custom_icon);
}
//----

void gui::push_credits_brick()
{
  push_brick(brick_type_credits, *new null_controller);
}
//----

void gui::push_hotkey_brick()
{
  push_brick(brick_type_hotkey, *new null_controller);
}
//----

void gui::pop_brick()
{
  if(m_bricks.size()<=1)
    throw runtime_error("Cannot pop main brick.");

  // apply hotkey, if necessary
  if(brick_type_hotkey==get_current_brick().type)
    update_hotkey();

  // destroy brick
  m_bricks.pop_front();

  // relayout user gui
  relayout();

  // update controller
  on_input_changed();
}
//----

void gui::hide()
{
  // don't hide during shutdown
  if(!m_bricks.size())
    return;

  // avoid recursive calls
  static bool s_hiding=false;
  if(s_hiding)
    return;
  s_hiding=true;

  // hide dropdown
  ShowWindow(m_dropdown, SW_HIDE);

  // hide all bricks
  for(ptr_deque<brick>::reverse_iterator iter=m_bricks.rbegin(); iter!=m_bricks.rend(); ++iter)
    ShowWindow(iter->handle, SW_HIDE);

  // pop secondary bricks
  while(m_bricks.size()>1)
    pop_brick();

  // hide dimmer
  for(int i=64; i>=0; i-=16)
    if(!SetLayeredWindowAttributes(m_dimmer, 0, i, LWA_ALPHA))
      throw std::runtime_error("Unable to dim window.");
  ShowWindow(m_dimmer, SW_HIDE);

  // reset main brick
  get_current_brick().input=L"";
  on_input_changed();

  s_hiding=false;
}
//----

void gui::refresh()
{
  on_input_changed();
}
//----------------------------------------------------------------------------

const wchar_t *gui::get_current_term() const
{
  // return search term of current brick
  if(brick_type_option!=get_current_brick().type)
    throw logic_error("get_current_term() should only be called for option bricks.");
  return get_current_brick().input.c_str();
}
//----

optional<boost::uint64_t> gui::get_current_option_data() const
{
  // return data of current brick
  if(brick_type_option!=get_current_brick().type)
    throw logic_error("get_current_option_data() should only be called for option bricks.");
  const brick &brick=get_current_brick();
  if(brick.active_option_index<brick.options.size())
    return brick.options[brick.active_option_index].data;
  return none;
}
//----

void gui::set_current_option(unsigned index)
{
  // set current option by index
  if(brick_type_option!=get_current_brick().type)
    throw logic_error("set_current_option() should only be called for option bricks.");
  brick &brick=get_current_brick();
  if(index>=brick.options.size())
    throw_errorf("Option index %u should be in [0,%u).", index, brick.options.size());
  brick.active_option_index=index;

  // repaint brick and dropdown
  repaint(get_current_brick());
  repaint_dropdown();
}
//----

void gui::add_term_char(wchar_t ch)
{
  try_add_char(ch);
  on_input_changed();
}
//----------------------------------------------------------------------------

const wchar_t *gui::get_current_text() const
{
  // return text of current brick
  if(brick_type_text!=get_current_brick().type)
    throw logic_error("get_current_text() should only be called for text bricks.");
  return get_current_brick().input.c_str();
}
//----------------------------------------------------------------------------

LRESULT CALLBACK gui::wnd_proc_brick(HWND win, UINT msg, WPARAM wparam, LPARAM lparam)
{
  gui &gui=*s_instance;

  // ignore repaint requests
  if(WM_PAINT==msg)
  {
    PAINTSTRUCT ps;
    BeginPaint(win, &ps);
    EndPaint(win, &ps);
    return 0;
  }

  // allow operating system shutdown
  if(WM_QUERYENDSESSION==msg)
  {
    PostQuitMessage(0);
    return 1;
  }

  // hide gui on deactivation
  if(WM_ACTIVATEAPP==msg && !wparam)
  {
    gui.hide();
    return 0;
  }

  // install timers when brick is shown
  if(WM_SHOWWINDOW==msg && wparam)
  {
    // start drop down timer
    if(!SetTimer(win, IDT_DROPDOWN, 1000, 0))
      throw runtime_error("Unable to install drop down timer.");

    // start credits animation timer
    brick &brick=gui.get_current_brick();
    if(brick_type_credits==brick.type)
    {
      if(!SetTimer(win, IDT_CREDITS, 30, 0))
        throw runtime_error("Unable to install credits animation timer.");
      brick.credits_start_ticks=GetTickCount();
    }
    return 0;
  }
  // kill timers when brick is hidden
  if(WM_SHOWWINDOW==msg && !wparam)
  {
    // kill timers
    KillTimer(win, IDT_DROPDOWN);
    KillTimer(win, IDT_CREDITS);
    return 0;
  }
  // reset dropdown timer on key press
  if(WM_KEYDOWN==msg)
  {
    // reset timer if key has been pressed
    if(!SetTimer(win, IDT_DROPDOWN, 1000, 0))
      throw runtime_error("Unable to install drop down timer.");
  }
  // show dropdown when timer triggers
  if(WM_TIMER==msg && IDT_DROPDOWN==wparam)
  {
    // show dropdown only if there is any user input
    const brick &brick=gui.get_current_brick();
    if(brick_type_option==brick.type && brick.input.size()>0)
    {
      ShowWindow(gui.m_dropdown, SW_SHOWNOACTIVATE);
      SetWindowPos(gui.m_dropdown, brick.handle, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    }
    return 0;
  }
  // repaint brick when timer triggers
  if(WM_TIMER==msg && IDT_CREDITS==wparam)
  {
    brick &brick=gui.get_current_brick();
    if(brick_type_credits==brick.type)
      gui.repaint(brick);
  }

  // Ctrl-Space: show main brick
  if(WM_COLIBRI_ACTIVATE==msg)
  {
    // reset hot key
    brick &brick=gui.get_current_brick();
    if(brick_type_hotkey==brick.type)
      gui.repaint(brick);

    // bring main brick to foreground
    DWORD pidForeground=GetWindowThreadProcessId(GetForegroundWindow(), 0);
    DWORD pidSelf=GetWindowThreadProcessId(win, 0);
    if(pidForeground!=pidSelf)
    {
      AttachThreadInput(pidForeground, pidSelf, TRUE);
      SetForegroundWindow(win);
      AttachThreadInput(pidForeground, pidSelf, FALSE);
    }
    else
      SetForegroundWindow(win);

    // show dimmer
    ShowWindow(gui.m_dimmer, SW_SHOW);
    for(int i=0; i<=64; i+=16)
      if(!SetLayeredWindowAttributes(gui.m_dimmer, 0, i, LWA_ALPHA))
        throw std::runtime_error("Unable to dim window.");

    // show/restore main window and set focus
    ShowWindow(win, SW_SHOW);
    SetFocus(win);
    return 0;
  }

  // Escape: hide gui
  if(WM_KEYDOWN==msg && VK_ESCAPE==wparam)
  {
    gui.hide();
    return 0;
  }

  // Shift-Tab: pop secondary brick/hide gui
  if(WM_KEYDOWN==msg && ((VK_TAB==wparam && GetAsyncKeyState(VK_SHIFT)&0x8000) || VK_LEFT==wparam))
  {
    database &db=gui.m_colibri.get_db();
    if (db.trigger_action(L"colibri_actions.press_brick_pop_button"))
      return 0;

    if(gui.m_bricks.size()>1)
      gui.pop_brick();
    else
      gui.hide();
    return 0;
  }

  // Alt-F4: quit colibri
  if(WM_SYSKEYDOWN==msg && VK_F4==wparam && GetAsyncKeyState(VK_MENU)&0x8000)
  {
    PostQuitMessage(0);
    return 0;
  }

  // Hotkey brick: handle all key presses
  if(gui.m_bricks.size() && brick_type_hotkey==gui.get_current_brick().type && (WM_KEYDOWN==msg || WM_KEYUP==msg || WM_SYSKEYDOWN==msg || WM_SYSKEYUP==msg || WM_CHAR==msg || WM_SYSCHAR==msg))
  {
    if(WM_KEYDOWN==msg || WM_SYSKEYDOWN==msg)
    {
      bool ctrl=0!=(GetAsyncKeyState(VK_CONTROL)&0x8000);
      bool alt=0!=(GetAsyncKeyState(VK_MENU)&0x8000);
      bool shift=0!=(GetAsyncKeyState(VK_SHIFT)&0x8000);
      bool lwin=0!=(GetAsyncKeyState(VK_LWIN)&0x8000);
      bool rwin=0!=(GetAsyncKeyState(VK_RWIN)&0x8000);
      if((VK_CONTROL!=wparam && VK_MENU!=wparam && VK_SHIFT!=wparam && VK_LWIN!=wparam && VK_RWIN!=wparam) && (ctrl || alt || lwin || rwin))
      {
        // update hotkey
        hotkey hk;
        hk.ctrl=ctrl;
        hk.alt=alt;
        hk.shift=shift;
        hk.lwin=lwin;
        hk.rwin=rwin;
        hk.vk=DWORD(wparam);
        gui.m_colibri.set_hotkey(hk);
        gui.repaint(gui.get_current_brick());
      }
      else if(VK_F1<=wparam && wparam<=VK_F24)
      {
        // update hotkey
        hotkey hk;
        hk.ctrl=ctrl;
        hk.alt=alt;
        hk.shift=shift;
        hk.lwin=lwin;
        hk.rwin=rwin;
        hk.vk=DWORD(wparam);
        gui.m_colibri.set_hotkey(hk);
        gui.repaint(gui.get_current_brick());
      }
    }
    return 0;
  }

  // Enter/Tab/Right arrow: redirect to controller
  if(WM_KEYDOWN==msg && (VK_RETURN==wparam || VK_TAB==wparam || VK_RIGHT==wparam))
  {
    // delegate event to controller
    controller &ctrl=gui.get_current_brick().controller;
    bool ok=VK_RETURN==wparam ? ctrl.on_enter(gui) : ctrl.on_tab(gui);
    if(!ok)
      MessageBeep(MB_ICONEXCLAMATION);
    return 0;
  }

  // Ctrl-V/Shift-Insert: paste text from clipboard
  if(WM_KEYDOWN==msg && ((L'V'==wparam && GetAsyncKeyState(VK_CONTROL)&0x8000) ||
                         (VK_INSERT==wparam && GetAsyncKeyState(VK_SHIFT)&0x8000)))
  {
    // open clipboard
    bool ok=false;
    if(OpenClipboard(win))
    {
      // paste text if available
      if(IsClipboardFormatAvailable(CF_UNICODETEXT))
      {
        // get clipboard data
        if(HANDLE mem=GetClipboardData(CF_UNICODETEXT))
        {
          // lock clipboard data
          const wchar_t *str=reinterpret_cast<const wchar_t*>(GlobalLock(mem));
          if(str)
          {
            // append text
            while(*str)
              ok|=gui.try_add_char(*str++);
            if(ok)
              gui.on_input_changed();
  
            // unlock clipboard data
            GlobalUnlock(mem);
          }
          else
            logger::warn("Unable to lock clipboard.");
        }
        else
          logger::warn("Unable to get clipboard data.");
      }

      // close clipboard
      CloseClipboard();
    }
    else
      logger::warn("Unable to open clipboard.");

    // beep on failure
    if(!ok)
      MessageBeep(MB_ICONEXCLAMATION);
    return 0;
  }

  // Arrow up/down: Select previous/next option
  // Page up/down: Scroll page up/down
  if(WM_KEYDOWN==msg && (VK_UP==wparam || VK_DOWN==wparam || VK_PRIOR==wparam || VK_NEXT==wparam))
  {
    brick &brick=gui.get_current_brick();
    bool ok=false;
    if(const vector<option>::size_type num_options=brick.options.size())
    {
      const unsigned dropdown_rows_per_page=gui.m_theme->get_dropdown_rows_per_page();
      switch(wparam)
      {
      case VK_UP:
        // select previous option
        if(ok=brick.active_option_index>0)
          --brick.active_option_index;
        break;

      case VK_DOWN:
        // select next option
        if(ok=brick.active_option_index<num_options-1)
          ++brick.active_option_index;
        break;

      case VK_PRIOR:
        // scroll page up
        if(ok=brick.active_option_index>0)
          brick.active_option_index=brick.active_option_index>=dropdown_rows_per_page ? brick.active_option_index-dropdown_rows_per_page : 0;
        break;

      case VK_NEXT:
        // scroll page down
        if(ok=brick.active_option_index<num_options-1)
          brick.active_option_index=brick.active_option_index+dropdown_rows_per_page>=num_options ? num_options-1 : brick.active_option_index+dropdown_rows_per_page;
        break;
      }
    }

    // show dropdown
    if(brick_type_option==brick.type)
    {
      ShowWindow(gui.m_dropdown, SW_SHOWNOACTIVATE);
      SetWindowPos(gui.m_dropdown, brick.handle, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    }

    // scroll&repaint on success
    if(ok)
    {
      // remember option as having been conciously actived by the user
      brick.last_user_activated_option=brick.options[brick.active_option_index].data;

      // update scrolling and repaint
      gui.repaint(brick);
      gui.repaint_dropdown();
    }
    else
      MessageBeep(MB_ICONEXCLAMATION);
    return 0;
  }

  // (Ctrl+)Backspace: erase last character/word
  if(WM_CHAR==msg && (VK_BACK==wparam || 0x7f==wparam))
  {
    brick &brick=gui.get_current_brick();
    const unsigned input_len=unsigned(brick.input.size());
    if(input_len>0)
    {
      // Ctrl pressed?
      if(0x7f==wparam)
      {
        // erase entire last word (or whitespace sequence)
        bool skipWS=0!=iswspace(brick.input[input_len-1]);
        unsigned count=1;
        while(count<input_len && (skipWS ^ (0==iswspace(brick.input[input_len-1-count]))))
          ++count;
        brick.input.resize(input_len-count);
      }
      else
        brick.input.resize(input_len-1);
      gui.on_input_changed();
    }
    else
      MessageBeep(MB_ICONEXCLAMATION);
    return 0;
  }

  // Any printable character: add to text string
  if(WM_CHAR==msg && wparam>=L' ')
  {
    if(gui.try_add_char(wchar_t(wparam)))
      gui.on_input_changed();
  }

  return DefWindowProc(win, msg, wparam, lparam);
}
//----

LRESULT CALLBACK gui::wnd_proc_dropdown(HWND win, UINT msg, WPARAM wparam, LPARAM lparam)
{
  gui &gui=*s_instance;

  // ignore repaint requests
  if(WM_PAINT==msg)
  {
    PAINTSTRUCT ps;
    BeginPaint(win, &ps);
    EndPaint(win, &ps);
    return 0;
  }

  // ignore all other messages until Colibri has been initialized
  if(gui.m_in_startup)
    return DefWindowProc(win, msg, wparam, lparam);

  // activate current brick on activation
  if(WM_ACTIVATE==msg && WA_INACTIVE!=wparam)
  {
    ShowWindow(gui.get_current_brick().handle, SW_SHOW);
    return 0;
  }

  // Tray icon left click, tray icon context menu->open: show main brick
  if((WM_COLIBRI_TRAYICON==msg && WM_LBUTTONDOWN==lparam) ||
     (WM_COMMAND==msg && IDC_COLIBRI_OPEN==LOWORD(wparam)))
  {
    if(gui.m_bricks.size()==1)
      PostMessage(gui.get_current_brick().handle, WM_COLIBRI_ACTIVATE, 0, 0);
    return 0;
  }

  // Tray icon right click: show context menu
  if(WM_COLIBRI_TRAYICON==msg && WM_RBUTTONDOWN==lparam)
  {
    // determine mouse cursor position
    POINT pos;
    GetCursorPos(&pos);

    // retrieve context menu, set "Open" as default item, check "Enable Hotkey" as required
    HMENU menu=GetSubMenu(gui.m_context_menu, 0);    
    SetMenuDefaultItem(menu, 0, TRUE);
    CheckMenuItem(menu, IDC_COLIBRI_ENABLE_HOTKEY, MF_BYCOMMAND|(gui.is_hotkey_enabled()?MF_CHECKED:MF_UNCHECKED));

    // popup context menu (for magic, see Platform SDK on TrackPopupMenu)
    /*SetForegroundWindow(win_);*/
    TrackPopupMenu(menu, TPM_LEFTALIGN|TPM_LEFTBUTTON, pos.x, pos.y, 0, win, 0);
    PostMessage(win, WM_NULL, 0, 0);
    return 0;
  }

  // Tray icon context menu->Enable Hotkey: Toggle hotkey enable state
  if(WM_COMMAND==msg && IDC_COLIBRI_ENABLE_HOTKEY==LOWORD(wparam))
  {
    gui.m_colibri.enable_hotkey(!gui.m_colibri.is_hotkey_enabled());
    return 0;
  }

  // Tray icon context menu->Preferences: Show Colibri preferencees
  if(WM_COMMAND==msg && IDC_COLIBRI_PREFERENCES==LOWORD(wparam))
  {
    if(gui.m_bricks.size()==1)
    {
      // show main brick
      SendMessage(gui.get_current_brick().handle, WM_COLIBRI_ACTIVATE, 0, 0);

      // open colibri->preferences
      database &db=gui.m_colibri.get_db();
      db.trigger_action(L"colibri_actions.open_colibri_menu");
      db.trigger_action(L"colibri_actions.open_preferences_menu");
    }
    return 0;
  }

  // Tray icon context menu->Help: Show Colibri help
  if(WM_COMMAND==msg && IDC_COLIBRI_HELP==LOWORD(wparam))
  {
    if(!launch(L"http://colibri.leetspeak.org/help"))
      throw runtime_error("Unable to open Colibri help page.");
    return 0;
  }

  // Tray icon context menu->Exit: quit colibri
  if(WM_COMMAND==msg && IDC_COLIBRI_EXIT==LOWORD(wparam))
  {
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProc(win, msg, wparam, lparam);
}
//----

LRESULT CALLBACK gui::wnd_proc_dimmer(HWND win, UINT msg, WPARAM wparam, LPARAM lparam)
{
  gui &gui=*s_instance;

  // ignore repaint requests
  if(WM_PAINT==msg)
  {
    PAINTSTRUCT ps;
    BeginPaint(win, &ps);
    EndPaint(win, &ps);
    return 0;
  }

  // ignore all other messages until Colibri has been initialized
  if(gui.m_in_startup)
    return DefWindowProc(win, msg, wparam, lparam);
  return DefWindowProc(win, msg, wparam, lparam);
}
//----------------------------------------------------------------------------

void gui::push_brick(e_brick_type type, controller &controller, boost::optional<icon_info> custom_icon)
{
  // hide dropdown
  ShowWindow(m_dropdown, SW_HIDE);

  // create new brick
  HWND win=CreateWindowExW(WS_EX_LAYERED|WS_EX_TOOLWINDOW, L"ColibriBrick", L"Colibri", WS_POPUP, 0, 0, m_theme->get_brick_width(), m_theme->get_brick_height(), 0, 0, GetModuleHandle(0), this);
  if(!win)
    throw runtime_error("Unable to create brick window.");

  // push onto brick stack
  brick *brick=new gui::brick(type, win, controller, custom_icon);
  m_bricks.push_front(brick);

  // first brick gets promoted to main brick
  if(1==m_bricks.size())
  {
    // rename brick
    if(!SetWindowTextW(win, L"ColibriMain"))
      throw runtime_error("Unable to update window title.");

    // initialize Topaz
    if(!m_topaz_init(brick->handle))
      throw runtime_error("Topaz initialization failed, see debug output.");

    // determine screen bounds
    m_rc_monitor=get_monitor(m_colibri.get_monitor()).rect;
  }

  // invoke controller
  controller.on_input_changed(*this);

  // relayout user gui
  relayout();

  // show secondary bricks by default
  if(m_bricks.size()>1)
  {
    // show brick
    ShowWindow(brick->handle, SW_SHOW);
    UpdateWindow(brick->handle);
  }
}
//----

const gui::brick &gui::get_current_brick() const
{
  return m_bricks.front();
}
//----

gui::brick &gui::get_current_brick()
{
  return m_bricks.front();
}
//----

vector<gui::option> &gui::get_options()
{
  // return search term of current brick
  if(brick_type_option!=get_current_brick().type)
    throw logic_error("get_options() should only be called for option bricks.");
  return get_current_brick().options;
}
//----

bool gui::try_add_char(wchar_t ch)
{
  // filter control chars
  if(ch<L' ')
    return false;

  // add char
  get_current_brick().input+=ch;
  return true;
}
//----

void gui::on_input_changed()
{
  // repaint brick and dropdown
  repaint(get_current_brick());
  repaint_dropdown();

  // trigger event handler
  get_current_brick().controller.on_input_changed(*this);
}
//----

void gui::on_options_changed()
{
  // select and scroll to current option (if possible)
  brick &brick=get_current_brick();
  brick.active_option_index=0;
  if(optional<boost::uint64_t> last_user_activated_option=brick.last_user_activated_option)
  {
    // try to select current option
    brick.last_user_activated_option=none;
    for(vector<option>::size_type idx=0; idx<brick.options.size(); ++idx)
    {
      if(*last_user_activated_option==brick.options[idx].data)
      {
        brick.active_option_index=idx;
        brick.last_user_activated_option=*last_user_activated_option;
        break;
      }
    }
  }

  // repaint brick and dropdown
  repaint(brick);
  repaint_dropdown();
}
//----

void gui::update_tray_icon(bool add)
{
  // update tray icon
  NOTIFYICONDATAW nid;
  nid.cbSize=sizeof(NOTIFYICONDATA);
  nid.hWnd=m_dropdown;
  nid.uID=0;
  nid.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;
  nid.uCallbackMessage=WM_COLIBRI_TRAYICON;
  nid.hIcon=m_in_startup ? (is_hotkey_enabled()?m_icon_colibri_l:m_icon_colibri_dl) : (is_hotkey_enabled()?m_icon_colibri:m_icon_colibri_d);
  wcscpy(nid.szTip, L"Colibri");

  // update tray icon?
  if(!add)
  {
    if(!Shell_NotifyIconW(NIM_MODIFY, &nid))
      throw runtime_error("Unable to update tray icon.");
    return;
  }

  // add tray icon
  const unsigned sleep_secs=1, max_sleep_secs=60;
  for(unsigned slept_secs=0; slept_secs<max_sleep_secs; slept_secs+=sleep_secs)
  {
    // add tray icon
    if(Shell_NotifyIconW(NIM_ADD, &nid))
      return;
    
    // on failure, sleep and retry
    logger::warnf("Unable to add tray icon, sleeping %u seconds.", sleep_secs);
    Sleep(sleep_secs*1000);
  }
  throw runtime_error("Unable to add tray icon.");
}
//----

void gui::update_hotkey()
{
  const hotkey hk=m_colibri.get_hotkey();
  m_topaz_set_hotkey(hk.ctrl, hk.alt, hk.shift, hk.lwin, hk.rwin, hk.vk);
}
//----

void gui::relayout()
{
  enum {VGAP=-20, VGAP_LAST=10};

  // get brick metrics
  const unsigned brick_width=m_theme->get_brick_width();
  const unsigned brick_height=m_theme->get_brick_height();

  // layout dimmer
  SetWindowPos(m_dimmer, HWND_TOPMOST, 0, 0, m_rc_monitor.right-m_rc_monitor.left, m_rc_monitor.bottom-m_rc_monitor.top, SWP_NOACTIVATE);

  // layout brick
  unsigned x=m_rc_monitor.left+(m_rc_monitor.right-m_rc_monitor.left-brick_width)/2;
  unsigned y=m_rc_monitor.top+(m_rc_monitor.bottom-m_rc_monitor.top-brick_height)/2 - (m_bricks.size()-1)*(brick_height+VGAP)/2;
  for(ptr_deque<brick>::reverse_iterator iter=m_bricks.rbegin(); iter!=m_bricks.rend(); ++iter)
  {
    SetWindowPos(iter->handle, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE);
    y+=brick_height+VGAP;
    repaint(*iter);
  }
  y+=VGAP_LAST;

  // layout dropdown
  SetWindowPos(m_dropdown, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE);
  repaint_dropdown();
}
//----

void gui::repaint(brick &brick)
{
  // create GDI+ graphics context
  dc dc(m_theme->get_brick_width(), m_theme->get_brick_height());
  Gdiplus::Graphics graphics(dc.get_dc());
  if(Gdiplus::Ok!=graphics.GetLastStatus())
    throw runtime_error("Unable to initialize GDI+ Graphics object for repainting brick.");

  // theme brick
  m_theme->paint_brick(graphics, brick, m_colibri);

  // update layered window
  dc.update(brick.handle);
}
//----

void gui::repaint_dropdown()
{
  // create GDI+ graphics context
  dc dc(m_theme->get_dropdown_width(), m_theme->get_dropdown_height());
  Gdiplus::Graphics graphics(dc.get_dc());
  if(Gdiplus::Ok!=graphics.GetLastStatus())
    throw runtime_error("Unable to initialize GDI+ Graphics object for repainting dropdown.");

  // theme brick
  m_theme->paint_dropdown(graphics, get_current_brick());

  // update layered window
  dc.update(m_dropdown);
}
//----------------------------------------------------------------------------


//============================================================================
// gui::option
//============================================================================
gui::option::option(const wstring &title, const wstring &description, const ::icon_info &icon_info, boost::uint64_t data)
  :title(title)
  ,description(description)
  ,icon_info(icon_info)
  ,data(data)
  ,has_arrow_overlay(false)
{
}
//----------------------------------------------------------------------------


//============================================================================
// theme
//============================================================================
theme::theme(const wstring &name)
{
  // load default font family
  HFONT defaultFont=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
  LOGFONTW logFont;
  GetObjectW(defaultFont, sizeof(LOGFONTW), &logFont);
  m_default_font=logFont.lfFaceName;

  // load default theme
  wstring errors;
  if(!load(L"default", errors))
    throw_errorf("Unable to load default theme:\n\n%S", errors.c_str());

  // load specific theme
  if(name!=L"default" && !load(name, errors))
  {
    logger::errorf("Unable to load '%S' theme:\n\n%S", name.c_str(), errors.c_str());
    MessageBoxW(0, errors.c_str(), (L"Unable to load '"+name+L"' theme").c_str(), MB_OK|MB_ICONERROR);
  }
}
//----------------------------------------------------------------------------

bool theme::load(const wstring &name, wstring &errors)
{
  errors.clear();
  logger::infof("Loading theme '%S'...", name.c_str());

  // determine INI file path
  wstring ini;
  if(!get_ini(name, ini))
  {
    errors+=name+L" not found\n";
    return false;
  }
  set_theme_for_icons_hack(name);

  // brick
  load_theme_image(name, L"brick.png", m_brick, errors);

  // brick title
  load_theme_font(ini.c_str(), L"brick_title", L"font", m_brick_title_font, errors);
  load_theme_brush(ini.c_str(), L"brick_title", L"color", m_brick_title_brush, errors);
  load_theme_rect(ini.c_str(), L"brick_title", L"rect", m_brick_title_rect, errors);

  // brick hint
  load_theme_font(ini.c_str(), L"brick_hint", L"font", m_brick_hint_font, errors);
  load_theme_brush(ini.c_str(), L"brick_hint", L"color", m_brick_hint_brush, errors);
  load_theme_rect(ini.c_str(), L"brick_hint", L"rect", m_brick_hint_rect, errors);

  // brick icon
  load_theme_pos(ini.c_str(), L"brick_icon", L"pos", m_brick_icon_pos, errors);
  load_theme_image(name, L"brick_icon_text_default.png", m_default_text_brick_icon, errors);
  load_theme_image(name, L"brick_icon_hotkey.png", m_hotkey_brick_icon, errors);

  // load overlay icons
  load_theme_image(name, L"arrow_overlay_32.png", m_arrow_overlay_32, errors);
  load_theme_image(name, L"arrow_overlay_48.png", m_arrow_overlay_48, errors);

  // credits
  load_theme_image(name, L"brick_icon_credits.png", m_credits_brick_icon, errors);
  load_theme_font(ini.c_str(), L"brick_credits", L"font1", m_credits_font_1, errors);
  load_theme_font(ini.c_str(), L"brick_credits", L"font2", m_credits_font_2, errors);
  load_theme_font(ini.c_str(), L"brick_credits", L"font3", m_credits_font_3, errors);
  load_theme_font(ini.c_str(), L"brick_credits", L"font4", m_credits_font_4, errors);
  load_theme_brush(ini.c_str(), L"brick_credits", L"color", m_credits_brush, errors);
  load_theme_rect(ini.c_str(), L"brick_credits", L"rect", m_credits_rect, errors);

  // dropdown
  load_theme_int(ini.c_str(), L"dropdown", L"height", m_dropdown_height, errors);

  // dropdown header
  load_theme_image(name, L"dropdown_header.png", m_dropdown_header, errors);
  load_theme_font(ini.c_str(), L"dropdown_header", L"font", m_dropdown_header_text_font, errors);
  load_theme_brush(ini.c_str(), L"dropdown_header", L"color", m_dropdown_header_text_brush, errors);
  load_theme_rect(ini.c_str(), L"dropdown_header", L"rect", m_dropdown_header_text_rect, errors);

  // dropdown row
  load_theme_image(name, L"dropdown_row.png", m_dropdown_row, errors);
  load_theme_image(name, L"dropdown_row_active.png", m_dropdown_row_active, errors);
  load_theme_pos(ini.c_str(), L"dropdown_row", L"icon_pos", m_dropdown_row_icon_pos, errors);
  load_theme_font(ini.c_str(), L"dropdown_row", L"title_font", m_dropdown_row_title_font, errors);
  load_theme_font(ini.c_str(), L"dropdown_row", L"description_font", m_dropdown_row_description_font, errors);
  load_theme_brush(ini.c_str(), L"dropdown_row", L"title_color", m_dropdown_row_title_brush, errors);
  load_theme_brush(ini.c_str(), L"dropdown_row", L"description_color", m_dropdown_row_description_brush, errors);
  load_theme_rect(ini.c_str(), L"dropdown_row", L"title_rect", m_dropdown_row_title_rect, errors);
  load_theme_rect(ini.c_str(), L"dropdown_row", L"description_rect", m_dropdown_row_description_rect, errors);

  // dropdown footer
  load_theme_image(name, L"dropdown_footer.png", m_dropdown_footer, errors);
  load_theme_font(ini.c_str(), L"dropdown_footer", L"font", m_dropdown_footer_text_font, errors);
  load_theme_brush(ini.c_str(), L"dropdown_footer", L"color", m_dropdown_footer_text_brush, errors);
  load_theme_rect(ini.c_str(), L"dropdown_footer", L"rect", m_dropdown_footer_text_rect, errors);

  // splash screen
  load_theme_image(name, L"splash_screen.png", m_splash_screen, errors);

  // early out?
  if(!errors.empty())
    return false;

  // dependant values
  m_brick_width=m_brick->GetWidth();
  m_brick_height=m_brick->GetHeight();
  m_dropdown_width=m_dropdown_row->GetWidth();
  m_dropdown_row_height=m_dropdown_row->GetHeight();
  m_dropdown_page_height=m_dropdown_height - m_dropdown_header->GetHeight() - m_dropdown_footer->GetHeight();
  m_dropdown_rows_per_page=m_dropdown_page_height / m_dropdown_row_height;
  m_splash_screen_width=m_splash_screen->GetWidth();
  m_splash_screen_height=m_splash_screen->GetHeight();
  return true;
}
//----

void theme::load_about(const std::wstring &name, std::wstring &title, std::wstring &description, std::wstring &author)
{
  // determine INI file path
  wstring ini;
  if(!get_ini(name, ini))
    throw_errorf("Theme '%S' not found.", name.c_str());

  // load about
  wstring errors;
  get(ini, L"about", L"title", title, errors);
  get(ini, L"about", L"description", description, errors);
  get(ini, L"about", L"author", author, errors);
  if(errors.size())
    throw_errorf("Missing and/or malformed meta information in theme '%S': %S", name.c_str(), errors.c_str());
}
//----------------------------------------------------------------------------

unsigned theme::get_brick_width() const
{
  return m_brick_width;
}
//----

unsigned theme::get_brick_height() const
{
  return m_brick_height;
}
//----

unsigned theme::get_dropdown_width() const
{
  return m_dropdown_width;
}
//----

unsigned theme::get_dropdown_height() const
{
  return m_dropdown_height;
}
//----

unsigned theme::get_dropdown_rows_per_page() const
{
  return m_dropdown_rows_per_page;
}
//----

unsigned theme::get_splash_screen_width() const
{
  return m_splash_screen_width;
}
//----

unsigned theme::get_splash_screen_height() const
{
  return m_splash_screen_height;
}
//----------------------------------------------------------------------------

void theme::paint_brick(Gdiplus::Graphics &graphics, gui::brick &brick, colibri_plugin &colibri)
{
  // left format (title etc.)
  Gdiplus::StringFormat leftFormat;
  leftFormat.SetAlignment(Gdiplus::StringAlignmentNear);
  leftFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);
  leftFormat.SetHotkeyPrefix(Gdiplus::HotkeyPrefixShow);
  leftFormat.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);

  // center format (credits etc.)
  Gdiplus::StringFormat centerFormat(&leftFormat);
  centerFormat.SetAlignment(Gdiplus::StringAlignmentCenter);

  // start with background
  graphics.DrawImage(m_brick, 0, 0, m_brick_width, m_brick_height);

  // render content
  switch(brick.type)
  {
  case gui::brick_type_option:
    if(brick.active_option_index<brick.options.size())
    {
      // render icon with overlay
      const gui::option &option=brick.options[brick.active_option_index];
      graphics.DrawImage(load_icon(option.icon_info)->icon_48.get(), m_brick_icon_pos.X, m_brick_icon_pos.Y, 48, 48);
      if(option.has_arrow_overlay)
        graphics.DrawImage(m_arrow_overlay_48, m_brick_icon_pos.X, m_brick_icon_pos.Y, 48, 48);

      // render title
      graphics.DrawString(option.title.c_str(), (INT)option.title.size(), m_brick_title_font.get(), m_brick_title_rect, &leftFormat, m_brick_title_brush.get());
    }
    else if(!brick.input.size())
    {
      // render hint
      graphics.DrawString(L"Enter search term", -1, m_brick_hint_font.get(), m_brick_hint_rect, &centerFormat, m_brick_hint_brush.get());
    }
    break;

  case gui::brick_type_text:
    // render icon
    graphics.DrawImage(brick.custom_icon ? load_icon(*brick.custom_icon)->icon_48.get() : m_default_text_brick_icon, m_brick_icon_pos.X, m_brick_icon_pos.Y, 48, 48);

    // render text
    graphics.DrawString((brick.input+L'_').c_str(), -1, m_brick_title_font.get(), m_brick_title_rect, &leftFormat, m_brick_title_brush.get());
    break;

  case gui::brick_type_hotkey:
    // render icon
    graphics.DrawImage(m_hotkey_brick_icon, m_brick_icon_pos.X, m_brick_icon_pos.Y, 48, 48);

    // render text
    graphics.DrawString(str(colibri.get_hotkey()).c_str(), -1, m_brick_title_font.get(), m_brick_title_rect, &leftFormat, m_brick_title_brush.get());
    break;

  case gui::brick_type_credits:
    // update animation stats
    static const DWORD FRAME_MSECS=4000;
    const DWORD dt=GetTickCount()-brick.credits_start_ticks;

    // render icon
    graphics.DrawImage(m_credits_brick_icon, m_brick_icon_pos.X, m_brick_icon_pos.Y, 48, 48);

    // render text
    if(dt<FRAME_MSECS)
      graphics.DrawString(str(colibri_version()).c_str(), -1, m_brick_title_font.get(), m_credits_rect, &centerFormat, m_brick_title_brush.get());
    else if(dt<2*FRAME_MSECS)
      graphics.DrawString(L"\u00a9 2005-2007,\nMichael Walter", -1, m_brick_title_font.get(), m_credits_rect, &centerFormat, m_brick_title_brush.get());
    else if(dt<3*FRAME_MSECS)
      graphics.DrawString(L"With contributions by Torsten Schröder.", -1, m_brick_title_font.get(), m_credits_rect, &centerFormat, m_brick_title_brush.get());
    else if(dt<4*FRAME_MSECS)
      graphics.DrawString(L"Logo and splash screen are designed by Tobias Ehls.", -1, m_brick_title_font.get(), m_credits_rect, &centerFormat, m_brick_title_brush.get());
    else if(dt<5*FRAME_MSECS)
      graphics.DrawString(L"Colibri is in part based on Crystal Clear.", -1, m_brick_title_font.get(), m_credits_rect, &centerFormat, m_brick_title_brush.get());
    else if(dt>=7*FRAME_MSECS)
      brick.credits_start_ticks=GetTickCount();
    break;
  }
}
//----

void theme::paint_dropdown(Gdiplus::Graphics &graphics, gui::brick &brick)
{
  // scroll up/down to fully show current option
  const int relativeY=int(brick.active_option_index*m_dropdown_row_height-brick.y_scroll_amount);
  const int maxY=m_dropdown_page_height-m_dropdown_row_height;
  if(relativeY<0)
    brick.y_scroll_amount+=relativeY;
  else if(relativeY>maxY)
    brick.y_scroll_amount+=relativeY-maxY;

  // left format (title etc.)
  Gdiplus::StringFormat leftFormat;
  leftFormat.SetAlignment(Gdiplus::StringAlignmentNear);
  leftFormat.SetLineAlignment(Gdiplus::StringAlignmentNear);
  leftFormat.SetHotkeyPrefix(Gdiplus::HotkeyPrefixShow);
  leftFormat.SetTrimming(Gdiplus::StringTrimmingEllipsisPath);
  leftFormat.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);

  // render header
  graphics.DrawImage(m_dropdown_header, 0, 0, m_dropdown_header->GetWidth(), m_dropdown_header->GetHeight());
  graphics.DrawString(brick.input.c_str(), -1, m_dropdown_header_text_font.get(), m_dropdown_header_text_rect, &leftFormat, m_dropdown_header_text_brush.get());

  // clip dropdown area
  const int width=m_dropdown_row->GetWidth(), height=m_dropdown_row->GetHeight();
  const int y_footer=int(m_dropdown_height-m_dropdown_footer->GetHeight());
  const Gdiplus::Rect clip_rect(0, m_dropdown_header->GetHeight(), width, y_footer-m_dropdown_header->GetHeight());
  graphics.SetClip(clip_rect, Gdiplus::CombineModeReplace);

  // render rows
  int y=int(m_dropdown_header->GetHeight()-brick.y_scroll_amount);
  for(vector<gui::option>::const_iterator iter=brick.options.begin(); y<y_footer && iter!=brick.options.end(); ++iter, y+=m_dropdown_row->GetHeight())
  {
    // skip completely invisible rows
    if(y+m_dropdown_row->GetHeight()<=m_dropdown_header->GetHeight())
      continue;

    // render background
    const bool isActive=brick.options.begin()+brick.active_option_index==iter;
    const Gdiplus::Rect dest(0, y, width, height);
    graphics.DrawImage(isActive ? m_dropdown_row_active : m_dropdown_row, dest, 0, 0, width, height, Gdiplus::UnitPixel);

#if 0
    // render selection checkbox
    const bool isSelected=false; //brick.selectedOptions.find(iter->data)!=brick.selectedOptions.end();
    blendRect(isSelected?m_dropdownCheckboxOn:m_dropdownCheckboxOff, 0, 0, 10, 10, composition, 19, y+13);
#endif

    // render icon with overlay
    graphics.DrawImage(load_icon(iter->icon_info)->icon_32.get(), m_dropdown_row_icon_pos.X, m_dropdown_row_icon_pos.Y+y, 32, 32);
    if(iter->has_arrow_overlay)
      graphics.DrawImage(m_arrow_overlay_32, m_dropdown_row_icon_pos.X, m_dropdown_row_icon_pos.Y+y, 32, 32);

    // render title and description
    Gdiplus::RectF rc=m_dropdown_row_title_rect;
    rc.Y+=y;
    graphics.DrawString(iter->title.c_str(), -1, m_dropdown_row_title_font.get(), rc, &leftFormat, m_dropdown_row_title_brush.get());
    rc=m_dropdown_row_description_rect;
    rc.Y+=y;
    graphics.DrawString(iter->description.c_str(), -1, m_dropdown_row_description_font.get(), rc, &leftFormat, m_dropdown_row_description_brush.get());
  }

  // fill remaining dropdown when there aren't enough items
  for(; y<y_footer; y+=m_dropdown_row_height)
  {
    // render background
    const Gdiplus::Rect dest(0, y, width, height);
    graphics.DrawImage(m_dropdown_row, dest, 0, 0, width, height, Gdiplus::UnitPixel);
  }

  // reset clipping
  graphics.ResetClip();

  // render footer
  graphics.DrawImage(m_dropdown_footer, 0, y_footer, m_dropdown_footer->GetWidth(), m_dropdown_footer->GetHeight());
  wostringstream stats;
  if(brick.active_option_index<brick.options.size())
    stats<<unsigned(brick.active_option_index+1)<<L" of "<<unsigned(brick.options.size());
  else
    stats<<unsigned(brick.options.size())<<L" Items";
  Gdiplus::RectF rc=m_dropdown_footer_text_rect;
  rc.Y+=y_footer;
  graphics.DrawString(stats.str().c_str(), -1, m_dropdown_footer_text_font.get(), rc, &leftFormat, m_dropdown_footer_text_brush.get());
}
//----

void theme::paint_splash_screen(Gdiplus::Graphics &graphics, ufloat1 progress, const wchar_t *text)
{
  // blit splash screen
  graphics.DrawImage(m_splash_screen, 0, 0, m_splash_screen_width, m_splash_screen_height);
}
//----------------------------------------------------------------------------

std::shared_ptr<icon> theme::load_icon(const icon_info &ii)
{ 
  return ::load_icon(ii, L"fallback");
}
//----------------------------------------------------------------------------

void theme::load_theme_image(const std::wstring &theme, const wchar_t *filename, Gdiplus::Image *&img, std::wstring &errors)
{
  try
  {
    img=load_image(install_folder() / L"themes" / theme / filename).get();
  }
  catch(std::exception &e)
  {
    const char *ascii=e.what();
    while(*ascii)
      errors+=wchar_t(*ascii++);
    errors+=L"\n";
    img=0;
  }
}
//----

void theme::load_theme_font(const std::wstring &ini, const wchar_t *section, const wchar_t *key, std::auto_ptr<Gdiplus::Font> &font_, std::wstring &errors)
{
  // read string
  wstring str;
  if(!get(ini, section, key, str, errors))
    return;

  // parse
  wistringstream in(str);
  wstring face;
  getline(in, face, L':');
  Gdiplus::REAL size=0;
  in>>size;

  // create font
  Gdiplus::Font *font=new Gdiplus::Font((face==L"Default" ? m_default_font : face).c_str(), size, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

  // check status
  if(Gdiplus::Ok!=font->GetLastStatus())
    errors+=L"Unable to load font: "+str+L"\n";
  else
    font_.reset(font);
}
//----

void theme::load_theme_brush(const std::wstring &ini, const wchar_t *section, const wchar_t *key, std::auto_ptr<Gdiplus::SolidBrush> &brush_, std::wstring &errors)
{
  // read string
  wstring str;
  if(!get(ini, section, key, str, errors))
    return;

  // parse
  wistringstream in(str);
  wchar_t c;
  unsigned color;
  in>>c>>hex>>color;

  // fix gdi+ bug
  if((color>>24)==0xff)
    color=(color&0x00ffffff)|0xfe000000;

  // create font
  Gdiplus::SolidBrush *b=new Gdiplus::SolidBrush(Gdiplus::Color(color));
  if(c!=L'#' || Gdiplus::Ok!=b->GetLastStatus())
    errors+=L"Unable to load brush with color: "+str+L"\n";
  else
    brush_.reset(b);
}
//----

void theme::load_theme_rect(const std::wstring &ini, const wchar_t *section, const wchar_t *key, Gdiplus::RectF &rc_, std::wstring &errors)
{
  // read string
  wstring str;
  if(!get(ini, section, key, str, errors))
    return;

  // parse
  wistringstream in(str);
  in>>rc_.X>>rc_.Y>>rc_.Width>>rc_.Height;
}
//----

void theme::load_theme_pos(const std::wstring &ini, const wchar_t *section, const wchar_t *key, Gdiplus::Point &pos_, std::wstring &errors)
{
  // read string
  wstring str;
  if(!get(ini, section, key, str, errors))
    return;

  // parse
  wistringstream in(str);
  in>>pos_.X>>pos_.Y;
}
//----

void theme::load_theme_int(const std::wstring &ini, const wchar_t *section, const wchar_t *key, unsigned &n_, std::wstring &errors)
{
  // read string
  wstring str;
  if(!get(ini, section, key, str, errors))
    return;

  // parse
  wistringstream in(str);
  in>>n_;
}
//----

bool theme::get_ini(const std::wstring &theme, wstring &ini)
{
  ini=(install_folder() / L"themes" / theme / L"/theme.ini").string();
  return boost::filesystem::is_regular(ini);
}
//----

bool theme::get(const std::wstring &ini, const wchar_t *section, const wchar_t *key, std::wstring &str, std::wstring &errors)
{
  // read .INI entry
  wchar_t buffer[1024]=L"";
  bool ok=0!=GetPrivateProfileStringW(section, key, 0, buffer, 1024, ini.c_str());
  str=buffer;

  // report error, if any
  if(!ok)
    errors+=L"Unable to read "+wstring(section)+L"."+key+L" in "+ini+L"\n";
  return ok;
}
//----------------------------------------------------------------------------
