//============================================================================
// colibri_plugin.h: Colibri plugin
//
// (c) Michael Walter, 2006
//============================================================================

#ifndef COLIBRI_PLUGINS_COLIBRI_PLUGIN_H
#define COLIBRI_PLUGINS_COLIBRI_PLUGIN_H
#include "plugin.h"
#include "../gui/gui.h"
#include <boost/thread/condition.hpp>
//----------------------------------------------------------------------------

// Interface:
struct hotkey;
class colibri_plugin;
//----------------------------------------------------------------------------


//============================================================================
// hotkey
//============================================================================
struct hotkey
{
  bool ctrl;
  bool alt;
  bool shift;
  bool lwin;
  bool rwin;
  DWORD vk;
};
//----

std::wstring str(const hotkey&);
//----------------------------------------------------------------------------

//============================================================================
// colibri_plugin
//============================================================================
class colibri_plugin: public plugin
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
  //--------------------------------------------------------------------------

  // configuration
  bool is_auto_startup_enabled() const;
  void enable_auto_startup(bool);
  bool is_splash_screen_enabled() const;
  void enable_splash_screen(bool);
  bool is_hotkey_enabled() const;
  void enable_hotkey(bool);
  bool are_custom_items_stored() const;
  void store_custom_items(bool);
  bool check_for_updates() const;
  void set_check_for_updates(bool);
  std::wstring get_theme() const;
  void set_theme(const std::wstring&);
  std::wstring get_monitor() const;
  void set_monitor(const std::wstring&);
  hotkey get_hotkey() const;
  void set_hotkey(const hotkey&);
  //--------------------------------------------------------------------------

private:
  void perform_update_check();
  boost::condition m_feedback_allowed;
};
//----------------------------------------------------------------------------

#endif
