//============================================================================
// db_controller.h: Database controller
//
// (c) Michael Walter, 2005-2007
//============================================================================

#ifndef COLIBRI_DB_CONTROLLER_H
#define COLIBRI_DB_CONTROLLER_H
#include "../gui/controller.h"
#include <hash_map>
#include <boost/optional.hpp>
class database;
struct database_item;
//----------------------------------------------------------------------------

// Interface:
class db_controller;
//----------------------------------------------------------------------------


//============================================================================
// db_controller
//============================================================================
class db_controller: public controller
{
public:
  // construction
  db_controller(database&, boost::optional<boost::uint64_t> parent_id=boost::none);
  //--------------------------------------------------------------------------

  // behavior
  virtual bool on_tab(gui&);
  virtual bool on_enter(gui&);
  virtual void on_input_changed(gui&);
  //--------------------------------------------------------------------------

private:
  static bool is_url(std::wstring&);
  boost::optional<database_item> create_or_get_current_item(gui&);
  //--------------------------------------------------------------------------

  typedef stdext::hash_map<boost::uint64_t, database_item> items;
  database &m_db;
  boost::optional<boost::uint64_t> m_parent_id;
  items m_items;
};
//----------------------------------------------------------------------------

#endif
