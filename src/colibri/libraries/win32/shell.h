//============================================================================
// shell.h: Shell-related functionality
//
// (c) Michael Walter, 2006
//============================================================================

#ifndef UTILS_WIN32_SHELL_H
#define UTILS_WIN32_SHELL_H
#include "../core/defs.h"
#include <boost/optional.hpp>
#include <boost/none.hpp>
//----------------------------------------------------------------------------

// Interface:
enum e_special_folder;
std::wstring get_special_folder_path(e_special_folder);
std::wstring get_display_name(const std::wstring &filename);
bool launch(const std::wstring &object, boost::optional<std::wstring> args=boost::none);
//----------------------------------------------------------------------------


//============================================================================
// e_special_folder
//============================================================================
enum e_special_folder
{
  special_folder_startmenu=0,
  special_folder_common_startmenu,
  special_folder_system,
  special_folder_app_data,
  special_folder_quick_launch,
  special_folder_count
};
//----

const wchar_t *str(e_special_folder);
//----------------------------------------------------------------------------

#endif