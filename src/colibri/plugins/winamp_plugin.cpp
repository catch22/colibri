//============================================================================
// winamp_plugin.cpp: Audio plugin
//
// (c) Torsten Schröder, 2006
//============================================================================

#include "winamp_plugin.h"
#include "../gui/gui.h"
#include "../gui/controller.h"
#include "winamp.h"
using namespace std;
//----------------------------------------------------------------------------


//============================================================================
// anonymous namespace
//============================================================================
namespace
{
  //==========================================================================
  // get_winamp()
  //==========================================================================
  HWND get_winamp()
  {
    return FindWindowW(L"Winamp v1.x", NULL);
  }
  //--------------------------------------------------------------------------


  //==========================================================================
  // is_currently_playing()
  //==========================================================================
  bool is_currently_playing(HWND winamp)
  {
    return 1==SendMessageW(winamp, WM_WA_IPC, 0, IPC_ISPLAYING);
  }
  //--------------------------------------------------------------------------


  //==========================================================================
  // is_shuffle_enabled()
  //==========================================================================
  bool is_shuffle_enabled(HWND winamp)
  {
    return 1==SendMessageW(winamp, WM_WA_IPC, 0, IPC_GET_SHUFFLE);
  }
}
//----------------------------------------------------------------------------


//============================================================================
// winamp_plugin
//============================================================================
winamp_plugin::winamp_plugin()
{
}
//----

winamp_plugin::~winamp_plugin()
{
}
//----------------------------------------------------------------------------

const wchar_t *winamp_plugin::get_name() const
{
  return L"winamp";
}
//----

const wchar_t *winamp_plugin::get_title() const
{
  return L"Winamp";
}
//----------------------------------------------------------------------------

unsigned winamp_plugin::update_config(unsigned current_version)
{
  switch(current_version)
  {
  case 0:
    // version 1 doesn't have setting
    ;
  }
  return 1;
}
//----------------------------------------------------------------------------

void winamp_plugin::index(boost::uint64_t new_index_version)
{
  // fresh install
  database_item item;
  item.plugin_id=get_name();
  item.item_id=L"Winamp";
  item.title=L"Winamp";
  item.description=L"Control Winamp";
  item.is_transient=false;
  item.icon_info.set(icon_source_theme, L"winamp/winamp");
  item.index_version=new_index_version;
  item.on_enter=L"winamp_actions.open_winamp_menu";
  item.on_tab=L"winamp_actions.open_winamp_menu";
  get_db().add_or_update_item(item);
}
//----------------------------------------------------------------------------

bool winamp_plugin::on_action(const std::wstring &name, boost::optional<database_item> target)
{
  // XXX: filter actions by plugin name
  static const std::wstring prefix(L"winamp_actions.");
  if(name.compare(0, prefix.length(), prefix)!=0)
    return false;

  // check whether winamp is running
  HWND winamp=get_winamp();
  if(!winamp)
    return false;

  // open winamp menu
  if(name==L"winamp_actions.open_winamp_menu")
  {
    // open winamp menu
    menu_controller *ctrl=new menu_controller(get_db());
    ctrl->add_item(L"Play", L"Play current song", L"winamp/play", L"winamp_actions.play");
    ctrl->add_item(L"Pause", L"Pause current song", L"winamp/pause", L"winamp_actions.pause");
    ctrl->add_item(L"Stop", L"Stop playback", L"winamp/stop", L"winamp_actions.stop");
    ctrl->add_item(L"Previous song", L"Go to previous song", L"winamp/previous_song", L"winamp_actions.previous_song");
    ctrl->add_item(L"Next song", L"Go to next song", L"winamp/next_song", L"winamp_actions.next_song");
    ctrl->add_checkbox_item(L"Shuffle enabled", L"Shuffle disabled", is_shuffle_enabled(winamp), L"winamp_actions.toggle_shuffle");
    get_gui().push_option_brick(ctrl);
    return true;
  }

  // play, pause, stop, mute, previous song, next song
  if(name==L"winamp_actions.play")
  {
    SendMessageW(winamp, WM_COMMAND, WINAMP_BUTTON2, 0);
    return true;
  }
  if(name==L"winamp_actions.pause")
  {
    SendMessageW(winamp, WM_COMMAND, WINAMP_BUTTON3, 0);
    return true;
  }
  if(name==L"winamp_actions.stop")
  {
    SendMessageW(winamp, WM_COMMAND, WINAMP_BUTTON4, 0);
    return true;
  }
  if(name==L"winamp_actions.previous_song")
  {
    SendMessageW(winamp, WM_COMMAND, WINAMP_BUTTON1, 0);
    return true;
  }
  if(name==L"winamp_actions.next_song")
  {
    SendMessageW(winamp, WM_COMMAND, WINAMP_BUTTON5, 0);
    return true;
  }
  if(name==L"winamp_actions.toggle_shuffle")
  {
    SendMessageW(winamp, WM_WA_IPC, !is_shuffle_enabled(winamp), IPC_SET_SHUFFLE);      
    return true;
  }
  return false;
}
//----------------------------------------------------------------------------

