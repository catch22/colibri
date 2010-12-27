//============================================================================
// search_engines_plugin.h: Search Engines plugin
//
// (c) Michael Walter, 2005-2007
//============================================================================

#ifndef COLIBRI_PLUGINS_SEARCH_ENGINES_PLUGIN_H
#define COLIBRI_PLUGINS_SEARCH_ENGINES_PLUGIN_H
#include "plugin.h"
#include "../gui/gui.h"
#include "../gui/controller.h"
//----------------------------------------------------------------------------

// Interface:
class search_engines_plugin;
class search_engine_action;
//----------------------------------------------------------------------------


//============================================================================
// search_engines_plugin
//============================================================================
class search_engines_plugin: public plugin
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


//============================================================================
// search_engine_controller
//============================================================================
class search_engine_controller: public controller
{
public:
  // construction
  search_engine_controller(const std::wstring &url_template);
  //--------------------------------------------------------------------------

  // behaviour
  virtual bool on_enter(gui&);
  //--------------------------------------------------------------------------

private:
  std::wstring m_url_template;
};
//----------------------------------------------------------------------------

#endif