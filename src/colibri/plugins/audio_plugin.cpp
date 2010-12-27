//============================================================================
// audio_plugin.cpp: Audio plugin
//
// (c) Michael Walter, 2006
//============================================================================

#include "audio_plugin.h"
#include "../gui/gui.h"
#include "../gui/controller.h"
#include "../libraries/log/log.h"
#pragma comment(lib, "winmm.lib")
using namespace std;
//----------------------------------------------------------------------------


//============================================================================
// audio_plugin
//============================================================================
audio_plugin::audio_plugin()
  :m_mixer(0)
{
  // no mixer support available?
  if(!mixerGetNumDevs())
    return;

  // open wave out device
  WAVEFORMATEX format={WAVE_FORMAT_PCM, 1, 22050, 22050*2, 2, 16, 0};
  HWAVEOUT wave_out;
  if(MMSYSERR_NOERROR!=waveOutOpen(&wave_out, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL))
  {
    logger::warn("Unable to open wave out device (22kbps mono).");
    return;
  }

  // open mixer
  if(MMSYSERR_NOERROR!=mixerOpen(LPHMIXER(&m_mixer), UINT(size_t(wave_out)), 0, 0, MIXER_OBJECTF_HWAVEOUT))
  {
    logger::warn("Unable to open mixer.");
    waveOutClose(wave_out);
    return;
  }
  waveOutClose(wave_out);

  // get mixer line information
  MIXERLINEW mixer_line;
  mixer_line.cbStruct=sizeof(MIXERLINEW);
  mixer_line.dwComponentType=MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
  if(MMSYSERR_NOERROR!=mixerGetLineInfoW(m_mixer, &mixer_line, MIXER_GETLINEINFOF_COMPONENTTYPE))
  {
    logger::warn("Unable to get mixer destination line information.");
    mixerClose(HMIXER(m_mixer));
    m_mixer=0;
    return;
  }

  // get mixer line volume control information
  MIXERCONTROLW control;
  control.cbStruct=sizeof(MIXERCONTROLW);
  MIXERLINECONTROLSW line_controls;
  line_controls.cbStruct=sizeof(MIXERLINECONTROLSW);
  line_controls.dwControlType=MIXERCONTROL_CONTROLTYPE_VOLUME;
  line_controls.dwLineID=mixer_line.dwLineID;
  line_controls.cControls=1;
  line_controls.cbmxctrl=sizeof(MIXERCONTROLW);
  line_controls.pamxctrl=&control;
  if(MMSYSERR_NOERROR!=mixerGetLineControlsW(m_mixer, &line_controls, MIXER_GETLINECONTROLSF_ONEBYTYPE))
  {
    logger::warn("Unable to get mixer line volume control information.");
    mixerClose(HMIXER(m_mixer));
    m_mixer=0;
    return;
  }

  // store information
  m_num_channels=mixer_line.cChannels;
  m_volume_control_id=control.dwControlID;
  m_volume_min=control.Bounds.dwMinimum;
  m_volume_max=control.Bounds.dwMaximum;
  m_volume_step=control.Metrics.cSteps;

  // get mixer line mute control information
  line_controls.dwControlType=MIXERCONTROL_CONTROLTYPE_MUTE;
  if(MMSYSERR_NOERROR!=mixerGetLineControlsW(m_mixer, &line_controls, MIXER_GETLINECONTROLSF_ONEBYTYPE))
  {
    logger::warn("Unable to get mixer line mute control information.");
    mixerClose(HMIXER(m_mixer));
    m_mixer=0;
    return;
  }
  m_mute_control_id=control.dwControlID;
}
//----

audio_plugin::~audio_plugin()
{
  mixerClose(HMIXER(m_mixer));
}
//----------------------------------------------------------------------------

const wchar_t *audio_plugin::get_name() const
{
  return L"audio_plugin";
}
//----

const wchar_t *audio_plugin::get_title() const
{
  return L"Audio Plugin";
}
//----------------------------------------------------------------------------

unsigned audio_plugin::update_config(unsigned current_version)
{
  switch(current_version)
  {
  case 0:
    // version 1 doesn't have setting
    ;
  }
  return 1;
}
//----------------------------------------------------------------------------

