//============================================================================
// db_controller.cpp: Database controller
//
// (c) Michael Walter, 2005-2007
//============================================================================

#include "db_controller.h"
#include "db.h"
#include "../plugins/colibri_plugin.h"
#include "../libraries/win32/shell.h"
using namespace std;
//----------------------------------------------------------------------------

//============================================================================
// db_controller
//============================================================================
db_controller::db_controller(database &db, boost::optional<boost::uint64_t> parent_id)
  :m_db(db)
  ,m_parent_id(parent_id)
{
}
//----------------------------------------------------------------------------

bool db_controller::on_tab(gui &gui)
{
  const boost::optional<database_item> item=create_or_get_current_item(gui);
  if(!item)
  {
    // HACK: display last items
    if(!*gui.get_current_term())
    {
      gui.add_term_char(L' ');
      return true;
    }
    return false;
  }

  // redirect tab to currently selected action, if any
  if(const wstring *on_tab=item->on_tab.get_ptr())
    return m_db.trigger_action(*on_tab, (item->on_query_applicable && m_parent_id) ? m_db.get_item_for_id(*m_parent_id) : item);

  // otherwise, pop up new database item brick
  gui.push_option_brick(new db_controller(m_db, item->id));
  return true;
}
//----

bool db_controller::on_enter(gui &gui)
{
  const boost::optional<database_item> item=create_or_get_current_item(gui);
  if(!item)
    return false;

  // redirect enter to currently selected action, if any
  if(const wstring *on_enter=item->on_enter.get_ptr())
    return m_db.trigger_action(*on_enter, (item->on_query_applicable && m_parent_id) ? m_db.get_item_for_id(*m_parent_id) : item);

  // try to trigger default action
  database_result_set rs=m_db.search(L"", item->id);
  if(rs)
    if(const wstring *on_enter=rs.get_item().on_enter.get_ptr())
      return m_db.trigger_action(*on_enter, rs.get_item().on_query_applicable ? item : rs.get_item());
  return false;
}
//----

void db_controller::on_input_changed(gui &gui)
{
  vector<gui::option> options;
  m_items.clear();

  // perform search?
  bool all_have_arrow_overlay=true;
  if(*gui.get_current_term() || m_parent_id)
  {
    for(database_result_set rs=m_db.search(gui.get_current_term(), m_parent_id); rs; rs.next())
    {
      // remember item
      m_items[*rs.get_item().id]=rs.get_item();

      // add gui option
      gui::option option(rs.get_marked_up_title(), rs.get_item().description, rs.get_item().icon_info, *rs.get_item().id);
      if(!rs.get_item().path)
        option.has_arrow_overlay=true;
      else
        all_have_arrow_overlay=false;
      options.push_back(option);
    }
  }

  // remove arrow if all items have one
  if(all_have_arrow_overlay)
    for(vector<gui::option>::iterator iter=options.begin(); iter!=options.end(); ++iter)
      iter->has_arrow_overlay=false;

  // set options
  gui.set_options(options.begin(), options.end());
}
//----------------------------------------------------------------------------

bool db_controller::is_url(std::wstring &term)
{
  // everything with a protocol prefix is a URL
  if(0==_wcsnicmp(term.c_str(), L"http://", 7) ||
     0==_wcsnicmp(term.c_str(), L"mailto:", 7) ||
     0==_wcsnicmp(term.c_str(), L"www.", 4))
    return true;

  // everything without a backslash and with a dot is assumed to be a HTTP URL
  if(!wcschr(term.c_str(), L'\\') && wcschr(term.c_str(), L'.'))
  {
    term=L"http://"+term;
    return true;
  }

  return false;
}
//----

boost::optional<database_item> db_controller::create_or_get_current_item(gui &gui)
{
  // try to add item if none is selected
  if(!gui.get_current_option_data())
  {
    wstring term=gui.get_current_term();
    if(term.size()>0 && (is_url(term) || boost::filesystem::exists(term)))
    {
      // add item
      database_item item;
      item.plugin_id=L"DB Controller";
      item.item_id=term;
      item.title=term;
      item.description=get_display_name(term);
      item.is_transient=!m_db.get_colibri_plugin().are_custom_items_stored();
      item.icon_info.source=icon_source_shell;
      item.icon_info.path=term;
      item.path=term;
      m_db.add_item(item);

      // refresh UI (selects item)
      on_input_changed(gui);
    }       
  }

  // update timestamp if possible
  items::const_iterator iter=m_items.find(*gui.get_current_option_data());
  if(iter!=m_items.end())
  {
    m_db.update_history(*iter->second.id, gui.get_current_term());
    return iter->second;
  }
  return boost::none;
}
//----------------------------------------------------------------------------
