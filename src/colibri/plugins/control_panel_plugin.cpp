//============================================================================
// control_panel_plugin.h: Control panel plugin
//
// (c) Michael Walter, 2006
//============================================================================

#include "control_panel_plugin.h"
#include "../libraries/core/dynlib.h"
#include "../libraries/win32/shell.h"
#include "../libraries/log/log.h"
#include <sstream>
using namespace std;
using namespace boost;
//----------------------------------------------------------------------------


//============================================================================
// control_panel_plugin
//============================================================================
const wchar_t *control_panel_plugin::get_name() const
{
  return L"control_panel";
}
//----

const wchar_t *control_panel_plugin::get_title() const
{
  return L"Control Panel";
}
//----------------------------------------------------------------------------

unsigned control_panel_plugin::update_config(unsigned current_version)
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

void control_panel_plugin::index(boost::uint64_t new_index_version)
{
  // enumerate control panel applets in $WINDOWS/SYSTEM32
  const wstring system32=get_special_folder_path(special_folder_system)+L"\\";
  WIN32_FIND_DATAW wfd;
  HANDLE handle=FindFirstFileW((system32+L"*.cpl").c_str(), &wfd);
  if(INVALID_HANDLE_VALUE!=handle)
  {
    do
      index(get_db(), new_index_version, (system32+wfd.cFileName).c_str());
    while(FindNextFileW(handle, &wfd));
  }

  // enumerate control panel applets in registry
  const HKEY roots[]={HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER};
  const wchar_t *sub_key=L"Software\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cpls";
  for(unsigned i=0; i<sizeof(roots)/sizeof(roots[0]); ++i)
  {
    // open registry key
    HKEY key;
    if(ERROR_SUCCESS!=RegOpenKeyW(roots[i], sub_key, &key))
      continue;

    // query values
    DWORD type;
    wchar_t name[MAX_PATH];
    wchar_t data[MAX_PATH];
    for(DWORD i=0;; ++i)
    {
      // fetch next value
      DWORD name_size=MAX_PATH, data_size=MAX_PATH;
      const LONG res=RegEnumValueW(key, i, name, &name_size, 0, &type, (LPBYTE)data, &data_size);
      if(ERROR_NO_MORE_ITEMS==res)
        break;
      else if(ERROR_SUCCESS!=res)
        continue;

      // register applet
      if(REG_SZ==type)
        index(get_db(), new_index_version, data);
    }

    // cleanup
    RegCloseKey(key);
  }
}
//----------------------------------------------------------------------------

void control_panel_plugin::index(database &db, boost::uint64_t new_index_version, const wstring &path)
{
  // open library
  std::shared_ptr<dynamic_library> lib;
  try
  {
    lib.reset(new dynamic_library(path));
  }
  catch(std::exception&)
  {
    logger::warnf("Unable to load control panel appel '%S'", path.c_str());
    return;
  }

  // get entry point
  typedef LONG CALLBACK cpl_applet(HWND, UINT, LPARAM, LPARAM);
  cpl_applet *applet=lib->try_lookup<cpl_applet>("CPlApplet");
  if(!applet)
  {
    logger::warnf("Unable to get control panel applet entry point '%S'", path.c_str());
    return;
  }

  // initialize applet
  if(!applet(0, CPL_INIT, 0, 0))
    return;

  // register all dialogs
  const LONG applet_count=applet(0, CPL_GETCOUNT, 0, 0);
  for(LONG i=0; i<applet_count; ++i)
  {
    database_item item;

    // load old-style control panel info
    CPLINFO info;
    applet(0, CPL_INQUIRE, i, reinterpret_cast<LPARAM>(&info));
    if(CPL_DYNAMIC_RES!=info.idIcon && CPL_DYNAMIC_RES!=info.idName && CPL_DYNAMIC_RES!=info.idInfo)
    {
      // load title and description
      wchar_t name[MAX_PATH], description[MAX_PATH];
      LoadStringW(lib->get_hmodule(), info.idName, name, MAX_PATH);
      LoadStringW(lib->get_hmodule(), info.idInfo, description, MAX_PATH);
      item.title=name;
      item.description=description;

      // load icon from executable resources
      item.icon_info.source=icon_source_resource;
      wostringstream os;
      os<<path<<"#"<<info.idIcon;
      item.icon_info.path=os.str();
    }

    // load new-style control panel info
    if(item.title.empty())
    {
      union {NEWCPLINFOA ansi; NEWCPLINFOW unicode;} info;
      info.ansi.dwSize=info.ansi.dwFlags=0;
      applet(0, CPL_NEWINQUIRE, i, reinterpret_cast<LPARAM>(&info));
      if(info.unicode.dwSize==sizeof(NEWCPLINFOW))
      {
        // set title and description
        item.title=info.unicode.szName;
        item.description=info.unicode.szInfo;

        // load icon by querying applet
        item.icon_info.source=icon_source_new_style_cpl_applet;
        wostringstream os;
        os<<path<<"#"<<i;
        item.icon_info.path=os.str();
      }
      else if(info.ansi.dwSize==sizeof(NEWCPLINFOA))
      {
        // set title and description
        item.title=wstring(info.ansi.szName, info.ansi.szName+strlen(info.ansi.szName));
        item.description=wstring(info.ansi.szInfo, info.ansi.szInfo+strlen(info.ansi.szInfo));

        // load icon by querying applet
        item.icon_info.source=icon_source_new_style_cpl_applet;
        wostringstream os;
        os<<path<<"#"<<i;
        item.icon_info.path=os.str();
      }
    }
    if(item.title.empty())
      continue;

    // add item
    item.plugin_id=get_name();
    item.item_id=item.title;
    item.is_transient=false;
    item.index_version=new_index_version;
    wostringstream os;
    item.path=L"Control.exe";
    os<<L"\"";
    os<<path;
    os<<L"\",@";
    os<<i;
    item.launch_args=os.str();
    db.add_or_update_item(item);
  }

  // cleanup
  applet(0, CPL_EXIT, 0, 0);
}
//----------------------------------------------------------------------------
