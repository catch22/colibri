//============================================================================
// db.cpp: Database
//
// (c) Michael Walter, 2005-2007
//============================================================================

#include "db.h"
#include "match.h"
#include "../plugins/colibri_plugin.h"
#include "../libraries/log/log.h"
#include "../thirdparty/sqlite/sqlite3.h"
using namespace std;
using namespace boost;
//----------------------------------------------------------------------------


//============================================================================
// anonymous namespace
//============================================================================
namespace
{
  //==========================================================================
  // global state
  //==========================================================================
  database *g_database;
  wstring g_current_term;
  database_item g_current_parent_item;
  //--------------------------------------------------------------------------


  //==========================================================================
  // COLIBRI_HISTORY_SCORE (relative common prefix length)
  //==========================================================================
  void colibri_history_score(sqlite3_context *context, int, sqlite3_value **args)
  {
    // no search or reference term? classify all strings equal
    const wchar_t *history_term=reinterpret_cast<const wchar_t*>(sqlite3_value_text16le(args[0]));
    if(!g_current_term.size() || !history_term)
    {
      sqlite3_result_double(context, 0);
      return;
    }

    // count common prefix length
    const wchar_t *current_term=g_current_term.c_str();
    unsigned length=0;
    while(*current_term && *history_term && *current_term==*history_term)
      ++current_term, ++history_term, ++length;

    // score := common prefix length / term length
    sqlite3_result_double(context, double(length) / g_current_term.size());
  }
  //--------------------------------------------------------------------------


  //==========================================================================
  // COLIBRI_MATCH_SCORE (match() score)
  //==========================================================================
  void colibri_match_score(sqlite3_context *context, int, sqlite3_value **args)
  {
    // no search term? classify all strings equal
    if(!g_current_term.size())
    {
      sqlite3_result_double(context, 0);
      return;
    }

    // match
    float score;
    wstring marked_up_string;
    const wchar_t *string=reinterpret_cast<const wchar_t*>(sqlite3_value_text16le(args[0]));
    match(g_current_term.c_str(), string, score, marked_up_string);
    sqlite3_result_double(context, score);
  }
  //--------------------------------------------------------------------------


  //==========================================================================
  // COLIBRI_MATCH_MARKED_UP_STRING (match() marked_up_string)
  //==========================================================================
  void colibri_match_marked_up_string(sqlite3_context *context, int, sqlite3_value **args)
  {
    // no search term? classify all strings equal
    const wchar_t *string=reinterpret_cast<const wchar_t*>(sqlite3_value_text16le(args[0]));
    if(!g_current_term.size())
    {
      sqlite3_result_text16le(context, string, -1, SQLITE_TRANSIENT);
      return;
    }

    // match
    float score;
    wstring marked_up_string;
    match(g_current_term.c_str(), string, score, marked_up_string);
    sqlite3_result_text16le(context, marked_up_string.c_str(), static_cast<int>(marked_up_string.size()*2), SQLITE_TRANSIENT);
  }
  //--------------------------------------------------------------------------


  //==========================================================================
  // COLIBRI_TRIGGER_ACTION (trigger action on current parent item)
  //==========================================================================
  void colibri_trigger_action(sqlite3_context *context, int, sqlite3_value **args)
  {
    // trigger action on current parent item
    const wchar_t *action=reinterpret_cast<const wchar_t*>(sqlite3_value_text16le(args[0]));
    const bool action_supported=g_database->trigger_action(action, g_current_parent_item);

    // return success
    sqlite3_result_int(context, action_supported ? 1 : 0);
  }
}
//----------------------------------------------------------------------------


//============================================================================
// database_result_set
//============================================================================
database_result_set::database_result_set(std::shared_ptr<sqlite_statement> stmt)
  :m_stmt(stmt)
{
  next();
}
//----------------------------------------------------------------------------

database_result_set::operator const void*() const
{
  return *m_stmt;
}
//----

