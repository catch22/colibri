// Copyright (C) 2005, 2006, 2007 Michael Walter

#ifndef COLIBRI_CORE_DEFS_H
#define COLIBRI_CORE_DEFS_H

#include "../libraries/core/defs.h"
#include <string>
#include <boost/filesystem.hpp>
#include "../libraries/win32/win32.h"
#include "../libraries/win32/shell.h"


// typedefs
typedef float ufloat1;


// install_folder()
inline boost::filesystem::wpath install_folder()
{
  wchar_t path[MAX_PATH];
  GetModuleFileNameW(NULL, path, MAX_PATH);
  return boost::filesystem::wpath(path).branch_path().branch_path();
}


// profile_folder()
inline boost::filesystem::wpath profile_folder()
{
  // return if cached
  static boost::filesystem::wpath s_path;
  if(!s_path.empty())
    return s_path;

  // use %COLIBRI%/profile directory?
  s_path=install_folder() / L"profile";
  if(is_directory(s_path))
    return s_path;

  // use %APPDATA%/Colibri
  s_path=get_special_folder_path(special_folder_app_data);
  s_path/=L"\\Colibri";
  if(!is_directory(s_path))
    create_directory(s_path);
  return s_path;
}

#endif
