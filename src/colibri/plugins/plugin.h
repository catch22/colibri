//============================================================================
// plugin.h: Plugin base class
//
// (c) Michael Walter, 2005-2007
//============================================================================

#ifndef COLIBRI_PLUGINS_PLUGIN_H
#define COLIBRI_PLUGINS_PLUGIN_H
#include "../db/db.h"
//----------------------------------------------------------------------------

// Interface:
class plugin;
//----------------------------------------------------------------------------


//============================================================================
// plugin
//============================================================================
class plugin
{
public:
  // construction and destructioon
  void init(database&);
  void set_gui(gui&);
  virtual ~plugin();
  //--------------------------------------------------------------------------

  // information
  gui &get_gui() const;
  database &get_db() const;
  virtual const wchar_t *get_name() const=0;
  virtual const wchar_t *get_title() const=0;
  //--------------------------------------------------------------------------

  // config management
  sqlite_connection &get_config() const;
  virtual unsigned update_config(unsigned current_version)=0;
  //--------------------------------------------------------------------------

  // indexing
  void update_index();
  virtual void index(boost::uint64_t new_index_version)=0;
  //--------------------------------------------------------------------------

  // action handling
  virtual bool on_action(const std::wstring &name, boost::optional<database_item> target);
  //--------------------------------------------------------------------------

private:
  std::shared_ptr<sqlite_connection> m_config;
  gui *m_gui;
  database *m_db;
};
//----------------------------------------------------------------------------

#endif