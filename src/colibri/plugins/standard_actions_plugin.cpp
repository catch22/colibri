//============================================================================
// standard_actions_plugin.cpp: Standard Actions plugin
//
// (c) Michael Walter, 2005-2007
//============================================================================

#include "standard_actions_plugin.h"
#include "../gui/gui.h"
#include "../libraries/win32/shell.h"
using namespace std;
//----------------------------------------------------------------------------


//============================================================================
// standard_actions_plugin
//============================================================================
const wchar_t *standard_actions_plugin::get_name() const
{
  return L"standard_actions";
}
//----

const wchar_t *standard_actions_plugin::get_title() const
{
  return L"Standard Actions";
}
//----------------------------------------------------------------------------

unsigned standard_actions_plugin::update_config(unsigned current_version)
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

void standard_actions_plugin::index(boost::uint64_t new_index_version)
{
  // add "Launch" action
  database_item item;
  item.plugin_id=get_name();
  item.item_id=L"Launch";
  item.title=L"Launch";
  item.description=L"Launch the application or document";
  item.is_transient=false;
  item.icon_info.set(icon_source_theme, L"standard_actions/launch");
  item.index_version=new_index_version;
  item.on_tab=L"standard_actions.launch";
  item.on_enter=L"standard_actions.launch";
  item.on_query_applicable=L"standard_actions.query_launchable";
  get_db().add_or_update_item(item);
}
//----------------------------------------------------------------------------

bool standard_actions_plugin::on_action(const std::wstring &name, boost::optional<database_item> target)
{
  if(L"standard_actions.launch"==name && target)
  {
    bool ok=target->path && launch(*target->path, target->launch_args);
    get_gui().hide();
    return ok;
  }
  else if(L"standard_actions.query_launchable"==name && target)
  {
    return target->path;
  }
  return false;
}
//----------------------------------------------------------------------------
