//============================================================================
// winamp_plugin.h: Audio plugin
//
// (c) Torsten Schröder, 2006
//============================================================================

#ifndef COLIBRI_PLUGINS_WINAMP_PLUGIN_H
#define COLIBRI_PLUGINS_WINAMP_PLUGIN_H
#include "plugin.h"
//----------------------------------------------------------------------------

// Interface:
class winamp_plugin;
//----------------------------------------------------------------------------

//============================================================================
// winamp_plugin
//============================================================================
class winamp_plugin: public plugin
{
public:
  // construction and destruction
  winamp_plugin();
  ~winamp_plugin();
  //--------------------------------------------------------------------------

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
  //--------------------------------------------------------------------------

private:
};

#endif
