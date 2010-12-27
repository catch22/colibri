//============================================================================
// plugin.cpp: Plugin base class
//
// (c) Michael Walter, 2005-2007
//============================================================================

#include "plugin.h"
using namespace std;
//----------------------------------------------------------------------------


//============================================================================
// plugin
//============================================================================
void plugin::init(database &db)
{
  // initialize subsystem pointers
  m_gui=0;
  m_db=&db;

  // open config database
  const std::wstring name=get_name();
  m_config.reset(new sqlite_connection((profile_folder() / (name + L".sqlite")).string()));

  // update settings (and schema)
  unsigned current_version=m_config->begin_schema_update();
  m_config->prepare(L"INSERT OR IGNORE INTO meta (key, value) VALUES ('plugin.next_index_version', 1)")->exec();
  unsigned new_version=update_config(current_version);
  m_config->end_schema_update(name.c_str(), new_version);

  // trigger plugin-specific startup code
  on_action(L"global.startup", boost::none);
}
//----

void plugin::set_gui(gui &gui)
{
  m_gui=&gui;
}
//----

plugin::~plugin()
{
}
//----------------------------------------------------------------------------

gui &plugin::get_gui() const
{
  return *m_gui;
}
//----

database &plugin::get_db() const
{
  return *m_db;
}
//----------------------------------------------------------------------------

sqlite_connection &plugin::get_config() const
{
  return const_cast<sqlite_connection&>(*m_config);
}
//----------------------------------------------------------------------------

void plugin::update_index()
{
  boost::uint64_t next_index_version=m_config->prepare(L"SELECT value FROM meta WHERE key = 'plugin.next_index_version'")->exec().get_uint64();
  index(next_index_version);
  get_db().delete_old_items(get_name(), next_index_version);
  m_config->prepare(L"UPDATE meta SET value = ? WHERE key = 'plugin.next_index_version'")->bind(0, next_index_version+1).exec();
}
//----

void plugin::index(boost::uint64_t index_version)
{
}
//----------------------------------------------------------------------------

bool plugin::on_action(const std::wstring &name, boost::optional<database_item> target)
{
  return false;
}
//----------------------------------------------------------------------------
