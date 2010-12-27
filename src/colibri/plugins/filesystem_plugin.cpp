//============================================================================
// filesystem_plugin.cpp: Filesystem plugin
//
// (c) Michael Walter, 2005-2007
//============================================================================

#include "filesystem_plugin.h"
#include "../libraries/win32/shell.h"
#include "../libraries/log/log.h"
using namespace std;
using namespace boost;
//----------------------------------------------------------------------------


//============================================================================
// filesystem_plugin
//============================================================================
const wchar_t *filesystem_plugin::get_name() const
{
  return L"filesystem";
}
//----

const wchar_t *filesystem_plugin::get_title() const
{
  return L"Filesystem";
}
//----------------------------------------------------------------------------

unsigned filesystem_plugin::update_config(unsigned current_version)
{
  switch(current_version)
  {
  case 0:
    // fresh install
    get_config().prepare(L"CREATE TABLE folders (path TEXT UNIQUE)")->exec();
    std::shared_ptr<sqlite_statement> stmt=get_config().prepare(L"INSERT INTO folders (path) VALUES (?)");
    stmt->bind(0, get_special_folder_path(special_folder_startmenu));
    stmt->exec();
    stmt->bind(0, get_special_folder_path(special_folder_common_startmenu));
    stmt->exec();
    stmt->bind(0, get_special_folder_path(special_folder_quick_launch));
    stmt->exec();
  }
  return 1;
}
//----------------------------------------------------------------------------

void filesystem_plugin::index(boost::uint64_t new_index_version)
{
  // index all folders
  std::shared_ptr<sqlite_statement> stmt=get_config().prepare(L"SELECT path FROM folders");
  stmt->exec();
  while(*stmt)
  {
    wstring folder=stmt->get_string(0);
    index(get_db(), new_index_version, folder);
    stmt->next();
  }
}
//----------------------------------------------------------------------------

void filesystem_plugin::index(database &db, boost::uint64_t new_index_version, const wstring &folder)
{
  // find first file
  WIN32_FIND_DATAW wfd;
  HANDLE search=FindFirstFileExW((folder+L"\\*").c_str(), FindExInfoStandard, &wfd, FindExSearchNameMatch, 0, 0);
  if(INVALID_HANDLE_VALUE==search)
  {
    logger::warnf("unable to index directory: %S", folder.c_str());
    return;
  }

  // index all files
  do
  {
    // skip . and ..
    if(0==wcscmp(wfd.cFileName, L".") || 0==wcscmp(wfd.cFileName, L".."))
      continue;

    // skip hidden & system files
    if((FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM)&wfd.dwFileAttributes)
      continue;

    // create full path
    wstring path=folder;
    path+=L'\\';
    path+=wfd.cFileName;

    // recurse into sub-directories
    if(FILE_ATTRIBUTE_DIRECTORY&wfd.dwFileAttributes)
    {
      index(db, new_index_version, path);
      continue;
    }

    // skip Colibri link
    if(wcsstr(path.c_str(), L"Colibri.lnk"))
      continue;

    // add or update item
    database_item item;
    item.plugin_id=get_name();
    item.item_id=path;
    item.title=get_display_name(path);
    item.description=path;
    item.is_transient=false;
    item.icon_info.source=icon_source_shell;
    item.icon_info.path=path;
    item.index_version=new_index_version;
    item.path=path;
    db.add_or_update_item(item);
  }
  while(FindNextFileW(search, &wfd));
  FindClose(search);
}
//----------------------------------------------------------------------------