void audio_plugin::index(boost::uint64_t new_index_version)
{
  // add "Volume Control" item
  database_item item;
  item.plugin_id=get_name();
  item.item_id=L"Volume Control";
  item.title=L"Volume Control";
  item.description=L"Control audio playback volume";
  item.is_transient=false;
  item.icon_info.set(icon_source_theme, L"audio/volume_control");
  item.index_version=new_index_version;
  item.on_enter=L"audio_actions.open_volume_control_menu";
  item.on_tab=L"audio_actions.open_volume_control_menu";
  get_db().add_or_update_item(item);
}
//----------------------------------------------------------------------------

bool audio_plugin::on_action(const std::wstring &name, boost::optional<database_item> target)
{
  if(name==L"audio_actions.open_volume_control_menu")
  {
    // open volume control menu
    menu_controller *ctrl=new menu_controller(get_db());
    ctrl->add_item(L"Increase Volume", L"Increase audio playback volume", L"audio/increase_volume", L"audio_actions.increase_volume");
    ctrl->add_item(L"Decrease Volume", L"Decrease audio playback volume", L"audio/decrease_volume", L"audio_actions.decrease_volume");
    ctrl->add_item(L"Mute", L"Mute audio playback", L"audio/mute", L"audio_actions.mute");
    get_gui().push_option_brick(ctrl);
    return true;
  }
  else if((name==L"audio_actions.increase_volume" || name==L"audio_actions.decrease_volume") && m_mixer)
  {
    const float delta_pct=name==L"audio_actions.increase_volume" ? 0.1f : -0.1f;

    // determine current volume
    MIXERCONTROLDETAILS_UNSIGNED *details=new MIXERCONTROLDETAILS_UNSIGNED[m_num_channels];
    MIXERCONTROLDETAILS control_details;
    control_details.cbStruct=sizeof(MIXERCONTROLDETAILS);
    control_details.dwControlID=m_volume_control_id;
    control_details.cChannels=m_num_channels;
    control_details.cMultipleItems=0;
    control_details.cbDetails=sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    control_details.paDetails=details;
    if(MMSYSERR_NOERROR!=mixerGetControlDetails(m_mixer, &control_details, MIXER_GETCONTROLDETAILSF_VALUE))
    {
      logger::warn("Unable to determine current volume.");
      delete []details;
      return false;
    }
    DWORD volume=details[0].dwValue;

    // calculate new volume
    long delta=long(delta_pct*float(m_volume_max-m_volume_min));
    if(delta<0 && volume-m_volume_min<DWORD(-delta))
      volume=m_volume_min;
    else if(delta>0 && m_volume_max-volume<DWORD(delta))
      volume=m_volume_max;
    else
      volume+=delta;
    for(unsigned i=0; i<m_num_channels; ++i)
      details[i].dwValue=volume;

    // set new volume
    if(MMSYSERR_NOERROR!=mixerSetControlDetails(m_mixer, &control_details, MIXER_SETCONTROLDETAILSF_VALUE))
    {
      logger::warn("Unable to set new volume.");
      delete []details;
      return false;
    }
    delete []details;
    return true;
  }
  else if(name==L"audio_actions.mute" && m_mixer)
  {
    // determine current volume
    MIXERCONTROLDETAILS_BOOLEAN *details=new MIXERCONTROLDETAILS_BOOLEAN[m_num_channels];
    MIXERCONTROLDETAILS control_details;
    control_details.cbStruct=sizeof(MIXERCONTROLDETAILS);
    control_details.dwControlID=m_mute_control_id;
    control_details.cChannels=m_num_channels;
    control_details.cMultipleItems=0;
    control_details.cbDetails=sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    control_details.paDetails=details;
    if(MMSYSERR_NOERROR!=mixerGetControlDetails(m_mixer, &control_details, MIXER_GETCONTROLDETAILSF_VALUE))
    {
      logger::warn("Unable to determine current mute state.");
      delete []details;
      return false;
    }

    // calculate new volume
    for(unsigned i=0; i<m_num_channels; ++i)
      details[i].fValue=!details[i].fValue;

    // set new volume
    if(MMSYSERR_NOERROR!=mixerSetControlDetails(m_mixer, &control_details, MIXER_SETCONTROLDETAILSF_VALUE))
    {
      logger::warn("Unable to set new mute state.");
      delete []details;
      return false;
    }
    delete []details;
    return true;
  }

  return false;
}
//----------------------------------------------------------------------------