//============================================================================
// audio_plugin.h: Audio plugin
//
// (c) Michael Walter, 2006
//============================================================================

#ifndef COLIBRI_PLUGINS_AUDIO_PLUGIN_H
#define COLIBRI_PLUGINS_AUDIO_PLUGIN_H
#include "plugin.h"
//----------------------------------------------------------------------------

// Interface:
class audio_plugin;
//----------------------------------------------------------------------------


//============================================================================
// audio_plugin
//============================================================================
class audio_plugin: public plugin
{
public:
  // construction and destruction
  audio_plugin();
  ~audio_plugin();
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
  HMIXEROBJ m_mixer;
  DWORD m_volume_control_id, m_mute_control_id;
  DWORD m_volume_min, m_volume_max, m_volume_step;
  unsigned m_num_channels;
};
//----------------------------------------------------------------------------

#endif