void database_result_set::next()
{
  m_stmt->next();
  if(!*m_stmt)
    return;

  // fill mandatory fields
  m_item.id=m_stmt->get_uint64(0);
  m_item.plugin_id=m_stmt->get_string(1);
  m_item.item_id=m_stmt->get_string(2);
  m_item.title=m_stmt->get_string(3);
  m_item.description=m_stmt->get_string(4);
  m_item.is_transient=m_stmt->get_bool(5);
  m_item.icon_info.source=static_cast<e_icon_source>(m_stmt->get_unsigned(6));
  m_item.icon_info.path=m_stmt->get_string(7);
  m_item.index_version=m_stmt->get_uint64_option(8);

  // fill facet fields
  m_item.parent_id=m_stmt->get_uint64_option(9);
  m_item.path=m_stmt->get_string_option(10);
  m_item.launch_args=m_stmt->get_string_option(11);
  m_item.on_enter=m_stmt->get_string_option(12);
  m_item.on_tab=m_stmt->get_string_option(13);
  m_item.on_query_applicable=m_stmt->get_string_option(14);

  // fill search-only data
  m_history_score=m_stmt->get_float(15);
  m_match_score=m_stmt->get_float(16);
  m_marked_up_title=m_stmt->get_string(17);
}
//----------------------------------------------------------------------------

const database_item &database_result_set::get_item() const
{
  return m_item;
}
//----

float database_result_set::get_history_score() const
{
  return m_history_score;
}
//----

float database_result_set::get_match_score() const
{
  return m_match_score;
}
//----

const wstring &database_result_set::get_marked_up_title() const
{
  return m_marked_up_title;
}
//----------------------------------------------------------------------------


