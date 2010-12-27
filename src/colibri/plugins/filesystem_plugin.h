//============================================================================
// filesystem_plugin.h: File system plugin
//
// (c) Michael Walter, 2005-2007
//============================================================================

#ifndef COLIBRI_PLUGINS_FILESYSTEM_PLUGIN_H
#define COLIBRI_PLUGINS_FILESYSTEM_PLUGIN_H
#include "plugin.h"
//----------------------------------------------------------------------------

// Interface:
class filesystem_plugin;
//----------------------------------------------------------------------------


//============================================================================
// filesystem_plugin
//============================================================================
class filesystem_plugin: public plugin
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

private:
  void index(database&, boost::uint64_t new_index_version, const std::wstring &folder);
};
//----------------------------------------------------------------------------

#endif