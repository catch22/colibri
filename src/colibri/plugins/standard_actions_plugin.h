//============================================================================
// standard_actions_plugin.h: Standard Actions plugin
//
// (c) Michael Walter, 2005-2007
//============================================================================

#ifndef COLIBRI_PLUGINS_STANDARD_ACTIONS_PLUGIN_H
#define COLIBRI_PLUGINS_STANDARD_ACTIONS_PLUGIN_H
#include "plugin.h"
//----------------------------------------------------------------------------

// Interface:
class standard_actions_plugin;
//----------------------------------------------------------------------------


//============================================================================
// standard_actions_plugin
//============================================================================
class standard_actions_plugin: public plugin
{
public:
  // information
  virtual const wchar_t *get_name() const;
  virtual const wchar_t *get_title() const;
  //--------------------------------------------------------------------------

  // config management
  virtual unsigned update_config(unsigned current_version);
  //--------------------------------------------------------------------------

  // indexing
  virtual void index(boost::uint64_t new_index_version);
  //--------------------------------------------------------------------------

  // action handling
  virtual bool on_action(const std::wstring &name, boost::optional<database_item> target);
};
//----------------------------------------------------------------------------

#endif