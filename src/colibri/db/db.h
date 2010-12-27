//============================================================================
// db.h: Database
//
// (c) Michael Walter, 2005-2007
//============================================================================

#ifndef COLIBRI_DB_DB_H
#define COLIBRI_DB_DB_H
#include "../core/defs.h"
#include "../libraries/db/sqlite.h"
#include "../libraries/win32/gfx.h"
#include <vector>
class gui;
class plugin;
class colibri_plugin;
//----------------------------------------------------------------------------

// Interface:
struct database_item;
class action;
class database;
//----------------------------------------------------------------------------


//============================================================================
// database_item
//============================================================================
struct database_item
{
  // mandatory information
  boost::optional<boost::uint64_t> id;
  std::wstring plugin_id;
  std::wstring item_id;
  std::wstring title;
  std::wstring description;
  bool is_transient;
  icon_info icon_info;
  boost::optional<boost::uint64_t> index_version;

  // facets
  boost::optional<boost::uint64_t> parent_id;
  boost::optional<std::wstring> path;
  boost::optional<std::wstring> launch_args;
  boost::optional<std::wstring> on_enter;
  boost::optional<std::wstring> on_tab;
  boost::optional<std::wstring> on_query_applicable;
};
//----------------------------------------------------------------------------


//============================================================================
// database_result_set
//============================================================================
class database_result_set
{
public:
  // iteration
  operator const void*() const;
  void next();
  //--------------------------------------------------------------------------

  // accessors
  const database_item &get_item() const;
  float get_history_score() const;
  float get_match_score() const;
  const std::wstring &get_marked_up_title() const;
  //--------------------------------------------------------------------------

private:
  database_result_set(std::shared_ptr<sqlite_statement>);
  friend class database;
  //--------------------------------------------------------------------------

  std::shared_ptr<sqlite_statement> m_stmt;
  database_item m_item;
  float m_history_score;
  float m_match_score;
  std::wstring m_marked_up_title;
};
//----------------------------------------------------------------------------


//============================================================================
// database
//============================================================================
class database
{
public:
  // construction
  database();
  //--------------------------------------------------------------------------

  // plugins
  void add_plugin(std::shared_ptr<plugin>);
  plugin &get_plugin(const std::wstring &name);
  colibri_plugin &get_colibri_plugin();
  void set_gui(gui&);
  void update_index();
  //--------------------------------------------------------------------------

  // actions
  bool trigger_action(std::wstring name, boost::optional<database_item> target=boost::none);
  //--------------------------------------------------------------------------

  // item lookup
  database_result_set search(const std::wstring &term, boost::optional<boost::uint64_t> parent_id=boost::none);
  database_item get_item_for_id(boost::uint64_t id) const;
  //--------------------------------------------------------------------------

  // item management
  void add_item(const database_item&);
  void add_or_update_item(const database_item&);
  void delete_old_items(const std::wstring &plugin_name, boost::uint64_t current_index_version);
  void delete_unindexed_items(const std::wstring &plugin_name);
  //--------------------------------------------------------------------------

  // history management
  void update_history(boost::uint64_t id, const std::wstring &term);
  //--------------------------------------------------------------------------

private:
  typedef std::vector<std::shared_ptr<plugin> > plugins;
  gui *m_gui;
  plugins m_plugins;
  sqlite_connection m_db;
  std::shared_ptr<sqlite_statement> m_insert_query, m_update_query;
  std::shared_ptr<sqlite_statement> m_delete_old_item_history_query, m_delete_old_items_query;
  std::shared_ptr<sqlite_statement> m_delete_unindexed_item_history_query, m_delete_unindexed_items_query;
};
//----------------------------------------------------------------------------

#endif