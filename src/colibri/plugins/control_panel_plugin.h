//============================================================================
// control_panel_plugin.h: Control panel plugin
//
// (c) Michael Walter, 2006
//============================================================================

#ifndef COLIBRI_PLUGINS_CONTROL_PANEL_PLUGIN_H
#define COLIBRI_PLUGINS_CONTROL_PANEL_PLUGIN_H
#include "plugin.h"
//----------------------------------------------------------------------------

// Interface:
class control_panel_plugin;
//----------------------------------------------------------------------------


//============================================================================
// control_panel_plugin
//============================================================================
class control_panel_plugin: public plugin
{
public:
  // information
  virtual const wchar_t *get_name() const;
  virtual const wchar_t *get_title() const;
  //--------------------------------------------------------------------------

  // config management
  virtual unsigned update_config(unsigned current_version);
  //--------------------------------------------------------------------------

  // index
  virtual void index(boost::uint64_t new_index_version);
  //--------------------------------------------------------------------------

private:
  void index(database&, boost::uint64_t index_version, const std::wstring &path);
};
//----------------------------------------------------------------------------

#endif