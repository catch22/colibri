//============================================================================
// shell.cpp: Shell-related functionality
//
// (c) Michael Walter, 2006
//============================================================================

#include "shell.h"
#include "win32.h"
#include <shlobj.h>
using namespace std;
//----------------------------------------------------------------------------


//============================================================================
// get_special_folder_path()
//============================================================================
std::wstring get_special_folder_path(e_special_folder sf)
{
  // map enumeration to csidl
  static int s_csidl[]=
  {
    CSIDL_STARTMENU,
    CSIDL_COMMON_STARTMENU,
    CSIDL_SYSTEM,
    CSIDL_APPDATA,
    CSIDL_APPDATA,
  };

  // get shell malloc interface
  IMalloc *malloc=0;
  if(FAILED(SHGetMalloc(&malloc)))
    throw runtime_error("Unable to get shell malloc interface.");

  // get special folder location
  ITEMIDLIST *id_list=0;
  if(FAILED(SHGetFolderLocation(0, s_csidl[sf], 0, 0, &id_list)))
  {
    malloc->Release();
    throw_errorf("Unable to get location of special folder '%S'.", str(sf));
  }

  // convert id list to path
  wchar_t path[MAX_PATH];
  if(!SHGetPathFromIDListW(id_list, path))
  {
    malloc->Free(id_list);
    malloc->Release();
    throw runtime_error("Unable to translate item id list to filesystem path.");
  }

  // cleanup
  malloc->Free(id_list);
  malloc->Release();

  // append magic?
  if(special_folder_quick_launch==sf)
    wcscat(path, L"\\Microsoft\\Internet Explorer\\Quick Launch");
  return path;
}
//----------------------------------------------------------------------------


//============================================================================
// get_display_name()
//============================================================================
std::wstring get_display_name(const std::wstring &filename)
{
  // get file information
  SHFILEINFOW info;
  if(!SHGetFileInfoW(filename.c_str(), FILE_ATTRIBUTE_NORMAL, &info, sizeof(SHFILEINFOW), SHGFI_DISPLAYNAME|SHGFI_USEFILEATTRIBUTES))
    throw_errorf("Unable to get file information for '%S'", filename.c_str());
  return info.szDisplayName;
}
//----------------------------------------------------------------------------


//============================================================================
// launch()
//============================================================================
bool launch(const std::wstring &object, boost::optional<std::wstring> args)
{
  SHELLEXECUTEINFOW info;
  info.cbSize=sizeof(SHELLEXECUTEINFOW);
  info.fMask=0;
  info.hwnd=0;
  info.lpVerb=0;
  info.lpFile=object.c_str();
  info.lpParameters=args ? args->c_str() : 0;
  info.lpDirectory=0;
  info.nShow=SW_SHOWDEFAULT;
  info.hInstApp=0;
  return 0!=ShellExecuteExW(&info);
}
//----------------------------------------------------------------------------


//============================================================================
// str()
//============================================================================
const wchar_t *str(e_special_folder sf)
{
  switch(sf)
  {
  case special_folder_startmenu:         return L"startmenu";
  case special_folder_common_startmenu:  return L"common_startmenu";
  case special_folder_app_data:          return L"app_data";
  case special_folder_quick_launch:      return L"quick_launch";
  default:                               return L"unknown";
  }
}
//----------------------------------------------------------------------------