//============================================================================
// database
//============================================================================
database::database()
  :m_gui(0)
  ,m_db((profile_folder() / L"database.sqlite").string())
{
  // update database schema
  switch(unsigned version=m_db.begin_schema_update())
  {
  case 0:
    // fresh install
    m_db.prepare(L"CREATE TABLE IF NOT EXISTS items (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, plugin_id TEXT NOT NULL, item_id TEXT NOT NULL, title TEXT NOT NULL, description TEXT NOT NULL, is_transient INTEGER NOT NULL, icon_source INTEGER NOT NULL, icon_path TEXT NOT NULL, index_version INTEGER NULL, parent_id INTEGER NULL, path TEXT NULL, on_enter TEXT NULL, on_tab TEXT NULL, on_query_applicable TEXT NULL, UNIQUE (plugin_id, item_id))")->exec();
    m_db.prepare(L"CREATE TABLE IF NOT EXISTS item_history (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, item_id INTEGER NOT NULL, term TEXT NOT NULL, last_invokation TEXT NOT NULL, invokation_count TEXT NOT NULL, UNIQUE(item_id, term))")->exec();

  case 1:
    // add "launch_args" column
    m_db.prepare(L"ALTER TABLE items ADD COLUMN launch_args TEXT NULL")->exec();
  }
  m_db.end_schema_update(L"database", 2);

  // register custom functions
  g_database=this;
  m_db.reg_function(L"COLIBRI_HISTORY_SCORE", 1, colibri_history_score);
  m_db.reg_function(L"COLIBRI_MATCH_SCORE", 1, colibri_match_score);
  m_db.reg_function(L"COLIBRI_MATCH_MARKED_UP_STRING", 1, colibri_match_marked_up_string);
  m_db.reg_function(L"COLIBRI_TRIGGER_ACTION", 1, colibri_trigger_action);

  // prepare queries
  m_insert_query=m_db.prepare(L"INSERT INTO items (plugin_id, item_id, title, description, is_transient, icon_source, icon_path, index_version, parent_id, path, launch_args, on_enter, on_tab, on_query_applicable) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
  m_update_query=m_db.prepare(L"UPDATE items SET title = ?, description = ?, is_transient = ?, icon_source = ?, icon_path = ?, index_version = ?, parent_id = ?, path = ?, launch_args = ?, on_enter = ?, on_tab = ?, on_query_applicable = ? WHERE plugin_id = ? AND item_id = ?");
  m_delete_old_item_history_query=m_db.prepare(L"DELETE FROM item_history WHERE item_history.item_id IN (SELECT id FROM items WHERE plugin_id = ? AND index_version < ?)");
  m_delete_old_items_query=m_db.prepare(L"DELETE FROM items WHERE plugin_id = ? AND index_version < ?");
  m_delete_unindexed_item_history_query=m_db.prepare(L"DELETE FROM item_history WHERE item_history.item_id IN (SELECT id FROM items WHERE plugin_id = ? AND index_version IS NULL)");
  m_delete_unindexed_items_query=m_db.prepare(L"DELETE FROM items WHERE plugin_id = ? AND index_version IS NULL");

  // clear transient items
  m_db.prepare(L"DELETE FROM item_history WHERE item_history.item_id IN (SELECT id FROM items WHERE is_transient <> 0)")->exec();
  m_db.prepare(L"DELETE FROM items WHERE is_transient <> 0")->exec();
  logger::infof("Deleted %u transient items", m_db.get_num_affected_rows());
}
//----------------------------------------------------------------------------

void database::add_plugin(std::shared_ptr<plugin> plugin)
{
  // add plugin to list
  m_plugins.push_back(plugin);

  // initialize
  plugin->init(*this);
  if(m_gui)
    plugin->set_gui(*m_gui);
}
//----

plugin &database::get_plugin(const std::wstring &name)
{
  for(plugins::iterator iter=m_plugins.begin(); iter!=m_plugins.end(); ++iter)
    if((*iter)->get_name()==name)
      return **iter;
  throw_errorf("Plugin '%S' not found", name.c_str());
}
//----

colibri_plugin &database::get_colibri_plugin()
{
  return static_cast<colibri_plugin&>(get_plugin(L"colibri"));
}
//----

void database::set_gui(gui &gui)
{
  m_gui=&gui;
  for(plugins::iterator iter=m_plugins.begin(); iter!=m_plugins.end(); ++iter)
    (*iter)->set_gui(gui);
}
//----

void database::update_index()
{
  /*XXX*/
  for(plugins::iterator iter=m_plugins.begin(); iter!=m_plugins.end(); ++iter)
  {
    logger::infof("[%S] Updating index", (*iter)->get_name());
    m_db.prepare(L"BEGIN TRANSACTION")->exec();
    (*iter)->update_index();
    m_db.prepare(L"COMMIT TRANSACTION")->exec();
  }
}
//----------------------------------------------------------------------------

bool database::trigger_action(std::wstring name, boost::optional<database_item> target)
{
  bool ok=false;
  for(plugins::iterator iter=m_plugins.begin(); iter!=m_plugins.end(); ++iter)
    ok|=(*iter)->on_action(name, target);
  return ok;
}
//----------------------------------------------------------------------------

database_result_set database::search(const wstring &term, boost::optional<boost::uint64_t> parent_id)
{
  // store search term
  g_current_term=normalized_term(term);

  // search
  std::shared_ptr<sqlite_statement> query;
  if(boost::uint64_t *pid=parent_id.get_ptr())
  {
    // fetch parent item
    g_current_parent_item=get_item_for_id(*pid);

    // construct query
    query=m_db.prepare(L"SELECT items.id AS id, plugin_id, items.item_id AS item_id, title, description, is_transient, icon_source, icon_path, index_version, parent_id, path, launch_args, on_enter, on_tab, on_query_applicable, COLIBRI_HISTORY_SCORE(item_history.term) AS history_score, COLIBRI_MATCH_SCORE(title) AS match_score, COLIBRI_MATCH_MARKED_UP_STRING(title) AS marked_up_title FROM items OUTER LEFT JOIN item_history ON item_history.item_id = items.id WHERE (parent_id = ? OR (on_query_applicable IS NOT NULL AND COLIBRI_TRIGGER_ACTION(on_query_applicable))) AND match_score!=? GROUP BY items.id ORDER BY MAX(history_score) DESC, MAX(last_invokation), match_score DESC, title ASC, description ASC");
    query->bind(0, *pid);
    query->bind(1, score_not_found());
  }
  else
  {
    // construct query
    query=m_db.prepare(L"SELECT items.id AS id, plugin_id, items.item_id AS item_id, title, description, is_transient, icon_source, icon_path, index_version, parent_id, path, launch_args, on_enter, on_tab, on_query_applicable, COLIBRI_HISTORY_SCORE(item_history.term) AS history_score, COLIBRI_MATCH_SCORE(title) AS match_score, COLIBRI_MATCH_MARKED_UP_STRING(title) AS marked_up_title, item_history.last_invokation AS last_invokation FROM items OUTER LEFT JOIN item_history ON item_history.item_id = items.id WHERE on_query_applicable IS NULL AND match_score!=? GROUP BY items.id ORDER BY MAX(history_score) DESC, MAX(last_invokation) DESC, match_score DESC, title ASC, description ASC");
    query->bind(0, score_not_found());
  }
  return query;
}
//----

database_item database::get_item_for_id(boost::uint64_t id) const
{
  // fetch item data
  std::shared_ptr<sqlite_statement> query=const_cast<sqlite_connection&>(m_db).prepare(L"SELECT id, plugin_id, item_id, title, description, is_transient, icon_source, icon_path, index_version, parent_id, path, launch_args, on_enter, on_tab, on_query_applicable FROM items WHERE id = ?");
  query->bind(0, id);
  query->exec();
  if(!*query)
    throw_errorf("Item not found for id %lu", id);

  // fill item
  database_item item;
  item.id=query->get_uint64(0);
  item.plugin_id=query->get_string(1);
  item.item_id=query->get_string(2);
  item.title=query->get_string(3);
  item.description=query->get_string(4);
  item.is_transient=query->get_bool(5);
  item.icon_info.source=static_cast<e_icon_source>(query->get_unsigned(6));
  item.icon_info.path=query->get_string(7);
  item.index_version=query->get_uint64_option(8);
  item.parent_id=query->get_uint64_option(9);
  item.path=query->get_string_option(10);
  item.launch_args=query->get_string_option(11);
  item.on_enter=query->get_string_option(12);
  item.on_tab=query->get_string_option(13);
  item.on_query_applicable=query->get_string_option(14);
  return item;
}
//----

void database::add_item(const database_item &item)
{
  // bind values and execute
  m_insert_query->bind(0, item.plugin_id);
  m_insert_query->bind(1, item.item_id);
  m_insert_query->bind(2, item.title);
  m_insert_query->bind(3, item.description);
  m_insert_query->bind(4, item.is_transient);
  m_insert_query->bind(5, unsigned(item.icon_info.source));
  m_insert_query->bind(6, item.icon_info.path);
  m_insert_query->bind(7, item.index_version);
  m_insert_query->bind(8, item.parent_id);
  m_insert_query->bind(9, item.path);
  m_insert_query->bind(10, item.launch_args);
  m_insert_query->bind(11, item.on_enter);
  m_insert_query->bind(12, item.on_tab);
  m_insert_query->bind(13, item.on_query_applicable);
  m_insert_query->exec();
}
//----

void database::add_or_update_item(const database_item &item)
{
  // bind values and execute
  m_update_query->bind(0, item.title);
  m_update_query->bind(1, item.description);
  m_update_query->bind(2, item.is_transient);
  m_update_query->bind(3, unsigned(item.icon_info.source));
  m_update_query->bind(4, item.icon_info.path);
  m_update_query->bind(5, item.index_version);
  m_update_query->bind(6, item.parent_id);
  m_update_query->bind(7, item.path);
  m_update_query->bind(8, item.launch_args);
  m_update_query->bind(9, item.on_enter);
  m_update_query->bind(10, item.on_tab);
  m_update_query->bind(11, item.on_query_applicable);
  m_update_query->bind(12, item.plugin_id);
  m_update_query->bind(13, item.item_id);
  m_update_query->exec();
  if(!m_db.get_num_affected_rows())
    add_item(item);
}
//----

void database::delete_old_items(const std::wstring &plugin_name, boost::uint64_t current_index_version)
{
  // delete history of old items
  m_delete_old_item_history_query->bind(0, plugin_name);
  m_delete_old_item_history_query->bind(1, current_index_version);
  m_delete_old_item_history_query->exec();

  // bind old items
  m_delete_old_items_query->bind(0, plugin_name);
  m_delete_old_items_query->bind(1, current_index_version);
  m_delete_old_items_query->exec();
}
//----

void database::delete_unindexed_items(const std::wstring &plugin_name)
{
  // delete history of unindexed_item items
  m_delete_unindexed_item_history_query->bind(0, plugin_name);
  m_delete_unindexed_item_history_query->exec();

  // bind unindexed_item items
  m_delete_unindexed_items_query->bind(0, plugin_name);
  m_delete_unindexed_items_query->exec();
}
//----------------------------------------------------------------------------

void database::update_history(boost::uint64_t id, const wstring &term)
{
  std::shared_ptr<sqlite_statement> query=m_db.prepare(L"SELECT id FROM item_history WHERE item_id = ? AND term = ?");
  query->bind(0, id);
  query->bind(1, normalized_term(term));
  query->exec();
  if(*query)
  {
    const boost::uint64_t history_id=query->get_uint64();
    std::shared_ptr<sqlite_statement> query=m_db.prepare(L"UPDATE item_history SET last_invokation = datetime('now'), invokation_count = invokation_count + 1 WHERE id = ?");
    query->bind(0, history_id);
    query->exec();
  }
  else
  {
    std::shared_ptr<sqlite_statement> query=m_db.prepare(L"INSERT INTO item_history (item_id, term, last_invokation, invokation_count) VALUES (?, ?, datetime('now'), 1)");
    query->bind(0, id);
    query->bind(1, normalized_term(term));
    query->exec();
  }
}
//----------------------------------------------------------------------------
